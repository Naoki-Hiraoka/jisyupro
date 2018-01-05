//g++ -o think_develop6 think_develop6.cpp -std=c++14 -lpthread
/*
  仕組み
  *終了状態からの逆算を探索する。
  　現在の盤面からN手先までボトムアップ的に探索すると、計算量は16のN乗となる。N=13なら、計算量は10の17乗近く。ギガヘルツ(10の9乗)で計算しても、時間がかかりすぎる。
  　ニューラルネットワークは、論理的な積み重ねの思考が要求されるこのゲームで、うまく動くものが作れる自信がない。誤差を最小にする過程で、単純な相手のリーチを止めると言った行動が誤差として残ってしまうことを防げる自信がない。
　　そこで、76通り存在するビンゴ状態を終了状態とし、そこから逆算してダブルリーチなど勝ちの盤面を生成する。現在の盤面がそれらの勝ちの盤面のどれかに該当するなら、その手をさせば良い。また、相手がそれらの勝ちの盤面に至りそうなら、それをとめればよい。現在の盤面からN手以内に至れないビンゴ状態等を計算から除くことで、計算量はそこまで大きくならず、実験的にはN=13の時に計算量は10の8乗程度となる。実際には、ヒトが我慢して待てる範囲内の時間で9~15手読める。

　*欠点
　　逆算によって考えることの弊害。現在の盤面が「勝ちだ」と思っても、実際に交互にコマを置いていくと途中で相手のリーチが完成したりしてその対応に追われ勝てないことがある。これを改善するためには、現在の盤面が「勝ちだ」と思ったときに、ボトムアップ的にもう一度計算してやる必要がある。普通にボトムアップ的に計算する時と比べ、自分の勝利までの手が決まっている分計算量は小さくなるはずではある。
　　計算量は小さいが、逆算には論理的にいろいろな処理が必要で、単純なボトムアップと比べて処理が複雑になる。このプログラム内にも一部、論理的に完璧でない部分があることを自覚しているが、どこが完璧でないのか自分でも分からなくなってしまった。

　*タスク
　　相手が勝ちの盤面に至るのを防ぐ、自分が勝ちの盤面なら勝ちに突き進む。という行動を徹底する上で、残ったタスク空間で行うこと。
　　上記を徹底すれば、読む手数N以上の見事なコンボを相手に打たれるケースを除けば、負けることはない。すると、ゲームは終盤の置ける場所がないから仕方なく置くという状況にもつれ込む。このとき、高さ2の場所に相手にとって置けない場所(置くと自分が勝つ)が多くあるプレイヤーが勝つ。そのため、このような場所を作ること、作らせないことが第２タスクとなる。
　　第３タスク以降は、読む手数N以上のコンボを防ぐorいつの間にか自分が作ることを目標に、製作者のセンスで設定した。<-ここには、機械学習の入る余地がありそう
 */

#include <iostream>
#include <bitset> //bitset
#include <vector> //vector
#include <memory> //shared_ptr
#include <algorithm>
#include<random>
#include<thread>
#include<future>

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
  bitset<64> badpoint{};//myactで置く予定のAの場所。相手が置いてくれたらむしろ嬉しい
  bool ssx1=false;//sのx1のmerge可能性のあるpre
  bool swx1=false;//swのx1のmerge1可能性のあるpre
  bool unchange=false;//for make_child
};
_pre::_pre(): board(3){
}

class state;

class loopresult{
public:
  vector<vector<vector<shared_ptr<state> > > > result{};
  bitset<16> s3x1spot{};
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
  vector<bitset<64> > next{};//次に至るのを対戦相手が妨害できる行動(実際に置けるとは限らない)
  int count=0;//ビンゴまでの手数(最大)
  int mincount=0;//ビンゴまでの手数(最小)
  bitset<8> type{};
  bitset<16> s3x1spot{};//tos3x1の、完成した場合の拘束スポット
  void genpre(bool) noexcept;//now_stateからpreを自動生成する
};
state::state(): now_state(3) {
}

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
	temppre.badpoint[x*4+z]=1;
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
    temp->mincount=0;
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
      temp->mincount=0;
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
      temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
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
    temp->mincount=0;
    temp->type=tobingos2;
    temp->genpre();
    result.push_back(move(temp));
  }
  
  return result;
}

//デバッグ用ビンゴ状態を作る
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
      temp->mincount=0;
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
      temp->mincount=0;
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
  static Rand_int rand(0,63/*,static_cast<int>(ros::Time::now().sec)*/);
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
inline vector<shared_ptr<state> > make_child(const shared_ptr<state>& parent) noexcept{
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
      bitset<64> next{};
      for(int x=0;x<16;x++){
	for(int z=0;z<4;z++){
	  if(temp->now_state[2][x*4+z]&&pre.badpoint[x*4+z]==0){
	    next[x*4+z]=1;
	    break;
	  }
	}
      }
      if(next.any()){
	//next.nextstate=parent;
	temp->next.push_back(move(next));
	temp->count=parent->count+1;
	temp->mincount=parent->mincount+1;
	temp->genpre(true);
	result.push_back(move(temp));
      }
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
	  bitset<64> next{};
	  //取り除いた土台の場所以外の場所に置くと妨害できる
	  //don't careの場所、または、取り除いた土台以外の場所の一番下のempty
	  next.set();
	  for(auto& req:pre.board) next&=~req;//don't careを抽出
	  //一番下のemptyを抽出
	  for(int _x=0;_x<16;_x++){
	    for(int _z=0;_z<4;_z++){
	      if(_x==x)break;//取り除いた土台の場所以外
	      if(pre.board[2][_x*4+_z]){
		next[_x*4+_z]=1;
		break;
	      }
	    }
	  }
	  temp->next.push_back(move(next));
	  temp->count=parent->count+1;
	  temp->mincount=parent->mincount+1;
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
inline int countpre_x2(const vector<bitset<64> >& board,bool me,const vector<bitset<64> >& preboard)noexcept{
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

//そのpre盤面にこの1行動で至るためのactionを返す
bitset<64> gopre_x2(const vector<bitset<64> >& board,const vector<bitset<64> >& preboard)noexcept{
  //ボールの場所に関する拘束
  if((preboard[0]&board[1]).any()||(preboard[2]&(board[0]|board[1])).any()){//決して作れない
    return bitset<64>{};
  }else{
    int Mnum=preboard[0].count()-(board[0]&preboard[0]).count();
    int Anum=preboard[0].count()+preboard[1].count()-((board[0]|board[1])&(preboard[0]|preboard[1])).count();
    if(Mnum>1||Anum>1)return bitset<64>{};

    return (preboard[0]|preboard[1])&~(board[0]|board[1]);
  }
}


//now_stateを満たしているnewx2を削除
//me: boardがpreを満たしているnew_x2はresult[0][0]/result[1][0]へ
//me: boardから今後preが作れないnew_x2は削除or result[0][N]/result[0][N]へ
//op: boardから今後preが作れないnew_x2は削除
//result.size()で分岐?
//削除されたらnullptr
void check_x2pre(const vector<bitset<64> >& board,int loopnum,bool me,shared_ptr<state>& x2,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16>& s3x1spot)noexcept{
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
inline void remove_if_x1(const vector<bitset<64> >& board,int loopnum,bool me,shared_ptr<state>& x1,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16>& s3x1spot)noexcept{
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
  if((state1->next[0]&state2->next[0]).any())return nullptr;//相手が止める行動が重複
  shared_ptr<state> result=nullptr; 
  for(const auto& pre1: state1->pre){
    for(const auto& pre2: state2->pre){
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
	  if((state1->type&tos3x1).any()&&(state2->type&tos3x1).any()){
	    result->count=max(state1->count,state2->count);
	    result->mincount=min(state1->mincount,state2->mincount);
	  }
	  else if((state1->type&tos3x1).any()) {
	    result->count=state1->count;
	    result->mincount=state1->mincount;
	  }
	  else if((state2->type&tos3x1).any()) {
	    result->count=state2->count;
	    result->mincount=state2->mincount;
	  }
	}else{
	  result->count=max(state1->count,state2->count);
	  result->mincount=min(state1->mincount,state2->mincount);
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
      temppre.badpoint=pre1.badpoint|pre2.badpoint;
      result->pre.push_back(temppre);
    }
  }
  return result;
}

//x1がs3x1ならコピーした上で加える
//genpreし直す
inline void levelup_if(shared_ptr<state>& x1,vector<shared_ptr<state> >& new_new_x2)noexcept{
  if(x1->type==tobingos1&&x1->s3x1spot.any()){
    shared_ptr<state> temp{new state{*x1}};
    temp->type=tos3x1;
    temp->count=0;
    temp->mincount=0;
    temp->genpre(true);
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
	  //if(next1.nextstate==next2.nextstate){
	  if(next1==next2){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->next.push_back(move(next1));
      }
      (*_state2)->count=min((*_state)->count,(*_state2)->count);
      (*_state2)->mincount=min((*_state)->mincount,(*_state2)->mincount);
      //(*_state2)->type放置で問題ない
      (*_state)=nullptr;
    }
    //同じでなければ、resultへ
    else result.push_back(move(*_state));
  }
  result.push_back(move(*(--new_x2.end())));
  new_x2=move(result);
  return;
}

//x1のunique用
bool state_sort_func_x1(const shared_ptr<state> state1,const shared_ptr<state> state2)noexcept{
  //now_stateによってソート
  if(state1->now_state[0].to_ullong()<state2->now_state[0].to_ullong())return true;
  else if(state1->now_state[0]==state2->now_state[0]){
    if(state1->now_state[1].to_ullong()<state2->now_state[1].to_ullong())return true;
    else if(state1->now_state[1]==state2->now_state[1]){
      if(state1->now_state[2].to_ullong()<state2->now_state[2].to_ullong())return true;
      else if(state1->now_state[2].to_ullong()==state2->now_state[2].to_ullong()){
	//typeによってソート
	if(state1->type.to_ulong() < state2->type.to_ulong())return true;
	else if(state1->type.to_ulong() == state2->type.to_ulong()){
	  //nextによってソート[0]のみと仮定
	  if(state1->next[0].to_ullong() < state2->next[0].to_ullong())return true;
	}
      }
    }
  }
  return false;
}

//x2の重複をまとめる
//s3x1はうっかりまとめてはいけない
void unique_x1(vector<shared_ptr<state> >& _x1){
  if(_x1.empty())return;
  vector<shared_ptr<state> > result{};
  sort(_x1.begin(),_x1.end(),state_sort_func_x1);
  for(auto _state=_x1.begin();_state!=_x1.end();_state++){
    auto _state2=_state;
    _state2++;
    if(_state2==_x1.end())break;
    if(((*_state)->now_state[0]==(*_state2)->now_state[0])&&((*_state)->now_state[1]==(*_state2)->now_state[1])&&((*_state)->now_state[2]==(*_state2)->now_state[2])/*&&!((*_state)->type&tos3x1).any()&&!((*_state2)->type&tos3x1).any()*/&&((*_state)->type==(*_state2)->type)&&((*_state)->next[0] == (*_state2)->next[0])){//同じでかつ、タイプが同じでかつ、nextが同じなら、次のやつにまとめる
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
      /*
      for(auto& next1:(*_state)->next){
	bool same=false;
	for(auto& next2:(*_state2)->next){
	  //if(next1.nextstate==next2.nextstate){
	  if(next1==next2){
	    same=true;
	    break;
	  }
	}
	if(!same) (*_state2)->next.push_back(move(next1));
      }
      */
      (*_state2)->count=min((*_state)->count,(*_state2)->count);
      (*_state2)->mincount=min((*_state)->mincount,(*_state2)->mincount);
      //(*_state2)->type放置で問題ない
      (*_state)=nullptr;
    }
    //同じでなければ、resultへ
    else result.push_back(move(*_state));
  }
  result.push_back(move(*(--_x1.end())));
  _x1=move(result);
  return;
}


//_x1を削除する
//op: boardからopは削除or result[0][N]/result[1][N]へ
void del_x1(const vector<bitset<64> >& board,bool me,vector<shared_ptr<state> >& _x1,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16>& s3x1spot) noexcept{
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
void del_x2(const vector<bitset<64> >& board,bool me,vector<shared_ptr<state> >& new_x2,vector<vector<vector<shared_ptr<state> > > >& result,bitset<16>& s3x1spot) noexcept{
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

//すでに完成しているs3x1spotを除く
void removebads3x1spot(vector<shared_ptr<state> >& xx,const bitset<16>& s3x1spot){
  vector<shared_ptr<state> > result{};
  for(auto& x:xx){
    if((x->type&tos3x1).any()&&(s3x1spot&x->s3x1spot).any()){//すでに完成しているs3x1を作っても意味はない
      continue;
    }else{
      result.push_back(move(x));
    }
  }
  xx=move(result);
  return;
}

//tobingo,tos3x1を計算する
loopresult s3x1loop(const vector<bitset<64> >& board,int loopnum_max,bool me,int level) noexcept{
  vector<vector<vector<shared_ptr<state> > > > result(2,vector<vector<shared_ptr<state> > >(2));
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
  removenullptr(new_x2);
  cout<<"bingo: "<<new_x2.size()<<" "<<loopnum_max<<" "<<me<<" "<<level<<endl;

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
    for(auto& x2:new_x2){
      auto children=make_child(x2);//x1を生成
      for(auto& child :children){
       	remove_if_x1(board,loopnum,me,child,result,s3x1spot);
	if(child==nullptr)continue;//現在の盤から至れない
	//x1のリスト内から、統合できるものを探す。ダブルリーチを製造.
	for(auto& target:_x1){
	  shared_ptr<state> result=merge(child,target);
	  if(result==nullptr)continue;//統合できない
	  new_new_x2.push_back(move(result));
	}
	//s3x1なら、x2に昇格
	if(level>0)levelup_if(child,new_new_x2);
	_x1.push_back(move(child));
      }
    }
    //x1について重複をまとめる
    unique_x1(_x1);

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
    if(new_x2.size()>10000){//時間がかかりすぎる
      return loopresult{};
    }
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
  //重複削除はmustではないのでs3x1のs3x1spotの削除だけ行う
  removebads3x1spot(result[1][0],s3x1spot);
  removebads3x1spot(result[1][1],s3x1spot);
  loopresult _result{};
  _result.result=move(result);
  _result.s3x1spot=move(s3x1spot);
  return _result;
}

loopresult looptask(const vector<bitset<64> >& board,int loopnum_max,bool me,int level) noexcept{
  loopresult result{};
  for(int loopnum=loopnum_max;loopnum>=0;loopnum--){
    result=s3x1loop(board,loopnum,me,level);
    if(!result.result.empty())return result;
  }
  return result;
}

bitset<64> go_bingo(const vector<bitset<64> >& board){
  static const auto bingos=make_bingo();
  bitset<64> result{};
  for(const auto& bingo:bingos){
    if((board[0]&bingo->now_state[0]).count()==3&&(board[1]&bingo->now_state[0]).count()==0){
      result|=(~board[0])&bingo->now_state[0];
    }
  }
  result&=canplace(board);
  return result;
}

vector<int> evalgoodpos(const vector<bitset<64> >& board){
  static const auto bingos=make_bingo();
  vector<int> goodpos(64,0);
  for(const auto& bingo:bingos){
    if((board[0]&bingo->now_state[0]).count()==0&&(board[1]&bingo->now_state[0]).count()==0){//何もない　1点
      for(int i=0;i<64;i++)if(bingo->now_state[0][i])goodpos[i]+=1;
    }
    if((board[0]&bingo->now_state[0]).count()==1&&(board[1]&bingo->now_state[0]).count()==0){//自分のが1つ　2点
      for(int i=0;i<64;i++)if(bingo->now_state[0][i])goodpos[i]+=2;
    }
    if((board[0]&bingo->now_state[0]).count()==0&&(board[1]&bingo->now_state[0]).count()==1){//対戦相手のが1つ　1点
      for(int i=0;i<64;i++)if(bingo->now_state[0][i])goodpos[i]+=1;
    }
    if((board[0]&bingo->now_state[0]).count()==0&&(board[1]&bingo->now_state[0]).count()==2){//対戦相手のが2つ　2点
      for(int i=0;i<64;i++)if(bingo->now_state[0][i])goodpos[i]+=2;
    }
  }
  return goodpos;
}

bitset<64> think(const vector<bitset<64> >& board){
  if(!(canplace(board)).any()){
    cout<<"connot place"<<endl;
    return bitset<64>{};//置ける場所が無い
  }
  vector<bitset<64> > mystart=board;
  vector<bitset<64> > opstart=make_opponent_board(mystart);
  
  //とりあえず自分のリーチがあったらノータイムでビンゴにする
  bitset<64> easywin=go_bingo(mystart);
  if(easywin.any())return random1(easywin);
  //とりあえず相手のリーチがあったらノータイムで止める
  bitset<64> easyblock=go_bingo(opstart);
  if(easyblock.any())return random1(easyblock);
  
  vector<vector<vector<shared_ptr<state> > > > mys3x1_x2{};
  bitset<16> mys3x1spot{};
  vector<vector<vector<shared_ptr<state> > > > ops3x1_x1{};
  bitset<16> ops3x1spot{};

  int bingoloopnum_max=6;
  int s3x1loopnum_max=5;
  auto mybingofu=async(launch::async,looptask,mystart,bingoloopnum_max,true,0);
  auto opbingofu=async(launch::async,looptask,opstart,bingoloopnum_max,false,0);
  auto mys3x1fu=async(launch::async,looptask,mystart,s3x1loopnum_max,true,1);
  auto ops3x1fu=async(launch::async,looptask,opstart,s3x1loopnum_max,false,1);
  loopresult myresultbingo=mybingofu.get();
  loopresult opresultbingo=opbingofu.get();
  loopresult myresults3x1=mys3x1fu.get();
  loopresult opresults3x1=ops3x1fu.get();
  mys3x1_x2=move(myresults3x1.result);
  mys3x1_x2[0]=move(myresultbingo.result[0]);
  mys3x1spot=move(myresults3x1.s3x1spot);
  ops3x1_x1=move(opresults3x1.result);
  ops3x1_x1[0]=move(opresultbingo.result[0]);
  ops3x1spot=move(opresults3x1.s3x1spot); 
  
  bitset<64> myaction{};
  //第一タスク。ダブルリーチの連鎖で勝つor負けない
  for(auto& x2:mys3x1_x2[0][0]){//完成しているtobingoのx2について、勝てるか見る
    for(auto& pre:x2->pre){
      bitset<64> tempmyaction=pre.myact;
      for(auto& x1:ops3x1_x1[0][0]){//放置すると完成する相手のx1で、mincountが自分のmincountよりも早いもの
	if(x1->mincount<=x2->mincount){
	  for(auto& next:x1->next) tempmyaction&=next;
	}
      }
      myaction|=tempmyaction;
    }
  }
  if(!myaction.any()){//すぐに勝てないなら、相手の妨害に専念
    myaction=canplace(board);
    for(auto& x1:ops3x1_x1[0][0]){
      cout<<x1->now_state;
      for(auto& next:x1->next) {
	myaction&=next;
	cout<<next<<endl;
      }
    }
    if(myaction.any()){
      cout<<"not attack my bingo"<<endl;
    }else{
      //mincountがもっとも小さいものだけでも止める
      for(int count=1;count<s3x1loopnum_max;count++){
	bitset<64> stopaction;
	for(auto& x1:ops3x1_x1[0][0]){
	  if(x1->mincount<=count){
	    for(auto& next:x1->next) stopaction|=next;
	  }
	}
	stopaction&=canplace(board);
	if(stopaction.any())myaction=stopaction;
      }
      if(!myaction.any())myaction=canplace(board);
      //自分のtobingoのpreを満たしたらあるいは
      bitset<64> t1_myaction{};
      for(int count=0;count<s3x1loopnum_max;count++){
	bitset<64> t1_goodpoint{};
	for(auto& x2:mys3x1_x2[0][1]){
	  if(x2->count<=count){
	    for(auto& pre:x2->pre)t1_goodpoint|=gopre_x2(board,pre.board);
	  }
	}
	t1_myaction=myaction;
	t1_myaction&=t1_goodpoint;
	if(t1_myaction.any())break;
      }
      if(t1_myaction.any())myaction=t1_myaction;
      else myaction=myaction;
      cout<<"connot block op's bingo"<<endl;
    }
  }else{
    cout<<"attack my bingo"<<endl;
  }
  cout<<myaction<<endl;

  //これらの第一タスクを満たす範囲内で、第2タスクを行う
  bitset<64> t_myaction=myaction;
  //第2タスク。自分のs3x1を相手より増やす
  //ある場所に置くことによる相手のs3x1の増加より自分のs3x1の増加が多ければ良いが、道中で話が変わることも多々あるので、とりあえずまず自分のs3x1spotを埋めず、次に相手のs3x1を止めて、次に自分のs3x1の完成を目指す
  for(int x=0;x<16;x++){
    if(mys3x1spot[x]==1){
      t_myaction[x*4+1]=0;//自分のs3x1spotを埋めない
    }
  }
  if(t_myaction.any()){
    bitset<64> t1_myaction=t_myaction;
    for(auto& x1:ops3x1_x1[1][0]){
      for(auto& next:x1->next) t1_myaction&=next;
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
	for(auto& pre:x2->pre) t1_2myaction|=(pre.myact&t_myaction);
      }
      if(t1_2myaction.any()){
	myaction=t1_2myaction;
	cout<<"connot block op's s3x1 attack my s3x1"<<endl;
      }else{
	myaction=t_myaction;
	cout<<"connot block op's s3x1"<<endl;
      }
    }
  }else{
    myaction=myaction;
    cout<<"break my s3x1"<<endl;
  }
  cout<<myaction<<endl;

  //第3タスク
  //自分の今は完成していないx2のpreを満たすと相手が嫌がる
  bitset<64> t4_1_myaction=myaction;
  //まずはtobingo
  bitset<64> t4_goodpoint{};
  for(auto& x2:mys3x1_x2[0][1]){
    for(auto& pre:x2->pre) t4_goodpoint|=gopre_x2(board,pre.board);
  }
  t4_1_myaction&=t4_goodpoint;
  if(t4_1_myaction.any()){//次はtos3x1
    bitset<64> t4_2_myaction=t4_1_myaction;
    t4_goodpoint.reset();
    for(auto& x2:mys3x1_x2[1][1]){
      for(auto& pre:x2->pre) t4_goodpoint|=gopre_x2(board,pre.board);
    }
    t4_2_myaction&=t4_goodpoint;
    if(t4_2_myaction.any()){
      myaction=t4_2_myaction;
      cout<<"attack my tobingo/s3x1pre"<<endl;
    }else{
      myaction=t4_1_myaction;
      cout<<"attack my tobingopre"<<endl;
    }
  }else{//tos3x1だけでも
    bitset<64> t4_2_myaction=myaction;
    t4_goodpoint.reset();
    for(auto& x2:mys3x1_x2[1][1]){
      for(auto& pre:x2->pre) t4_goodpoint|=gopre_x2(board,pre.board);
    }
    t4_2_myaction&=t4_goodpoint;
    if(t4_2_myaction.any()){
      myaction=t4_2_myaction;
      cout<<"attack my s3x1pre"<<endl;
    }else{
      myaction=myaction;
      cout<<"connot my tobingo/s3x1pre"<<endl;
    }
  }
  cout<<myaction<<endl;
  
  
  //第4タスク
  //置いたことにより今は満たしていない相手のx1のpreを満たすことは避けたい
  //なぜなら、相手のx1が完成することで、opactの拘束が新たに発生するため。
  //少し保守的すぎるか。いい場所を取ろうとする意思に欠く。
  bitset<64> t3_1_myaction=myaction;
  for(auto& x1:ops3x1_x1[0][1]){//まずはtobingoのx1を防ぐ
    for(auto& pre:x1->pre){//pre.board[1][場所]==1 かつ myaction[場所]==1だとだめ
      t3_1_myaction&=(~pre.board[1]);
    }
  }
  if(t3_1_myaction.any()){//次にtos3x1のx1を防ぐ
    bitset<64> t3_2_myaction=t3_1_myaction;
    for(auto& x1:ops3x1_x1[1][1]){
      for(auto& pre:x1->pre){//pre.board[1][場所]==1 かつ myaction[場所]==1だとだめ
	t3_2_myaction&=(~pre.board[1]);
      }
    }
    if(t3_2_myaction.any()) {
      myaction=t3_2_myaction;
      cout<<"block op's tobingo/s3x1x1pre"<<endl;
    }
    else{
      myaction=t3_1_myaction;
      cout<<"block op's tobingopre"<<endl;
    }
  }else{//tos3x1のx1だけでも防ぐ
    bitset<64> t3_2_myaction=myaction;
    for(auto& x1:ops3x1_x1[1][1]){
      for(auto& pre:x1->pre){//pre.board[1][場所]==1 かつ myaction[場所]==1だとだめ
	t3_2_myaction&=(~pre.board[1]);
      }
    }
    if(t3_2_myaction.any()) {
      myaction=t3_2_myaction;
      cout<<"block op's s3x1x1pre"<<endl;
    }
    else {
      myaction=myaction;
      cout<<"connot block op's tobingo/s3x1x1pre"<<endl;
    }
  }
  cout<<myaction<<endl;
  
  
  //第5タスク
  //まだ存在しうるbingoに含まれる場所の方が強い
  //投票する
  vector<int> goodpos=evalgoodpos(board);
  int maxnum=0;
  for(int i=0;i<64;i++){
    if(myaction[i]&&goodpos[i]>maxnum)maxnum=goodpos[i];
  }
  for(auto i=0;i<64;i++){
    if(myaction[i]&&goodpos[i]<maxnum)myaction[i]=0;
  }
  cout<<"seems strong"<<endl;
  cout<<myaction<<endl;
  
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
  while(true){
    auto action=think(board);
    cout<<"CPU action"<<endl;
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
