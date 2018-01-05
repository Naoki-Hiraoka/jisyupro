#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include<jsk_recognition_msgs/PolygonArray.h>
#include<jsk_recognition_msgs/ModelCoefficientsArray.h>
#include<jisyupro/Vector3Array.h>
#include<vector>
#include<string>
#include<cmath>

using namespace std;
using namespace cv;

class MyCvPkg{
  ros::Subscriber polygon_sub_;
  ros::Subscriber coefficient_sub_;
  ros::Publisher polygon_pub_;
  ros::Publisher polygon2_pub_;
  ros::Publisher vector_pub_;
  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  vector<ros::Publisher> square_pub_;

  vector<float> coefficient{};
  
  void coefficientCallback(const jsk_recognition_msgs::ModelCoefficientsArrayConstPtr &msg){
    if(msg->coefficients.empty())return;
    coefficient=msg->coefficients[0].values;
    return;
  }
  
  void polygonCallback(const jsk_recognition_msgs::PolygonArrayConstPtr &msg){
    if(msg->polygons.empty()||coefficient.empty())return;
    jsk_recognition_msgs::PolygonArray polygonmsg;
    polygonmsg.header=msg->header;
    jsk_recognition_msgs::PolygonArray polygon2msg;
    polygon2msg.header=msg->header;
    jsk_recognition_msgs::PolygonArray squaremsg;
    squaremsg.header=msg->header;
    jisyupro::Vector3Array vectormsg;
    vectormsg.header=msg->header;
    
    //射影行列を作る
    float norm_x=sqrt(coefficient[2]*coefficient[2]+coefficient[0]*coefficient[0]);
    float norm_y=sqrt(coefficient[0]*coefficient[1]*coefficient[0]*coefficient[1]+(coefficient[0]*coefficient[0]+coefficient[2]*coefficient[2])*(coefficient[0]*coefficient[0]+coefficient[2]*coefficient[2])+coefficient[1]*coefficient[2]*coefficient[1]*coefficient[2]);
    Mat M=Mat{vector<float>{
	  -coefficient[2]/norm_x,-coefficient[0]*coefficient[1]/norm_y,0,
	  0,(coefficient[0]*coefficient[0]+coefficient[2]*coefficient[2])/norm_y,0,
	  coefficient[0]/norm_x,-coefficient[1]*coefficient[2]/norm_y,-coefficient[3]/coefficient[2]},true}.reshape(0,3);
    Mat M_inv=M.inv();
    //平面に射影
    vector<Point2f> cont{};
    for(auto& point:msg->polygons[0].polygon.points){
      Mat pointvec=Mat{vector<float>{point.x,point.y,point.z},true}.reshape(0,3);
      Mat newpointvec=M_inv*pointvec;
      cont.emplace_back(newpointvec.at<float>(0,0),newpointvec.at<float>(1,0));
    }

    //ボードの領域を長方形で表す。
    RotatedRect board=minAreaRect(cont);
    while(board.angle>45){
      board.angle-=90;
      swap(board.size.width,board.size.height);
    }
    while(board.angle<-45){
      board.angle+=90;
      swap(board.size.width,board.size.height);
    }
    Point2f pts[4];
    board.points(pts);

    //ボードの長方形を送信
    geometry_msgs::PolygonStamped polygon;
    polygon.header=msg->header;
    for(auto& pt:pts){
      Mat pointvec=Mat{vector<float>{pt.x,pt.y,1},true}.reshape(0,3);
      Mat newpointvec=M*pointvec;
      geometry_msgs::Point32 point{};
      point.x=newpointvec.at<float>(0,0);
      point.y=newpointvec.at<float>(1,0);
      point.z=newpointvec.at<float>(2,0);
      polygon.polygon.points.push_back(point);
    }
    swap(polygon.polygon.points[1],polygon.polygon.points[3]);//法線ベクトル案件
    polygonmsg.polygons.push_back(polygon);
    squaremsg.polygons=vector<geometry_msgs::PolygonStamped>{polygon};
    square_pub_[16].publish(squaremsg);

    //16分割する
    RotatedRect boardpart{Point2f{-1,-1},Size2f{board.size.width*1/6,board.size.height*1/6},board.angle};
    Point2f dwidth{boardpart.size.width*cos(boardpart.angle/180*(float)M_PI),boardpart.size.width*sin(boardpart.angle/180*(float)M_PI)};
    Point2f dheight{-boardpart.size.height*sin(boardpart.angle/180*(float)M_PI),boardpart.size.height*cos(boardpart.angle/180*(float)M_PI)};
    int square_num=0;
    for(int j=0;j<4;j++){
      for(int i=0;i<4;i++){
	boardpart.center.x=board.center.x+dwidth.x*(i-1.5-0.25)+dheight.x*(j-1.5);
	boardpart.center.y=board.center.y+dwidth.y*(i-1.5-0.25)+dheight.y*(j-1.5);
	Point2f pts[4];
	boardpart.points(pts);
	//四角形を送信
	geometry_msgs::PolygonStamped polygon;
	polygon.header=msg->header;
	for(auto& pt:pts){
	  Mat pointvec=Mat{vector<float>{pt.x,pt.y,1},true}.reshape(0,3);
	  Mat newpointvec=M*pointvec;
	  geometry_msgs::Point32 point{};
	  point.x=newpointvec.at<float>(0,0);
	  point.y=newpointvec.at<float>(1,0);
	  point.z=newpointvec.at<float>(2,0);
	  polygon.polygon.points.push_back(point);
	}
	swap(polygon.polygon.points[1],polygon.polygon.points[3]);//法線ベクトル案件
	polygonmsg.polygons.push_back(polygon);
	polygon2msg.polygons.push_back(polygon);
	squaremsg.polygons=vector<geometry_msgs::PolygonStamped>{polygon};
	square_pub_[square_num].publish(squaremsg);
	square_num++;
	//中心を送信
	Mat pointvec=Mat{vector<float>{boardpart.center.x,boardpart.center.y,1},true}.reshape(0,3);
	Mat newpointvec=M*pointvec;
	geometry_msgs::Vector3 pos;
	pos.x=newpointvec.at<float>(0,0);
	pos.y=newpointvec.at<float>(1,0);
	pos.z=newpointvec.at<float>(2,0);
	vectormsg.vector3s.push_back(pos);
      }
    }

    //publish
    polygon_pub_.publish(polygonmsg);
    polygon2_pub_.publish(polygon2msg);
    vector_pub_.publish(vectormsg);
    return;
  }
public:
  MyCvPkg(): nh_{}, private_nh_("~") {
    polygon_sub_ = nh_.subscribe("input_polygon",1,&MyCvPkg::polygonCallback,this);
    coefficient_sub_ = nh_.subscribe("input_coefficient",1,&MyCvPkg::coefficientCallback,this);
    polygon_pub_ = private_nh_.advertise<jsk_recognition_msgs::PolygonArray>("polygon",1);
    polygon2_pub_ = private_nh_.advertise<jsk_recognition_msgs::PolygonArray>("polygon2",1);
    vector_pub_ = private_nh_.advertise<jisyupro::Vector3Array>("vector",1);
    for(int i=0;i<16;i++){
      string pub_name=string{"square"}+to_string(i);
      square_pub_.push_back(private_nh_.advertise<jsk_recognition_msgs::PolygonArray>(pub_name.c_str(),1));
    }
    square_pub_.push_back(private_nh_.advertise<jsk_recognition_msgs::PolygonArray>("square",1));
  }
};

int main(int argc,char **argv){
  ros::init(argc,argv,"board_detector");
  MyCvPkg mcp{};
  ros::spin();
}


