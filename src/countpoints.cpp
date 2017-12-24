#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <pcl_ros/pcl_nodelet.h>
#include <pluginlib/class_list_macros.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include<jisyupro/Int32Stamped.h>
#include <thread>

using namespace std;

namespace jisyupro
{
  class countpoints : public nodelet::Nodelet{
  public:
    //top_ball_state() {}
    //~top_ball_state() {}

    ros::Publisher pub_;
    ros::Subscriber sub_;
    thread working_thread_;

    void cloud_cb (const sensor_msgs::PointCloud2ConstPtr& cloud_msg)
    {
      jisyupro::Int32Stamped outmsg{};
      outmsg.header=cloud_msg->header;
      outmsg.data=cloud_msg->data.size();      
      // Publish the data.
      pub_.publish (outmsg);
    }
    
    
    virtual void onInit(){
      ROS_INFO_STREAM("top_ball_state_init");
      pub_=getPrivateNodeHandle().advertise<jisyupro::Int32Stamped>("output",1);
      sub_=getNodeHandle().subscribe ("input", 1, &countpoints::cloud_cb,this);
      working_thread_=thread{[&](){
	  ros::Rate rate(10); 
	  while (ros::ok()) {
	    ros::spinOnce();
	    rate.sleep();
	  }
	}
      };
    }
  };
}

PLUGINLIB_EXPORT_CLASS(jisyupro::countpoints, nodelet::Nodelet)
