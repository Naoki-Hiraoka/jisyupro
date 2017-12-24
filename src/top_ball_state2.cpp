#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include<actionlib/server/simple_action_server.h>
#include<jisyupro/Int8Array.h>
#include<jisyupro/Int32Stamped.h>
#include<jisyupro/top_ball_state2Action.h>
#include<vector>
#include<string>
#include<cmath>

using namespace std;
using namespace cv;

class MyCvPkg{
  vector<ros::Subscriber> sub_;
  ros::Publisher pub_;
  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  actionlib::SimpleActionServer<jisyupro::top_ball_state2Action> server_;
  jisyupro::top_ball_state2Feedback feedback_;
  jisyupro::top_ball_state2Result result_;

  vector<Int32Stamped> red_state(64);
  vector<Int32Stamped> blue_state(64);
  int thre = 5;
  
  void action_callback(const jisyupro::top_ball_state2GoalConstPtr &goal){
    if(!goal->start)return;
    ROS_INFO_STREAM("top_ball_stateaction receiveded");
    ros::Rate loop_rate(30);
    //全てのtimeがactioncallがあった時点より後になるまで待機
    bool loop=true;
    ros::time calltime=ros::time::now();
    while(loop&&ros::ok()){
      ros::spinOnce();
      if(server.isPreemptRequested()){
	server.setPreempted();
	return;
      }
      loop=false;
      for(auto& elt:red_state){
	if(elt.header.stamp<calltime)loop=true;
      }
      for(auto& elt:blue_state){
	if(elt.header.stamp<calltime)loop=true;
      }
      loop_rate.sleep();
    }
    //ココでmsgを作って返す
    //16個の要素。一番上の段数に応じて1234(赤),-1234(青),0(無)
    //ボールの存在は、閾値で決める
    jisyupro::Int8Array outmsg;
    outmsg->header=red_state[0].header;
    outmsg->header.stamp=calltime;
    outmsg.int8s.resize(16,0);
    for(int i=0;i<16;i++){
      for(int j=3;j>-1;j--){
	if(red_state[i*4+j]>thre){
	  outmsg.int8s[i]=j+1;
	  break;
	}
	if(blue_state[i*4+j]>thre){
	  outmsg.int8s[i]=-j-1;
	  break;
	}
      }
      outmsg.int8s[i]=0;
    }
    result_.result=outmsg;
    server.setSucceeded(result_);
    ROS_INFO_STREAM("top_ball_action_Finished");
    return;
  }
  
  void Callback(const jisyupro::Int32StampedConstPtr &msg,string color,int i,int j){
    switch(color){
    case "red":
      red_state[i*4+j]=msg;
      break;
    case "blue":
      blue_state[i*4+j]=msg;
      break;
    }
    return;
  }
 
public:
  MyCvPkg(): nh_{}, private_nh_("~") server_{private_nh_,"",boost::bind(&MyCvPkg::action_callback,this,_1),false}{
    string input;
    nh.getParam("input", input);
    for(int i=0;i<16;i++){
      for(int j=0;j<4;j++){
	string topicname=input+"_red"+to_string(i)+"_"+to_string(j);
	sub_.push_back(nh_.subscribe(topicname.c_str(),1,&MyCvPkg::Callback,this,"red",i,j));
      }
    }
    for(int i=0;i<16;i++){
      for(int j=0;j<4;j++){
	string topicname=input+"_blue"+to_string(i)+"_"+to_string(j);
	sub_.push_back(nh_.subscribe(topicname.c_str(),1,&MyCvPkg::Callback,this,"blue",i,j));
      }
    }
    pub_ = private_nh_.advertise<jisyupro::Int8Array>("output",1);
    server_.start();
    ROS_INFO_STREAM("top_ball_state2 prepared");
  }
};

int main(int argc,char **argv){
  ros::init(argc,argv,"top_ball_state2");
  MyCvPkg mcp{};
  ros::spin();
}


