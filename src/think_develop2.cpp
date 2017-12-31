#include <iostream>
//#include <cstdint> //int64_t
#include <bitset> //bitset
#include <utility> //pair
#include <vector> //vector
#include <memory> //shared_ptr
#include <list> //list
#include <functional>//bind
#include <algorithm>
#include<random>

using namespace std;

/*要求の読み方
  vector<bitset<64> >
  [0]自分のコマ
  [1]誰かのコマ
  [2]空き
  1なら真。
  どこにも書いてなければdon't care
  don't careの上は必ずdon't careという前提
  
  bitsetは、x(0~15)番目のポールのz(0~3)の高さのコマが、
  [x*4+z]と扱われる
*/

class Rand_int{
public:
  Rand_int (int low,int high,int seed=0): r(bind(std::uniform_int_distribution<>(low,high),std::default_random_engine(seed))) {}
  int operator() (){return r();}
private:
  std::function<int()> r;
};


ostream& operator<<(ostream& s,const vector<bitset<64> >& state){
  if(state.size()==3){//要求
    for(int y=0;y<4;y++){
      for(int z=0;z<4;z++){
	for(int x=0;x<4;x++){
	  if(state[0][y*16+x*4+z])s<<"M";//自分
	  else if(state[1][y*16+x*4+z])s<<"A";//any ball
	  else if(state[2][y*16+x*4+z])s<<"E";//empty
	  else s<<"D";//don't care
	}
	s<<"  ";
      }
      s<<endl;
    }
    return s;
  }
  if(state.size()==2){//実際の状態
    for(int y=0;y<4;y++){
      for(int z=0;z<4;z++){
	for(int x=0;x<4;x++){
	  if(state[0][y*16+x*4+z])s<<"M";//自分
	  else if(state[1][y*16+x*4+z])s<<"O";//敵
	  else s<<"E";
	}
	s<<"  ";
      }
      s<<endl;
    }
    return s;
  }
}

ostream& operator<<(ostream& s,const pair<vector<bitset<64> >,bitset<64> >& pre){
  for(int y=0;y<4;y++){
    for(int z=0;z<4;z++){
      for(int x=0;x<4;x++){
	if(pre.first[0][y*16+x*4+z])s<<"M";//自分
	else if(pre.first[1][y*16+x*4+z])s<<"A";//any ball
	else if(pre.first[2][y*16+x*4+z])s<<"E";//empty
	else s<<"D";//don't care
      }
      s<<"  ";
    }
    s<<"-> ";
    for(int z=0;z<4;z++){
      for(int x=0;x<4;x++){
	if(pre.second[y*16+x*4+z])s<<"X";//自分がとる行動
	else s<<"-";//don't care
      }
      s<<"  ";
    }
    s<<endl;
  }
  return s;
}

ostream& operator<<(ostream& s,const bitset<64>& st){
  //相手が妨害のためとる行動
  for(int y=0;y<4;y++){
    for(int z=0;z<4;z++){
      for(int x=0;x<4;x++){
	if(st[y*16+x*4+z])s<<"X";//相手がとるべきがとる行動
	else s<<"-";//don't care
      }
      s<<" ";
    }
    s<<endl;
  }
  return s;
}

class state{
protected:
public:
  state();
  vector<bitset<64> > now_state;//自分のターン終了時の状態
  vector<pair<vector<bitset<64> >,bitset<64> > > pre{};//自分のターン開始時の状態の条件と、その時にこの状態になるために自分がとる行動のペアたち
  vector<pair<bitset<64> ,shared_ptr<state> > > next{};//次に至るのを対戦相手が妨害できる行動(実際に置けるとは限らない)と、次の状態
  int count=0;//ビンゴまでの手数(最大)
  //bool man=false;//強制配置に有効か

  void genpre(bool) noexcept;//now_stateからpreを自動生成する
};

state::state(): now_state(3) {
}

void state::genpre(bool same=false) noexcept{//now_stateからpreを自動生成する
  pre.clear();
  //now_stateから上からコマを一つ取り除けばよい
  for(int x=0;x<16;x++){
    for(int z=3;z>-1;z--){
      if(now_state[0][x*4+z]||now_state[1][x*4+z]){//一番上のコマ発見
	vector<bitset<64> > temp=now_state;
	temp[0][x*4+z]=0;
	temp[1][x*4+z]=0;//コマを取り除く
	for(int _z=z;_z<4;_z++)temp[2][x*4+_z]=true;//そこから上を空にする
	bitset<64> myact{};
	myact[x*4+z]=true;
	pre.emplace_back(move(temp),move(myact));
	break;
      }
    }
  }
  if(same){//すでに状況が出来上がっていて、自分がdon't careの場所に置いた。(土台を相手が置いてくれるのを待つ時のみ)
    vector<bitset<64> > temp=now_state;
    bitset<64> myact{};
    myact.set();
    for(auto& req:now_state) myact&=~req;//don't careを抽出
    pre.emplace_back(move(temp),move(myact));
  }
}

vector<shared_ptr<state> > make_child(const shared_ptr<state>& parent) noexcept{//parentのpreから一手前のstateを作る
  //前回のターンにどういう盤面で相手にターンを渡すかということ
  vector<shared_ptr<state> > result{};
  for(auto& pre:parent->pre){
    //parentのpreそのまま。相手が明後日の場所に置く
    shared_ptr<state> temp{new state};
    temp->now_state=pre.first;
    temp->genpre();
    bitset<64> interfere{};
    //要求を満たせなくする相手のアクションとは、空要求の場所を(次に自分が置きたい場所を含む)を埋めることである。埋めることができる場所は、一番下のempty
    for(int x=0;x<16;x++){
      for(int z=0;z<4;z++){
	if(temp->now_state[2][x*4+z]){
	  interfere[x*4+z]=1;
	  break;
	}
      }
    }
    temp->next.emplace_back(move(interfere),parent);
    temp->count=parent->count+1;
    result.push_back(move(temp));

    //parentのpreから土台を一つ取り除く。相手にそこに置いてもらう。
    for(int x=0;x<16;x++){
      for(int z=3;z>-1;z--){
	if(pre.first[0][x*4+z])break;
	if(pre.first[1][x*4+z]){

	  shared_ptr<state> temp{new state};
	  temp->now_state = pre.first;
	  temp->now_state[1][x*4+z]=0;
	  temp->now_state[2][x*4+z]=1;//前提条件より、上は全てemptyのはずなので処理しない
	  temp->genpre(true);

	  bitset<64> interfere{};
	  //取り除いた土台の場所以外の場所に置くと妨害できる
	  //don't careの場所、または、取り除いた土台以外の場所の一番下のempty
	  interfere.set();
	  for(auto& req:pre.first) interfere&=~req;//don't careを抽出
	  //一番下のemptyを抽出

	  for(int _x=0;_x<16;_x++){
	    for(int _z=0;_z<4;_z++){
	      if(_x==x)break;//取り除いた土台の場所以外
	      if(pre.first[2][_x*4+_z]){
		interfere[_x*4+_z]=1;
		break;
	      }
	    }
	  }
	  temp->next.emplace_back(move(interfere),parent);
	  temp->count=parent->count+1;
	  //if(parent->man&&(z<3)&&(pre.second[x*4+z+1]))temp->man=1;
	  result.push_back(move(temp));
	  break;
	}
      }
    }
  }
  return result;
}

shared_ptr<state> merge(const shared_ptr<state>& state1,const shared_ptr<state>& state2) noexcept{//２つのx1stateがダブルリーチ可能ならmerge、そうでないならnullptr
  //要求を同時に満たすことが可能で、その時の自分の行動を共有し、相手が止める行動が重複しなければよい
  //x1は、nextは1つずつしかない(前提)
  if((state1->next[0].first&state2->next[0].first).any())return nullptr;//相手が止める行動が重複
  shared_ptr<state> result=nullptr;
  for(auto& pre1: state1->pre){
    for(auto& pre2: state2->pre){
      if((pre1.second&pre2.second).none())continue;//自分の行動を共有しない
      if((((pre1.first[0]|pre1.first[1])&pre2.first[2])|((pre2.first[0]|pre2.first[1])&pre1.first[2])).any())continue;//要求を同時に満たせない
      if(result==nullptr){
	result=make_shared<state>();
	result->now_state[0]=state1->now_state[0]|state2->now_state[0];
	result->now_state[1]=(state1->now_state[1]&(~state2->now_state[0]))|(state2->now_state[1]&(~state1->now_state[0]));
	result->now_state[2]=state1->now_state[2]|state2->now_state[2];
	result->next.push_back(state1->next[0]);
	result->next.push_back(state2->next[0]);
	result->count=max(state1->count,state2->count)+1;
      }
      vector<bitset<64> > temp(3);//pre
      temp[0]=pre1.first[0]|pre2.first[0];
      temp[1]=(pre1.first[1]&(~pre2.first[0]))|(pre2.first[1]&(~pre1.first[0]));
      temp[2]=pre1.first[2]|pre2.first[2];
      result->pre.emplace_back(temp,(pre1.second&pre2.second));
    }
  }
  /*
  shared_ptr<state> result=nullptr;
  for(auto& next1:state1->next){
    for(auto& next2: state2->next){
      if((next1.first&next2.first).any())continue;//相手が止める行動が重複
      for(auto& pre1: state1->pre){
	for(auto& pre2: state2->pre){
	  if((pre1.second&pre2.second).none())continue;//自分の行動を共有しない
	  if((((pre1.first[0]|pre1.first[1])&pre2.first[2])|((pre2.first[0]|pre2.first[1])&pre1.first[2])).any())continue;//要求を同時に満たせない
	  if(result==nullptr){
	    result=make_shared<state>();
	    result->now_state[0]=state1->now_state[0]|state2->now_state[0];
	    result->now_state[1]=(state1->now_state[1]&(~state2->now_state[0]))|(state2->now_state[1]&(~state1->now_state[0]));
	    result->now_state[2]=state1->now_state[2]|state2->now_state[2];
	    result->next.push_back(next1);
	    result->next.push_back(next2);
	    result->count=max(state1->count,state2->count)+1;
	  }
	  vector<bitset<64> > temp(3);//pre
	  temp[0]=pre1.first[0]|pre2.first[0];
	  temp[1]=(pre1.first[1]&(~pre2.first[0]))|(pre2.first[1]&(~pre1.first[0]));
	  temp[2]=pre1.first[2]|pre2.first[2];
	  result->pre.emplace_back(temp,(pre1.second&pre2.second));
	}
      }
    }
  }
  */
  return result;
}

bool ballnump(int myball_max,int anyball_max,const shared_ptr<state>& state) noexcept{
  if((state->now_state[0].count()>myball_max)||(state->now_state[0].count()+state->now_state[1].count()>anyball_max))return true;
  return false;
}

bool statep(const vector<bitset<64> >& real,int restturn,bool me,shared_ptr<state>& state) noexcept{
  //realstateは自分のターンが回ってきた時の状態
  //restturn後に、自分のアクションによってstateに到れるかを見ている。
  //preの条件を満たすかで判断
  if(me){
    for(auto& pre:state->pre){
      if((pre.first[0].count()>(real[0]&pre.first[0]).count()+restturn)||(pre.first[0].count()+pre.first[1].count()>((real[0]|real[1])&(pre.first[0]|pre.first[1])).count()+restturn*2)//ボールの個数に関する条件
	 ||(pre.first[0]&real[1]).any()||(pre.first[2]&(real[0]|real[1])).any())continue;
      else return false;
    }
    return true;
  }else{//対戦相手の場合、any_ballが1つ増えてもよい
    for(auto& pre:state->pre){
      if((pre.first[0].count()>(real[0]&pre.first[0]).count()+restturn)||(pre.first[0].count()+pre.first[1].count()>((real[0]|real[1])&(pre.first[0]|pre.first[1])).count()+restturn*2+1)//ボールの個数に関する条件
	 ||(pre.first[0]&real[1]).any()||(pre.first[2]&(real[0]|real[1])).any())continue;
      else return false;
    }
    return true;
  }
}

void remove_if(const vector<bitset<64> >& real,int restturn,bool me,vector<shared_ptr<state> >& states){
  vector<shared_ptr<state> >result{};
  if(me){
    for(auto& state:states){
      vector<pair<vector<bitset<64> >,bitset<64> > > newpre{};
      for(auto& pre:state->pre){
	if((pre.first[0].count()>(real[0]&pre.first[0]).count()+restturn)||(pre.first[0].count()+pre.first[1].count()>((real[0]|real[1])&(pre.first[0]|pre.first[1])).count()+restturn*2)//ボールの個数に関する条件
	   ||(pre.first[0]&real[1]).any()||(pre.first[2]&(real[0]|real[1])).any())continue;
	else newpre.push_back(move(pre));
      }
      if(!newpre.empty()){
	state->pre=move(newpre);
	result.push_back(move(state));
      }
    }
  }else{
    for(auto& state:states){
      vector<pair<vector<bitset<64> >,bitset<64> > > newpre{};
      for(auto& pre:state->pre){
	if((pre.first[0].count()>(real[0]&pre.first[0]).count()+restturn)||(pre.first[0].count()+pre.first[1].count()>((real[0]|real[1])&(pre.first[0]|pre.first[1])).count()+restturn*2+1)//ボールの個数に関する条件//anyballが一つ多い
	   ||(pre.first[0]&real[1]).any()||(pre.first[2]&(real[0]|real[1])).any())continue;
	else newpre.push_back(move(pre));
      }
      if(!newpre.empty()){
	state->pre=move(newpre);
	result.push_back(move(state));
      }
    }
  }
  states=move(result);
  return;
}

bool statep2(const vector<bitset<64> >& real,shared_ptr<state>& state) noexcept{
  //realstateは自分のターンが回ってきた時の状態
  //現在の盤面がnow_stateを満たすかを考える
  if(((~real[0])&state->now_state[0]).any()||((~real[0])&(~real[1])&(state->now_state[1])).any()||((real[0]|real[1])&(state->now_state[2])).any()) return true;
  return false;
}
void remove_if2(const vector<bitset<64> >& real,vector<shared_ptr<state> >& states){
  vector<shared_ptr<state> >result{};
  for(auto& state:states){
    if(((~real[0])&state->now_state[0]).any()||((~real[0])&(~real[1])&(state->now_state[1])).any()||((real[0]|real[1])&(state->now_state[2])).any()) continue;
    result.push_back(move(state));
  }
  states=move(result);
  return;
}


bool state_sort_func(const shared_ptr<state> state1,const shared_ptr<state> state2)noexcept{
  //now_stateによってソート
  if(state1->now_state[0].to_ullong()<state2->now_state[0].to_ullong())return true;
  else if(state1->now_state[0]==state2->now_state[0]){
    if(state1->now_state[1].to_ullong()<state2->now_state[1].to_ullong())return true;
    else if(state1->now_state[1]==state2->now_state[1]){
      if(state1->now_state[1].to_ullong()<state2->now_state[1].to_ullong())return true;
    }
  }
  return false;
}

void unique(vector<shared_ptr<state> >& x2)noexcept{//vectorからnow_stateが同じなx2をまとめる。破壊的
  if(x2.empty())return;
  vector<shared_ptr<state> > result{};
  sort(x2.begin(),x2.end(),state_sort_func);
  for(auto _state=x2.begin();_state!=x2.end();_state++){
    auto _state2=_state;
    _state2++;
    if(_state2==x2.end())break;
    if(((*_state)->now_state[0]==(*_state2)->now_state[0])&&((*_state)->now_state[1]==(*_state2)->now_state[1])&&((*_state)->now_state[2]==(*_state2)->now_state[2])){//同じなら、次のやつにまとめる
      for(auto& pre1:(*_state)->pre){
	bool same=false;
	for(auto& pre2:(*_state2)->pre){
	  if(pre1.second==pre2.second){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->pre.push_back(move(pre1));
      }
      for(auto& next1:(*_state)->next){
	bool same=false;
	for(auto& next2:(*_state2)->next){
	  if(next1.second==next2.second){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->next.push_back(move(next1));
      }
      (*_state2)->count=min((*_state)->count,(*_state2)->count);
    (*_state)=nullptr;
    }
    //同じでなければ、resultへ
    else result.push_back(move(*_state));
  }
  result.push_back(move(*(--x2.end())));
  //x2.remove(nullptr);
  x2=move(result);
  return;
}

vector<shared_ptr<state> > make_bingo(){//76個のビンゴ状態を作る
  vector<shared_ptr<state> > result{};
  //上下 16
  for(int x=0;x<16;x++){
    shared_ptr<state> temp{new state};
    for(int _z=0;_z<4;_z++)temp->now_state[0][x*4+_z]=1;
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }

  //左右 16
  for(int y=0;y<4;y++){
    for(int z=0;z<4;z++){
      shared_ptr<state> temp{new state};
      for(int _x=0;_x<4;_x++)temp->now_state[0][y*16+_x*4+z]=1;
      for(int _z=0;_z<z;_z++){
	for(int _x=0;_x<4;_x++)temp->now_state[1][y*16+_x*4+_z]=1;
      }
      temp->genpre();
      temp->count=0;
      result.push_back(move(temp));
    }
  }
  //前後16
  for(int x=0;x<4;x++){
    for(int z=0;z<4;z++){
      shared_ptr<state> temp{new state};
      for(int _y=0;_y<4;_y++)temp->now_state[0][_y*16+x*4+z]=1;
      for(int _z=0;_z<z;_z++){
	for(int _y=0;_y<4;_y++)temp->now_state[1][_y*16+x*4+_z]=1;
      }
      temp->genpre();
      temp->count=0;
      result.push_back(move(temp));
    }
  }
  
  //xy8
  for(int z=0;z<4;z++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+i*4+z]=1;
    for(int _z=0;_z<z;_z++){
      for(int i=0;i<4;i++)temp->now_state[1][i*16+i*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  for(int z=0;z<4;z++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][(3-i)*16+i*4+z]=1;
    for(int _z=0;_z<z;_z++){
      for(int i=0;i<4;i++)temp->now_state[1][(3-i)*16+i*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  
  //yz8
  for(int x=0;x<4;x++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+x*4+i]=1;
    for(int _y=0;_y<4;_y++){
      for(int _z=0;_z<_y;_z++)temp->now_state[1][_y*16+x*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  for(int x=0;x<4;x++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+x*4+(3-i)]=1;
    for(int _y=0;_y<4;_y++){
      for(int _z=0;_z<(3-_y);_z++)temp->now_state[1][_y*16+x*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  
  //zx8
  for(int y=0;y<4;y++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][y*16+i*4+i]=1;
    for(int _x=0;_x<4;_x++){
      for(int _z=0;_z<_x;_z++)temp->now_state[1][y*16+_x*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  for(int y=0;y<4;y++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][y*16+i*4+(3-i)]=1;
    for(int _x=0;_x<4;_x++){
      for(int _z=0;_z<(3-_x);_z++)temp->now_state[1][y*16+_x*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  //斜め4
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+i*4+i]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<i;_z++)temp->now_state[1][i*16+i*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+i*4+(3-i)]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<(3-i);_z++)temp->now_state[1][i*16+i*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+(3-i)*4+i]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<i;_z++)temp->now_state[1][i*16+(3-i)*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+(3-i)*4+(3-i)]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<(3-i);_z++)temp->now_state[1][i*16+(3-i)*4+_z]=1;
    }
    temp->genpre();
    temp->count=0;
    result.push_back(move(temp));
  }
  
  return result;
}

vector<bitset<64> > make_opponent_board(const vector<bitset<64> >& board) noexcept{
  vector<bitset<64> > result=board;
  swap(result[0],result[1]);
  return result;
}

bitset<64> think(const vector<bitset<64> >& board){
  vector<bitset<64> > mystart=board;
  int loopnum_max=6;
  
  vector<shared_ptr<state> > myx2{};//処理後のnew_x2をどんどんためていく
  vector<shared_ptr<state> > mynew_x2{make_bingo()};//new_new_x2からこっちに移して処理する対象とする
  vector<shared_ptr<state> > mynew_new_x2{};//x1からmergeし作られたものを一時的に入れる
  vector<shared_ptr<state> > myx1{};//new_x2から作られたものをどんどんためていく
 
  //auto func=bind(statep,start,loopnum_max,std::placeholders::_1);
  remove_if(mystart,loopnum_max,true,mynew_x2);//読む手の数を超えているもの、現在の盤面から至れないものを削除
  
  cout<<mynew_x2.size()<<" "<<myx1.size()<<endl;
  
  for(int loopnum=loopnum_max-1;loopnum>=0;loopnum--){//loopnum=この手に至るまでのターン数。0なら今回
    //ループ
    auto func=bind(statep,mystart,loopnum,true,std::placeholders::_1);//読む手の数を超えているか、現在の盤面から至れないか
    remove_if(mystart,loopnum,true,myx1);
    //x2.remove_if(func);

    for(auto& targetx2:mynew_x2){
      auto children=make_child(targetx2);//targetx2に至るためひとつ前の状態を作る
      for(auto& child:children){
	if(func(child))continue;//読む手の数を超えている,現在の盤から至れない
	//x1のリスト内から、統合できるものを探す。ダブルリーチを製造。<-ここで時間がかかる
	for(auto& target:myx1){
	  shared_ptr<state> result=merge(child,target);
	  if(result==nullptr) continue;//統合できない	  
	  if(func(result))continue;//読む手の数を超えている,現在の盤から至れない
	  mynew_new_x2.push_back(move(result));
	}
	myx1.push_back(move(child));
      }
    }

    myx2.insert(myx2.end(),mynew_x2.begin(),mynew_x2.end());
    unique(mynew_new_x2);
    mynew_x2=move(mynew_new_x2);
    remove_if(mystart,loopnum,true,myx2);
    unique(myx2);
    cout<<myx2.size()<<" "<<mynew_x2.size()<<" "<<myx1.size()<<endl;

    /*
    if(loopnum<=2){
      for(auto& result:myx2){
       if(result->count>1){
       cout<<result->now_state<<endl;
	  for(auto& pre:result->pre)cout<<pre<<endl;
	  }
      }
    }
    */    
  }
  myx2.insert(myx2.end(),mynew_x2.begin(),mynew_x2.end());
  unique(myx2);
  
  //対戦相手バージョン
  vector<bitset<64> > opstart=make_opponent_board(mystart);
  vector<shared_ptr<state> > opx2{};//処理後のnew_x2をどんどんためていく
  vector<shared_ptr<state> > opnew_x2{make_bingo()};//new_new_x2からこっちに移して処理する対象とする
  vector<shared_ptr<state> > opnew_new_x2{};//x1からmergeし作られたものを一時的に入れる
  vector<shared_ptr<state> > opx1{};//new_x2から作られたものをどんどんためていく
 
  //auto func=bind(statep,start,loopnum_max,std::placeholders::_1);
  remove_if(opstart,loopnum_max-1,false,opnew_x2);//読む手の数を超えているもの、現在の盤面から至れないものを削除。読む手は1つ少なくて良い
  
  cout<<opnew_x2.size()<<" "<<opx1.size()<<endl;
  
  for(int loopnum=loopnum_max-2;loopnum>=0;loopnum--){//loopnum=この手に至るまでのターン数。0なら今回。読む手は1つ少なくて良い
    //ループ
    auto func=bind(statep,opstart,loopnum,false,std::placeholders::_1);//読む手の数を超えているか、現在の盤面から至れないか
    remove_if(opstart,loopnum,false,opx1);
    //x2.remove_if(func);
    
    for(auto& targetx2:opnew_x2){
      auto children=make_child(targetx2);//targetx2に至るためひとつ前の状態を作る
      for(auto& child:children){
	if(func(child))continue;//読む手の数を超えている,現在の盤から至れない
	//x1のリスト内から、統合できるものを探す。ダブルリーチを製造。<-ここで時間がかかる
	for(auto& target:opx1){
	  shared_ptr<state> result=merge(child,target);
	  if(result==nullptr) continue;//統合できない	  
	  if(func(result))continue;//読む手の数を超えている,現在の盤から至れない
	  opnew_new_x2.push_back(move(result));
	}
	opx1.push_back(move(child));
      }
    }
    
    opx2.insert(opx2.end(),opnew_x2.begin(),opnew_x2.end());
    unique(opnew_new_x2);
    opnew_x2=move(opnew_new_x2);
    remove_if(opstart,loopnum,false,opx2);
    unique(opx2);
    cout<<opx2.size()<<" "<<opnew_x2.size()<<" "<<opx1.size()<<endl;
    
    /*
      if(loopnum<=2){
      for(auto& result:opx2){
      if(result->count>1){
      cout<<result->now_state<<endl;
      for(auto& pre:result->pre)cout<<pre<<endl;
      }
      }
      } 
    */   
  }
  //最後に、opx2に至るx1を現在の盤面仕様にする。つまり、x1のnow_stateが現在の盤面と同じかどうか
  opx2.insert(opx2.end(),opnew_x2.begin(),opnew_x2.end());
  unique(opx2);
  opx1.clear();
  auto func2=bind(statep2,opstart,std::placeholders::_1);//now_stateが現在の盤面と一致するか
  for(auto& targetx2:opx2){
    auto children=make_child(targetx2);//targetx2に至るためひとつ前の状態を作る
    for(auto& child:children){
      if(func2(child))continue;////now_stateが現在の盤面と一致するか
      opx1.push_back(move(child));
    }
  }

  
  for(auto& st:myx2){
    cout<<st->count<<endl;
    cout<<st->now_state<<endl;
    for(auto& pre:st->pre){
      cout<<pre<<endl;
    }
    cout<<endl;
  }
  cout<<"vs"<<endl;
  for(auto& st:opx1){
    cout<<st->count<<endl;
    cout<<st->now_state<<endl;
    for(auto& next:st->next){
      cout<<next.first<<endl;
    }
    cout<<endl;
  }

  bitset<64> myaction{};
  if(!myx2.empty()){//勝てるかも
    for(auto& x2:myx2){
      for(auto& pre:x2->pre){
	bitset<64> tempmyaction=pre.second;
	//対戦相手の、countが自分のcount以下のx1の妨害もする必要
	for(auto& x1:opx1){
	  if(x1->count<=x2->count) tempmyaction&=x1->next[0].first;
	}
	myaction|=tempmyaction;
      }
    }
  }

  if(!myaction.any()){//すぐには勝てない
    //置ける場所
    for(int x=0;x<16;x++){
      for(int z=0;z<4;z++){
	if(board[0][x*4+z]||board[1][x*4+z])continue;
	else{
	  myaction[x*4+z]=1;
	  break;
	}
      }
    }
    //対戦相手の妨害優先
    for(auto& x1:opx1){
      myaction&=x1->next[0].first;
    }
  }
  if(!myaction.any()){ //多分負け。置ける場所からランダムに
    for(int x=0;x<16;x++){
      for(int z=0;z<4;z++){
	if(board[0][x*4+z]||board[1][x*4+z])continue;
	else{
	  myaction[x*4+z]=1;
	  break;
	}
      }
    }
  }//注盤が埋まっている時の処理がない

  //選択肢の中から一つに絞る
  Rand_int rand(0,63);
  while(true){
    int num=rand();
    if(myaction[num]){
      myaction.reset();
      myaction[num]=1;
      break;
    }
  }

  return myaction;
}

bool victory(vector<bitset<64> >& board) noexcept{
  static const auto bingos=make_bingo();
  for(const auto& bingo:bingos){
    if((board[0]&bingo->now_state[0]).count()==4)return true;
    if((board[1]&bingo->now_state[0]).count()==4)return true;
  }
  return false;
}

int main(int argc ,char** argv){
  vector<bitset<64> > board(2);
  //board[0][1]=1;board[0][5]=1;board[0][9]=1;board[0][2]=1;board[0][6]=1;board[0][10]=1;
  //board[1][0]=1;board[1][4]=1;board[1][8]=1;
  while(true){
    auto action=think(board);
    cout<<action<<endl;
    board[0]|=action;
    cout<<board<<endl;
    if(victory(board)){
      cout<<"CPU win"<<endl;
      return 0;
    }
    
    while(true){
      int youraction;
      cout<<"0~63"<<endl;
      cin>>youraction;
      if(0>youraction||youraction>63){
	continue;
      }
      bitset<64> _youraction{};
      _youraction[youraction]=1;
      bitset<64> canplace{};
      for(int x=0;x<16;x++){
	for(int z=0;z<4;z++){
	  if(board[0][x*4+z]||board[1][x*4+z])continue;
	  else{
	    canplace[x*4+z]=1;
	    break;
	  }
	}
      }
      if(!(_youraction&canplace).any())continue;
      board[1]|=_youraction;
      cout<<board<<endl;
      if(victory(board)){
	cout<<"YOU win"<<endl;
	return 0;
      }
      break;
    }
  }
}
