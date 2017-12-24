#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <pcl_ros/pcl_nodelet.h>
#include <pluginlib/class_list_macros.h>
#include <std_msgs/Int32.h>
#include <thread>

using namespace std;

namespace jisyupro
{
  class top_ball_state : public nodelet::Nodelet{
  public:
    //top_ball_state() {}
    //~top_ball_state() {}

    ros::Publisher pub_;
    thread working_thread_;
    
    virtual void onInit(){
      ROS_INFO_STREAM("top_ball_state_init");
      pub_=getPrivateNodeHandle().advertise<std_msgs::Int32>("output",1);
      working_thread_=thread{[&](){
	  ros::Rate rate(10); 
	  while (ros::ok()) {
	    std_msgs::Int32 outmsg;
	    outmsg.data=1;
	    pub_.publish(outmsg); 
	    rate.sleep();
	  }
	}
      };
    }
  };
}

PLUGINLIB_EXPORT_CLASS(jisyupro::top_ball_state, nodelet::Nodelet)
