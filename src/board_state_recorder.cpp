#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include<actionlib/server/simple_action_server.h>
#include<jisyupro/Int8Array.h>
#include<jisyupro/Int32Array.h>
#include<visualization_msgs/MarkerArray.h>
#include<jisyupro/board_state_recorderAction.h>
#include<vector>
#include<string>
#include<cmath>
#include<algorithm>

using namespace std;
using namespace cv;

class MyCvPkg{
  ros::Publisher pub_;
  ros::Subscriber red_sub_;
  ros::Subscriber blue_sub_;
  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  actionlib::SimpleActionServer<jisyupro::board_state_recorderAction> server_;
  jisyupro::board_state_recorderFeedback feedback_;
  jisyupro::board_state_recorderResult result_;

  int thre;
  //x,y,zが、z*16+y*4+x*1
  jisyupro::Int8Array board_state;
  jisyupro::Int8Array pre_board_state;
  jisyupro::Int32Array red_count;
  jisyupro::Int32Array blue_count;
  bool action=false;
  int red_cb_num=0;
  int blue_cb_num=0;
  int newpoint =-1;
  
  void action_callback(const jisyupro::board_state_recorderGoalConstPtr &goal){
    if(goal->update){
      ROS_INFO_STREAM("ball_state_update_action receiveded");
      ros::Rate loop_rate(30);
      action=true;
      //全てのtimeがactioncallがあった時点より後になるまで待機
      bool loop=true;
      ros::Time calltime=ros::Time::now();
      while(loop&&ros::ok()){
	ros::spinOnce();
	pub_.publish(board_state);
	if(server_.isPreemptRequested()){
	  server_.setPreempted();
	  action=false;
	  return;
	}
	loop=false;
	if(red_count.header.stamp<calltime)loop=true;
	if(blue_count.header.stamp<calltime)loop=true;
	loop_rate.sleep();
      }
      //10msgの平均をとる
      red_cb_num=0;
      blue_cb_num=0;
      fill(red_count.int32s.begin(),red_count.int32s.end(),0);
      fill(blue_count.int32s.begin(),blue_count.int32s.end(),0);
      while((red_cb_num<10||blue_cb_num<10)&&ros::ok()){
	ros::spinOnce();
	pub_.publish(board_state);
	if(server_.isPreemptRequested()){
	  server_.setPreempted();
	  action=false;
	  return;
	}
	loop_rate.sleep();
      }
      for(int i=0; i<red_count.int32s.size();i++){
	red_count.int32s[i]/=red_cb_num;
      }
      for(int i=0; i<blue_count.int32s.size();i++){
	blue_count.int32s[i]/=blue_cb_num;
      }
      //しきい値以上の場所には球がある。board_stateを更新する
      //64個の要素。1(赤),-1(青),0(無)
      //ボールの存在は、閾値で決める
      //単純に上書きするだけ。不自然かの判定はしない
      pre_board_state=board_state;
      board_state.header=red_count.header;
      for(int i=0;i<board_state.int8s.size();i++){
	if(red_count.int32s[i]>thre){
	  board_state.int8s[i]=1;
	}
	if(blue_count.int32s[i]>thre){
	  board_state.int8s[i]=-1;
	}
      }
      result_.board_state=board_state;
      server_.setSucceeded(result_);
      action=false;
      ROS_INFO_STREAM("ball_state_action_Finished");
      return;
    }else if(goal->teach){
      ROS_INFO_STREAM("ball_state_teach_action receiveded");
      if(goal->newpos<0||goal->newpos>=64||goal->color==0){//何もせずに帰る
	ROS_INFO_STREAM("error goal->newpos<0||goal->newpos>=64||color==0");
	result_.board_state=board_state;
	server_.setSucceeded(result_);
	ROS_INFO_STREAM("ball_state_action_Finished");
	return;
      }
      pre_board_state=board_state;
      board_state.header.stamp=ros::Time::now();
      if(goal->color>0){
	board_state.int8s[goal->newpos]=1;
      }else{
	board_state.int8s[goal->newpos]=-1;
      }
      result_.board_state=board_state;
      server_.setSucceeded(result_);
      ROS_INFO_STREAM("ball_state_action_Finished");
      return;
    }else if(goal->undo){
      ROS_INFO_STREAM("ball_state_undo_action receiveded");
      board_state=pre_board_state;
      result_.board_state=board_state;
      server_.setSucceeded(result_);
      ROS_INFO_STREAM("ball_state_action_Finished");
      return;
    }else if(!goal->new_state.int8s.empty()){
      ROS_INFO_STREAM("ball_state_overwrite_action receiveded");
      if(goal->new_state.int8s.size()!=64){//何もせずに帰る
	ROS_INFO_STREAM("error goal->new_state.size()!=64");
	result_.board_state=board_state;
	server_.setSucceeded(result_);
	ROS_INFO_STREAM("ball_state_action_Finished");
	return;
      }
      pre_board_state=board_state;
      board_state.header.stamp=goal->new_state.header.stamp;
      board_state.int8s=goal->new_state.int8s;
      result_.board_state=board_state;
      server_.setSucceeded(result_);
      ROS_INFO_STREAM("ball_state_action_Finished");
      return;
    }
    return;
  }
  
  void Callback_red(const jisyupro::Int32ArrayConstPtr &msg){
    if(action){ 
      red_cb_num++;
      red_count.header=msg->header;
      for(int i=0; i<red_count.int32s.size();i++){
	red_count.int32s[i]+=msg->int32s[i];
      }
    }
    return;
  }

  void Callback_blue(const jisyupro::Int32ArrayConstPtr &msg){
    if(action){
      blue_cb_num++;
      blue_count.header=msg->header;
      for(int i=0; i<blue_count.int32s.size();i++){
	blue_count.int32s[i]+=msg->int32s[i];
      }
    }
    return;
  }
 
public:
  MyCvPkg(): nh_{}, private_nh_("~"), server_{private_nh_,"",boost::bind(&MyCvPkg::action_callback,this,_1),false} {
    private_nh_.param<int>("thre", thre,20);
    pub_ = private_nh_.advertise<jisyupro::Int8Array>("output",1);
    red_sub_ = nh_.subscribe("input_red",1,&MyCvPkg::Callback_red,this);
    blue_sub_ = nh_.subscribe("input_blue",1,&MyCvPkg::Callback_blue,this);
    server_.start();

    board_state.int8s.resize(64,0);
    red_count.int32s.resize(64,0);
    blue_count.int32s.resize(64,0);
    ROS_INFO_STREAM("top_ball_state2 prepared");
  }

  void pub(){
    pub_.publish(board_state);
  }
};

int main(int argc,char **argv){
  ros::init(argc,argv,"top_ball_state2");
  MyCvPkg mcp{};
  ros::Rate loop_rate(30);
  while(ros::ok()){
    ros::spinOnce();
    mcp.pub();
    loop_rate.sleep();
  }
}
