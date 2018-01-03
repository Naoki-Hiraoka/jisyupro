#include <iostream>
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

class _pre{
protected:
public:
  _pre();
  vector<bitset<64> > board;
  bitset<64> myact{};
  bitset<64> strongA{};//myactで置きたいMの下。(make_child時にこのAを除いたらs3x1になる)
  bool ssx1=false;//sのx1のmerge可能性のあるpre
  bool swx1=false;//swのx1のmerge1可能性のあるpre
  bool unchange=false;//for make_child
};
_pre::_pre(): board(3){
}

class state;

class _next{
public:
  bitset<64> opact{};//次に至るのを対戦相手が妨害できる行動(実際に置けるとは限らない)
  shared_ptr<state> nextstate{};
};

ostream& operator<<(ostream& s,const vector<bitset<64> >& state){
  if(state.size()==3){//要求now_state,_pre.board
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
  if(state.size()==2){//実際の状態board
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

ostream& operator<<(ostream& s,const _pre& pre){
  for(int y=0;y<4;y++){
    for(int z=0;z<4;z++){
      for(int x=0;x<4;x++){
	if(pre.board[0][y*16+x*4+z])s<<"M";//自分
	else if(pre.board[1][y*16+x*4+z])s<<"A";//any ball
	else if(pre.board[2][y*16+x*4+z])s<<"E";//empty
	else s<<"D";//don't care
      }
      s<<"  ";
    }
    s<<"-> ";
    for(int z=0;z<4;z++){
      for(int x=0;x<4;x++){
	if(pre.myact[y*16+x*4+z])s<<"X";//自分がとる行動
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
  //state(const state&);
  vector<bitset<64> > now_state;//自分のターン終了時の状態
  vector<_pre> pre{};//自分のターン開始時の状態の条件と、その時にこの状態になるために自分がとる行動のペアたち
  vector<_next> next{};//次に至るのを対戦相手が妨害できる行動(実際に置けるとは限らない)と、次の状態
  int count=0;//ビンゴまでの手数(最大)
  
  bitset<8> type{};
  bitset<16> s3x1spot{};//tos3x1の、完成した場合の拘束スポット
  void genpre(bool) noexcept;//now_stateからpreを自動生成する
};
state::state(): now_state(3) {
}
/*
  state::state(const state& target){
  now_state=target.now_state;
  pre=target.
  }*/

const bitset<8> tobingo  {"00001111"};//bingoに至る
const bitset<8> tobingon {"00000001"};//bingoに至る普通
const bitset<8> tobingos2{"00000010"};//bingoに至るstrongなx2
const bitset<8> tobingos1{"00000100"};//bingoに至るstrongなx1
const bitset<8> tobingow1{"00001000"};//bingoに至るstrongweakなx1
const bitset<8> tos3x1   {"00010000"};//s3x1に至る

void state::genpre(bool unchange=false) noexcept{//now_stateからpreを自動生成する
  _pre temppre{};
  if(!unchange){//make_childとgenpreでunchangeが連続しないように
    //now_stateそのまま.自分はdon't careに置く
    temppre.board=now_state;
    temppre.myact.set();
    for(auto& req:now_state) temppre.myact&=~req;//don't careを抽出
    temppre.unchange=true;
    if((type&tobingos1).any())temppre.ssx1=true;
    pre.push_back(move(temppre));
  }
  //Mを一つ取り除く
  for(int x=0;x<16;x++){
    for(int z=3;z>-1;z--){
      if(now_state[0][x*4+z]){//一番上のコマ発見
	temppre=_pre{};
	temppre.board=now_state;
	temppre.board[0][x*4+z]=0;//コマを取り除く
	for(int _z=z;_z<4;_z++)temppre.board[2][x*4+_z]=1;//そこから上を空にする
	temppre.myact[x*4+z]=1;
	if((type&tobingow1).any()){
	  temppre.swx1=true;
	  if(z>0)temppre.strongA[x*4+z-1]=1;
	}else if(type==tobingos2){
	  if(z>0)temppre.strongA[x*4+z-1]=1;
	}
	pre.push_back(move(temppre));
	break;
      }
      if(now_state[1][x*4+z])break;
    }
  }
  //Aを一つ取り除く
  for(int x=0;x<16;x++){
    for(int z=3;z>-1;z--){
      if(now_state[1][x*4+z]){//一番上のコマ発見
	temppre=_pre{};
	temppre.board=now_state;
	temppre.board[1][x*4+z]=0;//コマを取り除く
	for(int _z=z;_z<4;_z++)temppre.board[2][x*4+_z]=1;//そこから上を空にする
	temppre.myact[x*4+z]=1;
	pre.push_back(move(temppre));
	break;
      }
      if(now_state[0][x*4+z])break;
    }
  }
}
//76個のビンゴ状態を作る
vector<shared_ptr<state> > make_bingo() noexcept{
  vector<shared_ptr<state> > result{};
  //上下 16
  for(int x=0;x<16;x++){
    shared_ptr<state> temp{new state};
    for(int _z=0;_z<4;_z++)temp->now_state[0][x*4+_z]=1;
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
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
      temp->count=0;
      temp->type=tobingos2;
      temp->genpre();
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
      temp->count=0;
      temp->type=tobingos2;
      temp->genpre();
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
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  for(int z=0;z<4;z++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][(3-i)*16+i*4+z]=1;
    for(int _z=0;_z<z;_z++){
      for(int i=0;i<4;i++)temp->now_state[1][(3-i)*16+i*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  
  //yz8
  for(int x=0;x<4;x++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+x*4+i]=1;
    for(int _y=0;_y<4;_y++){
      for(int _z=0;_z<_y;_z++)temp->now_state[1][_y*16+x*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  for(int x=0;x<4;x++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+x*4+(3-i)]=1;
    for(int _y=0;_y<4;_y++){
      for(int _z=0;_z<(3-_y);_z++)temp->now_state[1][_y*16+x*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  
  //zx8
  for(int y=0;y<4;y++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][y*16+i*4+i]=1;
    for(int _x=0;_x<4;_x++){
      for(int _z=0;_z<_x;_z++)temp->now_state[1][y*16+_x*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  for(int y=0;y<4;y++){
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][y*16+i*4+(3-i)]=1;
    for(int _x=0;_x<4;_x++){
      for(int _z=0;_z<(3-_x);_z++)temp->now_state[1][y*16+_x*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  //斜め4
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+i*4+i]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<i;_z++)temp->now_state[1][i*16+i*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+i*4+(3-i)]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<(3-i);_z++)temp->now_state[1][i*16+i*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+(3-i)*4+i]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<i;_z++)temp->now_state[1][i*16+(3-i)*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  {
    shared_ptr<state> temp{new state};
    for(int i=0;i<4;i++)temp->now_state[0][i*16+(3-i)*4+(3-i)]=1;
    for(int i=0;i<4;i++){
      for(int _z=0;_z<(3-i);_z++)temp->now_state[1][i*16+(3-i)*4+_z]=1;
    }
    temp->count=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  
  return result;
}

//ビンゴ状態を作る
vector<shared_ptr<state> > make_debug() noexcept{
  vector<shared_ptr<state> > result{};
  //左右 16
  for(int y=0;y<1;y++){
    for(int z=2;z<3;z++){
      shared_ptr<state> temp{new state};
      for(int _x=0;_x<4;_x++)temp->now_state[0][y*16+_x*4+z]=1;
      for(int _z=0;_z<z;_z++){
	for(int _x=0;_x<4;_x++)temp->now_state[1][y*16+_x*4+_z]=1;
      }
      temp->count=0;
      temp->type=tobingos2;
      temp->genpre();
      result.push_back(move(temp));
    }
  }
  //前後16
  for(int x=0;x<1;x++){
    for(int z=3;z<4;z++){
      shared_ptr<state> temp{new state};
      for(int _y=0;_y<4;_y++)temp->now_state[0][_y*16+x*4+z]=1;
      for(int _z=0;_z<z;_z++){
	for(int _y=0;_y<4;_y++)temp->now_state[1][_y*16+x*4+_z]=1;
      }
      temp->count=0;
      temp->type=tobingos2;
      temp->genpre();
      result.push_back(move(temp));
    }
  }
  
   return result;
}


bool victory(vector<bitset<64> >& board) noexcept{
  static const auto bingos=make_bingo();
  for(const auto& bingo:bingos){
    if((board[0]&bingo->now_state[0]).count()==4)return true;
    if((board[1]&bingo->now_state[0]).count()==4)return true;
  }
  return false;
}

//対戦相手からみたボードを作る
vector<bitset<64> > make_opponent_board(const vector<bitset<64> >& board) noexcept{
  vector<bitset<64> > result=board;
  swap(result[0],result[1]);
  return result;
}

//ボードの中で物理的に置ける場所を返す
bitset<64> canplace(const vector<bitset<64> >& board) noexcept{
  bitset<64> result;
  for(int x=0;x<16;x++){
    for(int z=0;z<4;z++){
      if(board[0][x*4+z]||board[1][x*4+z])continue;
      else{
	result[x*4+z]=1;
	break;
      }
    }
  }
  return result;
}

//myactionの中から一つに絞る
bitset<64> random1(const bitset<64>& target) noexcept{
  static Rand_int rand(0,63);
  bitset<64> _target{};
  if(!target.any()){
    cout<<"target is empty"<<endl;
    return _target;
  }else{
    while(true){
      int num=rand();
      if(target[num]){
	_target[num]=1;
	break;
      }
    }
    return _target;
  }
}

//parentのpreから一手前のstateを作る
vector<shared_ptr<state> > make_child(const shared_ptr<state>& parent) noexcept{
  vector<shared_ptr<state> > result{};
  for(auto&pre:parent->pre){
    if(!pre.unchange){
      //preそのまま
      shared_ptr<state> temp{new state};
      temp->now_state=pre.board;
      if(parent->type==tos3x1){
	temp->type=tos3x1;
	temp->s3x1spot=parent->s3x1spot;
      }
      else if(parent->type==tobingos2)temp->type=tobingow1;
      else temp->type=tobingon;
      //要求を満たせなくする相手のアクションとは、空要求の場所を(次に自分が置きたい場所を含む)を埋めることである。埋めることができる場所は、一番下のempty
      _next next{};
      for(int x=0;x<16;x++){
	for(int z=0;z<4;z++){
	  if(temp->now_state[2][x*4+z]){
	    next.opact[x*4+z]=1;
	    break;
	  }
	}
      }
      next.nextstate=parent;
      temp->next.push_back(move(next));
      temp->count=parent->count+1;
      temp->genpre(true);
      result.push_back(move(temp));
    }
    //上のAを取り除く
    for(int x=0;x<16;x++){
      for(int z=3;z>-1;z--){
	if(pre.board[0][x*4+z])break;
	if(pre.board[1][x*4+z]){	    
	  shared_ptr<state> temp{new state};
	  temp->now_state = pre.board;
	  temp->now_state[1][x*4+z]=0;
	  temp->now_state[2][x*4+z]=1;//前提条件より、上は全てemptyのはずなので処理しない
	  if(parent->type==tos3x1){
	    temp->type=tos3x1;
	    temp->s3x1spot=parent->s3x1spot;
	  }else if((parent->type&tobingos2).any()&&(pre.strongA[x*4+z]==1)){
	    temp->type=tobingos1;
	    if(z==1)temp->s3x1spot[x]=1;
	  }else{
	    temp->type=tobingon;
	  }
	  _next next{};
	  //取り除いた土台の場所以外の場所に置くと妨害できる
	  //don't careの場所、または、取り除いた土台以外の場所の一番下のempty
	  next.opact.set();
	  for(auto& req:pre.board) next.opact&=~req;//don't careを抽出
	  //一番下のemptyを抽出
	  for(int _x=0;_x<16;_x++){
	    for(int _z=0;_z<4;_z++){
	      if(_x==x)break;//取り除いた土台の場所以外
	      if(pre.board[2][_x*4+_z]){
		next.opact[_x*4+_z]=1;
		break;
	      }
	    }
	  }
	  temp->next.push_back(move(next));
	  temp->count=parent->count+1;
	  //if(parent->man&&(z<3)&&(pre.second[x*4+z+1]))temp->man=1;
	  temp->genpre();
	  result.push_back(move(temp));
	  break;
	}
      }
    }	
  }
  return result;
}

//そのpre盤面に至るまでの最低ターン数
int countpre_x2(const vector<bitset<64> >& board,bool me,const vector<bitset<64> >& preboard)noexcept{
  //ボールの場所に関する拘束
  if((preboard[0]&board[1]).any()||(preboard[2]&(board[0]|board[1])).any()){//決して作れない
    return -1;
  }else{
    int Mnum=preboard[0].count()-(board[0]&preboard[0]).count();
    int Anum=preboard[0].count()+preboard[1].count()-((board[0]|board[1])&(preboard[0]|preboard[1])).count();
    if(me){
      return max(Mnum,Anum/2+Anum%2);
    }else{//対戦相手の場合、anyballが一つ増えても良い
      return max(Mnum,(max(Anum-1,0)/2+max(Anum-1,0)%2));
    }
  }
}

//now_stateを満たしているnewx2を削除
//me: boardがpreを満たしているnew_x2はresult[0][0]/result[1][0]へ
//me: boardから今後preが作れないnew_x2は削除or result[0][N]/result[0][N]へ
//op: boardから今後preが作れないnew_x2は削除
//result.size()で分岐?
//削除されたらnullptr
void check_x2pre(const vector<bitset<64> >& board,int loopnum,bool me,shared_ptr<state>& x2,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16> s3x1spot)noexcept{
  if(((~board[0])&x2->now_state[0]).any()||((~board[0])&(~board[1])&(x2->now_state[1])).any()||((board[0]|board[1])&(x2->now_state[2])).any()){//now_stateを満たしていない
    if((x2->type&tos3x1).any()&&(s3x1spot&x2->s3x1spot).any()){//すでに完成しているs3x1を作っても意味はないので削除
      x2=nullptr;
      return;
    }
    if(me){//自分の場合。削除した時に記録する必要がある場合がある
      vector<_pre> newpre{};
      for(auto& pre:x2->pre){
	int precount=countpre_x2(board,me,pre.board);
	if(precount<0){//決して作れない
	  continue;
	}else if(precount!=0&&precount<=loopnum){//読む手数内で作れる
	  newpre.push_back(move(pre));
	  continue;
	}else{//読む手数内で作れないor もうpreを満たす
	  if(precount<result[0].size()){//記録する手数内
	    shared_ptr<state> temp{new state{*x2}};
	    temp->pre.clear();
	    temp->pre.push_back(move(pre));
	    if((temp->type&tobingo).any())result[0][precount].push_back(move(temp));
	    else result[1][precount].push_back(move(temp));
	    continue;
	  }else{//記録する手数外
	    continue;
	  }
	}
      }
      if(!newpre.empty()){//残す
	x2->pre=move(newpre);
	return;
      }else{//削除
	x2=nullptr;
	return;
      }
    }else{//相手の場合
      vector<_pre> newpre{};
      for(auto& pre:x2->pre){
	int precount=countpre_x2(board,me,pre.board);
	if(precount<0){//決して作れない
	  continue;
	}else if(precount<=loopnum){//読む手数内で作れる
	  newpre.push_back(move(pre));
	  continue;
	}else{//読む手数内で作れない
	  continue;
	}
      }
      if(!newpre.empty()){//残す
	x2->pre=move(newpre);
	return;
      }else{//削除
	x2=nullptr;
	return;
      }
    }
  }else{//now_stateを満たしている
    if((x2->type&=tos3x1).any()){//完成しているs3x1地点リストに追加
      s3x1spot|=x2->s3x1spot;
    }
    x2=nullptr;//削除
    return;
  }
}

/*x1について、
  s3x1spotがかぶっているものは削除
  me: 今後全てのpreが作れないものは削除
  op: now_stateを満たしているものはresult[0][0]/result[0][0]へ
  op: 今後全てのpreが作れないものは削除or result[0][N]/result[1][N]へ
  削除されたらnullptr
*/
void remove_if_x1(const vector<bitset<64> >& board,int loopnum,bool me,shared_ptr<state>& x1,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16>& s3x1spot)noexcept{
   if((x1->type&tos3x1).any()&&(s3x1spot&x1->s3x1spot).any()){//すでに完成しているs3x1を作っても意味はないので削除
     x1=nullptr;
     return;
   }else if(me){//preが作れないものを削除
     vector<_pre> newpre{};
      for(auto& pre:x1->pre){
	int precount=countpre_x2(board,me,pre.board);
	if(precount<0){//決して作れない
	  continue;
	}else if(precount<=loopnum){//読む手数内で作れる
	  newpre.push_back(move(pre));
	  continue;
	}else{//読む手数内で作れない
	  continue;
	}
      }
      if(!newpre.empty()){//残す
	x1->pre=move(newpre);
	return;
      }else{//削除
	x1=nullptr;
	return;
      }
   }else{//opponentの場合
     if(((~board[0])&x1->now_state[0]).any()||((~board[0])&(~board[1])&(x1->now_state[1])).any()||((board[0]|board[1])&(x1->now_state[2])).any()){//now_stateを満たしていない
       vector<_pre> newpre{};
       for(auto& pre:x1->pre){
	 int precount=countpre_x2(board,me,pre.board);
	 if(precount<0){//決して作れない
	   continue;
	 }else if(precount<=loopnum){//読む手数内で作れる
	   newpre.push_back(move(pre));
	   continue;
	 }else{//読む手数内で作れないor もうpreを満たす
	   if(precount<result[0].size()-1){//記録する手数内
	     shared_ptr<state> temp{new state{*x1}};
	     temp->pre.clear();
	     temp->pre.push_back(move(pre));
	     if((temp->type&tobingo).any())result[0][precount+1].push_back(move(temp));
	     else result[1][precount+1].push_back(move(temp));
	     continue;
	   }else{//記録する手数外
	     continue;
	   }
	 }
       }
       if(!newpre.empty()){//残す
	 x1->pre=move(newpre);
	 return;
       }else{//削除
	 x1=nullptr;
	 return;
       }
     }else{//now_stateを満たしている
       if((x1->type&tobingo).any())result[0][0].push_back(move(x1));
       else result[1][0].push_back(move(x1));
       x1=nullptr;//削除
       return;
     }
   }
}

inline shared_ptr<state> merge(const shared_ptr<state>& state1,const shared_ptr<state>& state2) noexcept{
  //２つのx1stateがダブルリーチ可能ならmerge、そうでないならnullptr
  //要求を同時に満たすことが可能で、その時の自分の行動を共有し、相手が止める行動が重複しなければよい
  //x1は、nextは1つずつしかない(前提)
  if((state1->next[0].opact&state2->next[0].opact).any())return nullptr;//相手が止める行動が重複
  shared_ptr<state> result=nullptr; 
  for(auto& pre1: state1->pre){
    for(auto& pre2: state2->pre){
      if((pre1.myact&pre2.myact).none())continue;//自分の行動を共有しない
      if((((pre1.board[0]|pre1.board[1])&pre2.board[2])|((pre2.board[0]|pre2.board[1])&pre1.board[2])).any())continue;//要求を同時に満たせない
      if(result==nullptr){
	result=make_shared<state>();
	result->now_state[0]=state1->now_state[0]|state2->now_state[0];
	result->now_state[1]=(state1->now_state[1]&(~state2->now_state[0]))|(state2->now_state[1]&(~state1->now_state[0]));
	result->now_state[2]=state1->now_state[2]|state2->now_state[2];
	result->next.push_back(state1->next[0]);
	result->next.push_back(state2->next[0]);
	if((state1->type&tos3x1).any()||(state2->type&tos3x1).any()){
	  result->type=tos3x1;
	  result->s3x1spot=state1->s3x1spot|state2->s3x1spot;
	  if((state1->type&tos3x1).any()&&(state2->type&tos3x1).any())result->count=max(state1->count,state2->count);
	  else if((state1->type&tos3x1).any()) result->count=state1->count;
	  else if((state2->type&tos3x1).any()) result->count=state2->count;
	}else{
	  result->count=max(state1->count,state2->count);
	  if((pre1.ssx1&&pre2.swx1)||(pre2.ssx1&&pre1.swx1)) result->type=tobingos2;
	  else result->type=tobingon;
	}
      }
      _pre temppre{};
      temppre.board[0]=pre1.board[0]|pre2.board[0];
      temppre.board[1]=(pre1.board[1]&(~pre2.board[0]))|(pre2.board[1]&(~pre1.board[0]));
      temppre.board[2]=pre1.board[2]|pre2.board[2];
      temppre.myact=pre1.myact&pre2.myact;
      temppre.strongA=pre1.strongA|pre2.strongA;
      result->pre.push_back(temppre);
    }
  }
  return result;
}

//x1がs3x1ならコピーした上で加える
void levelup_if(shared_ptr<state>& x1,vector<shared_ptr<state> >& new_new_x2)noexcept{
  if(x1->type==tobingos1&&x1->s3x1spot.any()){
    shared_ptr<state> temp{new state{*x1}};
    temp->type=tos3x1;
    new_new_x2.push_back(move(temp));
  }
  return;
}

bool state_sort_func(const shared_ptr<state> state1,const shared_ptr<state> state2)noexcept{
  //now_stateによってソート
  if(state1->now_state[0].to_ullong()<state2->now_state[0].to_ullong())return true;
  else if(state1->now_state[0]==state2->now_state[0]){
    if(state1->now_state[1].to_ullong()<state2->now_state[1].to_ullong())return true;
    else if(state1->now_state[1]==state2->now_state[1]){
      if(state1->now_state[2].to_ullong()<state2->now_state[2].to_ullong())return true;
      else if(state1->now_state[2].to_ullong()==state2->now_state[2].to_ullong()){
	//typeによってソート
	if(state1->type.to_ulong() < state2->type.to_ulong())return true;
      }
    }
  }
  return false;
}

//x2の重複をまとめる
//s3x1はうっかりまとめてはいけない
void unique(vector<shared_ptr<state> >& new_x2){
  if(new_x2.empty())return;
  vector<shared_ptr<state> > result{};
  sort(new_x2.begin(),new_x2.end(),state_sort_func);
  for(auto _state=new_x2.begin();_state!=new_x2.end();_state++){
    auto _state2=_state;
    _state2++;
    if(_state2==new_x2.end())break;
    if(((*_state)->now_state[0]==(*_state2)->now_state[0])&&((*_state)->now_state[1]==(*_state2)->now_state[1])&&((*_state)->now_state[2]==(*_state2)->now_state[2])/*&&!((*_state)->type&tos3x1).any()&&!((*_state2)->type&tos3x1).any()*/&&((*_state)->type==(*_state2)->type)){//同じでかつ、タイプが同じなら、次のやつにまとめる
      if((*_state)->type==tos3x1&&(*_state)->s3x1spot!=(*_state2)->s3x1spot){
	result.push_back(move(*_state));//s3x1は、ポイントが全く同じでないと行けない
	continue;
      }
      for(auto& pre1:(*_state)->pre){
	bool same=false;
	for(auto& pre2:(*_state2)->pre){
	  if(pre1.myact==pre2.myact){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->pre.push_back(move(pre1));
      }
      for(auto& next1:(*_state)->next){
	bool same=false;
	for(auto& next2:(*_state2)->next){
	  if(next1.nextstate==next2.nextstate){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->next.push_back(move(next1));
      }
      (*_state2)->count=min((*_state)->count,(*_state2)->count);
      //(*_state2)->type放置で問題ない
      (*_state)=nullptr;
    }
    //同じでなければ、resultへ
    else result.push_back(move(*_state));
  }
  result.push_back(move(*(--new_x2.end())));
  //x2.remove(nullptr);
  new_x2=move(result);
  return;
}

//_x1を削除する
//op: boardからopは削除or result[0][N]/result[1][N]へ
void del_x1(const vector<bitset<64> >& board,bool me,vector<shared_ptr<state> >& _x1,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16> s3x1spot) noexcept{
  if(me){//何もしない
    _x1.clear();
    return;
  }else{//preの現在からのターン数、now_stateを満たしているかによって分類
    for(auto& x1:_x1){
      if(((~board[0])&x1->now_state[0]).any()||((~board[0])&(~board[1])&(x1->now_state[1])).any()||((board[0]|board[1])&(x1->now_state[2])).any()){//now_stateを満たしていない
	for(auto& pre:x1->pre){
	  int precount=countpre_x2(board,me,pre.board);
	  if(precount<0){//決して作れない
	    continue;
	  }else{//読む手数内で作れないor もうpreを満たす
	    if(precount<result[0].size()-1){//記録する手数内
	      shared_ptr<state> temp{new state{*x1}};
	      temp->pre.clear();
	      temp->pre.push_back(move(pre));
	      if((temp->type&tobingo).any())result[0][precount+1].push_back(move(temp));
	      else result[1][precount+1].push_back(move(temp));
	      continue;
	    }else{//記録する手数外
	      continue;
	    }
	  }
	}
      }else{//now_stateを満たしている
	if((x1->type&tobingo).any())result[0][0].push_back(move(x1));
	else result[1][0].push_back(move(x1));
      }
    }
    _x1.clear();
    return;
  }
}


//new_x2を削除する
//me: boardからnew_x2は削除or result[0][N]/result[1][N]へ
void del_x2(const vector<bitset<64> >& board,bool me,vector<shared_ptr<state> >& new_x2,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16> s3x1spot) noexcept{
  if(me){//preの現在からのターン数によって分類
    for(auto& x2:new_x2){
      if(((~board[0])&x2->now_state[0]).any()||((~board[0])&(~board[1])&(x2->now_state[1])).any()||((board[0]|board[1])&(x2->now_state[2])).any()){//now_stateを満たしていない
	if((x2->type&tos3x1).any()&&(s3x1spot&x2->s3x1spot).any()){//すでに完成しているs3x1を作っても意味はないので記録しない
	  x2=nullptr;
	}
	vector<_pre> newpre{};
	for(auto& pre:x2->pre){
	  int precount=countpre_x2(board,me,pre.board);
	  if(precount<0){//決して作れない
	    continue;
	  }else{
	    if(precount<result[0].size()){//記録する手数内
	      shared_ptr<state> temp{new state{*x2}};
	      temp->pre.clear();
	      temp->pre.push_back(move(pre));
	      if((temp->type&tobingo).any())result[0][precount].push_back(move(temp));
	      else result[1][precount].push_back(move(temp));
	      continue;
	    }else{//記録する手数外
	      continue;
	    }
	  }
	}
      }else{//now_stateを満たしている
	if((x2->type&=tos3x1).any()){//完成しているs3x1地点リストに追加
	  s3x1spot|=x2->s3x1spot;
	}
	x2=nullptr;//削除
      }
    }
    new_x2.clear();
  }else{//何もしない
    new_x2.clear();
  }
}

//nullptrを除く関数
void removenullptr(vector<shared_ptr<state> >& xx) noexcept{
  vector<shared_ptr<state> > result{};
  for(auto& x:xx){
    if(x!=nullptr){
      result.push_back(move(x));
    }
  }
  xx=move(result);
  return ;
}

//tobingoを計算する
vector<vector<vector<shared_ptr<state> > > > bingoloop(const vector<bitset<64> >& board,int loopnum_max,bool me) noexcept{
}

//tobingo,tos3x1を計算する
vector<vector<vector<shared_ptr<state> > > > s3x1loop(const vector<bitset<64> >& board,int loopnum_max,bool me) noexcept{
  vector<vector<vector<shared_ptr<state> > > > result(2,vector<vector<shared_ptr<state> > >(loopnum_max+1));
  /*
    result[0][N]: Nターン後に完成するto_bingoの(me)?x2:x1を保管する
    result[1][N]: Nターン後に完成するto_s3x1の(me)?x2:x1を保管する
   */
  //vector<shared_ptr<state> > new_x2{make_debug()};//これをx1に展開していく
  vector<shared_ptr<state> > new_x2{make_bingo()};//これをx1に展開していく
  vector<shared_ptr<state> > new_new_x2{};//x1からmergeし作られたものを一時的に入れる
  vector<shared_ptr<state> > _x1{};//new_x2から展開されたものをどんどんためていく
  bitset<16> s3x1spot{};//すでに完成しているs3x1spot
  
  if(!me)loopnum_max--;//対戦相手の場合は読む手の数は1少なくて良い

  //new_x2重複をまとめる<-ここでは不要
  for(auto& x2:new_x2){
    /*最初の処理*/
    //now_stateを満たしているnewx2を削除
    //me: boardがpreを満たしているnew_x2はresult[0][0]/result[1][0]へ
    //me: boardから今後preが作れないnew_x2は削除or result[0][N]/result[1][N]へ
    //op: boardから今後preが作れないnew_x2は削除
    check_x2pre(board,loopnum_max,me,x2,result,s3x1spot);
  }
  //remove(new_x2.begin(),new_x2.end(),nullptr);
  removenullptr(new_x2);
  cout<<"bingo: "<<new_x2.size()<<endl;

  for(int loopnum=loopnum_max-1;loopnum>=0;loopnum--){
    for(auto& x1:_x1){
      /*x1について、
	s3x1spotがかぶっているものは削除
	me: 今後全てのpreが作れないものは削除
	op: now_stateを満たしているものはresult[0][0]/result[0][0]へ
	op: 今後全てのpreが作れないものは削除or result[0][N]/result[1][N]へ
      */
      remove_if_x1(board,loopnum,me,x1,result,s3x1spot);
    }
    removenullptr(_x1);
    /*
    if(loopnum==2){
      for(auto& x1:_x1){
	cout<<endl<<x1->now_state;
	for(auto& pre:x1->pre)cout<<pre;
      }
    }
    */
    for(auto& x2:new_x2){
      //if(loopnum==2)cout<<endl<<endl<<x2->now_state;
      //cout<<x2->type<<endl;
      auto children=make_child(x2);//x1を生成
      for(auto& child :children){
       	remove_if_x1(board,loopnum,me,child,result,s3x1spot);
	if(child==nullptr)continue;//現在の盤から至れない
	
	//cout<<child->now_state;
	//cout<<child->type<<endl;
	//cout<<child->s3x1spot<<endl<<endl;
	//for(auto& pre:child->pre)cout<<pre<<endl;
	//for(auto& next:child->next)cout<<next.opact<<endl;  
	
	//x1のリスト内から、統合できるものを探す。ダブルリーチを製造.
	for(auto& target:_x1){
	  shared_ptr<state> result=merge(child,target);
	  if(result==nullptr)continue;//統合できない
	  /*
	  if(loopnum==2){
	    cout<<"merge"<<endl;
	    cout<<child->now_state;
	    cout<<child->next[0].opact;
	    cout<<target->now_state;
	    cout<<target->next[0].opact;
	    cout<<result->now_state;
	  }
	  */
	  new_new_x2.push_back(move(result));
	}
	//s3x1なら、x2に昇格
	levelup_if(child,new_new_x2);
	_x1.push_back(move(child));
      }
    }
    //x1について重複をまとめる?

    //new_new_x2について
    //new_new_x2重複をまとめる
    unique(new_new_x2);
    for(auto& x2:new_new_x2){
      check_x2pre(board,loopnum,me,x2,result,s3x1spot);
    }
    removenullptr(new_new_x2);
    //new_x2を削除する
    //me: boardからnew_x2は削除or result[0][N]/result[1][N]へ
    del_x2(board,me,new_x2,result,s3x1spot);
    new_x2=move(new_new_x2);
    new_new_x2.clear();
    cout<<loopnum<<"\t"<<new_x2.size()<<"\t"<<_x1.size()<<endl;

    /*
    if(loopnum==3){
      for(auto& x2:new_x2){
	cout<<endl<<x2->now_state;
	for(auto& next:x2->next)cout<<next.opact;
      }
    }
    */
  }

  if(!me){
    //最後にもう一回展開
    for(auto& x2:new_x2){
      auto children=make_child(x2);
      for(auto& child:children){
	_x1.push_back(move(child));
      }
    }
  }
  del_x1(board,me,_x1,result,s3x1spot);
  del_x2(board,me,new_x2,result,s3x1spot);

  //ここでresultの重複解除や、s3x1の削除を行いたい
  return result;
}

bitset<64> think(const vector<bitset<64> >& board){
  if(!(canplace(board)).any()){
    cout<<"connot place"<<endl;
    return bitset<64>{};//置ける場所が無い
  }
  vector<bitset<64> > mystart=board;
  vector<bitset<64> > opstart=make_opponent_board(mystart);
  
  int s3x1loopnum_max=6;
  vector<vector<vector<shared_ptr<state> > > > mys3x1_x2=s3x1loop(mystart,s3x1loopnum_max,true);
  vector<vector<vector<shared_ptr<state> > > > ops3x1_x1=s3x1loop(opstart,s3x1loopnum_max,false);

  cout<<mys3x1_x2[0][0].size()<<endl;
  for(auto& x:mys3x1_x2[0][0]){
    cout<<x->now_state<<endl;
  }

  cout<<ops3x1_x1[0][0].size()<<endl;
  for(auto& x:ops3x1_x1[0][0]){
    cout<<x->now_state<<endl;
  }
  cout<<mys3x1_x2[1][0].size()<<endl;
  for(auto& x:mys3x1_x2[1][0]){
    cout<<x->now_state<<endl;
  }

  cout<<ops3x1_x1[1][0].size()<<endl;
  for(auto& x:ops3x1_x1[1][0]){
    cout<<x->now_state<<endl;
  }
 
  
  bitset<64> myaction{};
  //第一タスク。ダブルリーチの連鎖で勝つor負けない
  for(auto& x2:mys3x1_x2[0][0]){//完成しているtobingoのx2について、勝てるか見る
    for(auto& pre:x2->pre){
      bitset<64> tempmyaction=pre.myact;
      for(auto& x1:ops3x1_x1[0][0]){//放置すると完成する相手のx1で、countが自分よりも早いもの
	if(x1->count<=x2->count){
	  for(auto& next:x1->next) tempmyaction&=next.opact;
	}
      }
      myaction|=tempmyaction;
    }
  }
  if(!myaction.any()){//すぐに勝てないなら、相手の妨害に専念
    myaction=canplace(board);
    for(auto& x1:ops3x1_x1[0][0]){
      for(auto& next:x1->next) myaction&=next.opact;
    }
    cout<<"not attack my bingo"<<endl;
  }else{
    cout<<"attack my bingo"<<endl;
  }
  if(!myaction.any()){
    myaction=canplace(board);
    cout<<"connot block op's bingo"<<endl;
    /*
    //相手の妨害ができないということ.悪あがきするしか無い
    myaction=canplace(board);
    vector<int> myaction_p(64,0);
    for(int i=0;i<64;i++){
    if(myaction[i]==0){
    myaction_p=-1;
    continue;
    }
    bitset<64> tempaction{};
    tempaction[i]=1;
      for(auto& x1:ops3x1_x1[0][0]){
	for(auto& next:x1->next) {
	  if((tempaction&next.opact).any)myaction_p[i]++;
	}
      }
      for(auto& x2:mys3x1_x2[0][0]){
	for(auto& pre:=x2->pre){
	  if((tempaction&pre.myact).any())myaction_p[i]++;
	}
      }
    }
    auto maxp=max_element(myaction_p.begin(),myaction_p.end());
    myaction.reset();
    for(int i=0;i<64;i++){
      if(myaction_p[i]==*maxp)myaction[i]=1;
    }
    */
  }
  cout<<myaction<<endl;
  //これらの第一タスクを満たす範囲内で、第2タスクを行う
  bitset<64> t1_myaction=myaction;
  //第2タスク。自分のs3x1を相手より増やす
  //ある場所に置くことによる相手のs3x1の増加より自分のs3x1の増加が多ければ良いが、道中で話が変わることも多々あるので、とりあえずまず相手のs3x1を止めて、次に自分のs3x1の完成を目指す
  for(auto& x1:ops3x1_x1[1][0]){
    for(auto& next:x1->next) t1_myaction&=next.opact;
  }
  if(t1_myaction.any()){//止められる.この範囲で自分のs3x1の実現を目指す
    bitset<64> t1_2myaction{};
    for(auto& x2:mys3x1_x2[1][0]){
      for(auto& pre:x2->pre) t1_2myaction|=(pre.myact&t1_myaction);
    }
    if(t1_2myaction.any()){//これが第２タスクの結果
      myaction=t1_2myaction;
      cout<<"attack my s3x1"<<endl;
    }else{//自分のs3x1は実現しないのであきらめて相手の妨害のみに専念
      myaction=t1_myaction;
      cout<<"not attack my s3x1"<<endl;
    }
  }else{//止められない
    bitset<64> t1_2myaction{};
    for(auto& x2:mys3x1_x2[1][0]){
      for(auto& pre:x2->pre) t1_2myaction|=(pre.myact&myaction);
    }
    if(t1_2myaction.any()){
      myaction=t1_2myaction;
      cout<<"connot block op's s3x1 attack my s3x1"<<endl;
    }else{
      myaction=myaction;
      cout<<"connot block op's s3x1"<<endl;
    }
  }
  cout<<myaction<<endl;
  //第３タスクがあるならココに
  //自分のs3x1を埋めないとか
  //最後はランダム
  if(myaction.count()!=1){
    if(myaction.count()==0)myaction=canplace(board);
    myaction=random1(myaction);
  }
  return myaction;
}

int main(int argc ,char** argv){
  vector<bitset<64> > board(2);
  ///*board[0][2]=1;*/board[0][5]=1;board[0][6]=1;board[0][8]=1;//board[0][10]=1;
  //board[1][0]=1;board[1][1]=1;board[1][4]=1;board[1][9]=1;//board[1][12]=1;

  //board[0][17]=1;board[0][19]=1;board[0][32]=1;board[0][35]=1;board[0][48]=1;board[0][51]=1;
  //board[1][16]=1;board[1][18]=1;board[1][33]=1;board[1][34]=1;board[1][49]=1;board[1][50]=1;
  //board[0][0]=1;board[0][1]=1;board[0][12]=1;board[0][13]=1;
  //board[1][16]=1;board[1][17]=1;board[1][60]=1;board[1][61]=1;
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
