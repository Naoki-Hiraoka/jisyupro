#include <iostream>
//#include <cstdint> //int64_t
#include <bitset> //bitset
#include <utility> //pair
#include <vector> //vector
#include <memory> //shared_ptr
#include <list> //list
#include <functional>

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

list<shared_ptr<state> > make_child(const shared_ptr<state>& parent) noexcept{//parentのpreから一手前のstateを作る
  //前回のターンにどういう盤面で相手にターンを渡すかということ
  list<shared_ptr<state> > result{};
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
  return result;
}

bool ballnump(int myball_max,int anyball_max,const shared_ptr<state>& state) noexcept{
  if((state->now_state[0].count()>myball_max)||(state->now_state[0].count()+state->now_state[1].count()>anyball_max))return true;
  return false;
}

list<shared_ptr<state> > make_bingo(){//76個のビンゴ状態を作る
  list<shared_ptr<state> > result{};
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

int main(void){
  list<shared_ptr<state> > x2{};//処理後のnew_x2をどんどんためていく
  list<shared_ptr<state> > new_x2{make_bingo()};//new_new_x2からこっちに移して処理する対象とする
  list<shared_ptr<state> > new_new_x2{};//x1からmergeし作られたものを一時的に入れる
  list<shared_ptr<state> > x1{};//new_x2から作られたものをどんどんためていく
  /*
  for(auto& bingo :new_x2){
    cout<<bingo->now_state<<endl;
    for(auto& pre:bingo->pre){
      cout<<pre<<endl;
    }
    cout<<endl;
  }
  */
  
  /*
  for(auto& bingo:temp){
    cout<<"bingo"<<endl;
    auto children=make_child(bingo);
    cout<<bingo->now_state<<endl;
    for(auto& child:children){
      cout<<child->now_state<<endl;
      for(auto& pre:child->pre){
	cout<<pre<<endl;
      }
    }
    cout<<endl;
  }
  */
  int loopnum_max=7;
  
  auto func=bind(ballnump,loopnum_max+1,loopnum_max*2+1,std::placeholders::_1);
  new_x2.remove_if(func);//読む手の数を超えている
  cout<<new_x2.size()<<" "<<x1.size()<<endl;

  
  for(int loopnum=0;loopnum<loopnum_max;loopnum++){
    //ループ
    auto func=bind(ballnump,loopnum_max-loopnum,loopnum_max*2-loopnum*2-1,std::placeholders::_1);//読む手の数
    x1.remove_if(func);
    //x2.remove_if(func);
    
    for(auto& targetx2:new_x2){
      auto children=make_child(targetx2);
      for(auto& child:children){
	if(func(child))continue;//読む手の数を超えている
	//x1のリスト内から、統合できるものを探す
	for(auto& target:x1){
	  shared_ptr<state> result=merge(child,target);
	  if(result==nullptr) continue;
	  
	  if(func(result))continue;//読む手の数を超えている

	  /*
	  if(loopnum==3&&result!=nullptr){
	    cout<<result->now_state<<endl;
	    for(auto& pre:result->pre)cout<<pre<<endl;
	  }
	  */
	  
	  new_new_x2.push_back(move(result));
	}
	x1.push_back(move(child));
      }

    }
    x2.splice(x2.end(),new_x2);
    new_x2.splice(new_x2.end(),new_new_x2);

    cout<<new_x2.size()<<" "<<x1.size()<<endl;
  }

  /*
  for(auto& dou:new_x2){
    cout<<dou->now_state<<endl;
    for(auto& pre:dou->pre){
      cout<<pre<<endl;
    }
    cout<<endl;
  }
  */
  return 0;
}
