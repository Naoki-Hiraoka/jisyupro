#include <ros/ros.h>
#include<actionlib/server/simple_action_server.h>
#include<jisyupro/Int8Array.h>
#include<visualization_msgs/MarkerArray.h>
#include<jisyupro/thinkAction.h>
#include<vector>
#include<string>
#include<cmath>
#include<algorithm>
#include<random>

using namespace std;

class Rand_int{
public:
  Rand_int (int low,int high,int seed=0): r(bind(std::uniform_int_distribution<>(low,high),std::default_random_engine(seed))) {}
  int operator() (){return r();}
private:
  std::function<int()> r;
};

class MyCvPkg{
  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  actionlib::SimpleActionServer<jisyupro::thinkAction> server_;
  jisyupro::thinkFeedback feedback_;
  jisyupro::thinkResult result_;
  Rand_int rand;

  int think_random(vector<int8_t> board){
    while(true){
      int pos=rand()%16;
      for(int i=0;i<4;i++){
	if(board[pos*4+i]==0)return pos*4+i;
      }
    }
  }
  
  void action_callback(const jisyupro::thinkGoalConstPtr &goal){
    /*
    if(server_.isPreemptRequested()){
      server_.setPreempted();
    }
    server_.publishFeedback(feedback_);
    */
    ROS_INFO_STREAM("think_action_start");

    //ランダム
    ROS_INFO_STREAM("think_with_random_method");
    result_.newpos=think_random(goal->board.int8s);
    
    server_.setSucceeded(result_);
    ROS_INFO_STREAM("think_action_Finished");
    return;
  }
public:
  MyCvPkg(): nh_{}, private_nh_("~"), server_{private_nh_,"",boost::bind(&MyCvPkg::action_callback,this,_1),false},rand{0,10000,static_cast<int>(ros::Time::now().sec)} {
    //private_nh_.param<int>("thre", thre,20);
    server_.start();
    ROS_INFO_STREAM("think prepared");
  }
};

int main(int argc,char **argv){
  ros::init(argc,argv,"think");
  MyCvPkg mcp{};
  ros::Rate loop_rate(10);
  while(ros::ok()){
    ros::spinOnce();
    loop_rate.sleep();
  }
}
