//https://github.com/jsk-ros-pkg/jsk_recognition/blob/master/jsk_pcl_ros/src/multi_plane_extraction_nodelet.cppを参考にしました。

//#include "jsk_pcl_ros/multi_plane_extraction.h"
#include <ros/ros.h>
#include <ros/names.h>
#include <pcl_ros/pcl_nodelet.h>
#include <message_filters/pass_through.h>
#include <message_filters/time_synchronizer.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include "jsk_recognition_msgs/ClusterPointIndices.h"
#include "sensor_msgs/PointCloud2.h"
#include "jsk_recognition_msgs/ModelCoefficientsArray.h"
#include "jsk_recognition_msgs/PolygonArray.h"
#include <dynamic_reconfigure/server.h>
#include "jsk_pcl_ros/MultiPlaneExtractionConfig.h"
#include "jsk_recognition_utils/pcl_util.h"
#include "jsk_recognition_utils/pcl_conversion_util.h"
#include <jsk_topic_tools/vital_checker.h>
#include <jsk_topic_tools/diagnostic_nodelet.h>
#include "jsk_pcl_ros/tf_listener_singleton.h"

#include <pluginlib/class_list_macros.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/extract_polygonal_prism_data.h>
#include <jsk_recognition_utils/pcl_ros_util.h>
#include <jsk_topic_tools/log_utils.h>
#include <pcl/filters/project_inliers.h>
#include <set>
#include <sstream>

namespace jisyupro
{
  class MultiPlaneExtraction: public jsk_topic_tools::DiagnosticNodelet
  {
  public:

    typedef message_filters::sync_policies::ExactTime<
      sensor_msgs::PointCloud2,
    jsk_recognition_msgs::ClusterPointIndices,
    jsk_recognition_msgs::ModelCoefficientsArray,
    jsk_recognition_msgs::PolygonArray> SyncPolicy;
    typedef message_filters::sync_policies::ApproximateTime<
      sensor_msgs::PointCloud2,
      jsk_recognition_msgs::ClusterPointIndices,
      jsk_recognition_msgs::ModelCoefficientsArray,
      jsk_recognition_msgs::PolygonArray> ASyncPolicy;
    typedef jsk_pcl_ros::MultiPlaneExtractionConfig Config;

    MultiPlaneExtraction(): DiagnosticNodelet("MultiPlaneExtraction") { }
  protected:
    ////////////////////////////////////////////////////////
    // methods
    ////////////////////////////////////////////////////////
    virtual void onInit();

    virtual void fillEmptyIndices(
				  const jsk_recognition_msgs::PolygonArray::ConstPtr& polygons);
    virtual void fillEmptyCoefficients(
				       const jsk_recognition_msgs::PolygonArray::ConstPtr& polygons);

    virtual void extract(const sensor_msgs::PointCloud2::ConstPtr& input,
			 const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices,
			 const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients,
			 const jsk_recognition_msgs::PolygonArray::ConstPtr& polygons);

    virtual void configCallback (Config &config, uint32_t level);

    virtual void updateDiagnostic(
				  diagnostic_updater::DiagnosticStatusWrapper &stat);

    virtual void subscribe();
    virtual void unsubscribe();
    ////////////////////////////////////////////////////////
    // ROS variables
    ////////////////////////////////////////////////////////
    boost::mutex mutex_;
    ros::Publisher pub_, nonplane_pub_;
    ros::Publisher pub_indices_;
    boost::shared_ptr <dynamic_reconfigure::Server<Config> > srv_;
    message_filters::Subscriber<sensor_msgs::PointCloud2> sub_input_;
    message_filters::Subscriber<jsk_recognition_msgs::ModelCoefficientsArray> sub_coefficients_;
    message_filters::Subscriber<jsk_recognition_msgs::PolygonArray> sub_polygons_;
    message_filters::Subscriber<jsk_recognition_msgs::ClusterPointIndices> sub_indices_;
    message_filters::PassThrough<jsk_recognition_msgs::ClusterPointIndices> null_indices_;
    message_filters::PassThrough<jsk_recognition_msgs::ModelCoefficientsArray> null_coefficients_;
    boost::shared_ptr<message_filters::Synchronizer<SyncPolicy> > sync_;
    boost::shared_ptr<message_filters::Synchronizer<ASyncPolicy> > async_;


    ////////////////////////////////////////////////////////
    // Diagnostics Variables
    ////////////////////////////////////////////////////////
    jsk_recognition_utils::Counter plane_counter_;

    ////////////////////////////////////////////////////////
    // Parameters
    ////////////////////////////////////////////////////////
    tf::TransformListener* tf_listener_;
    bool use_async_;
    bool keep_organized_;
    int maximum_queue_size_;
    double min_height_, max_height_;
    bool use_indices_;
    double maginify_;
    bool use_sensor_frame_;
    std::string sensor_frame_;
    bool use_coefficients_;
  private:


  };
}

namespace jisyupro
{

  void MultiPlaneExtraction::onInit()
  {
    pcl::console::setVerbosityLevel(pcl::console::L_ERROR);
    DiagnosticNodelet::onInit();
    pnh_->param("use_indices", use_indices_, true);
    pnh_->param("use_async", use_async_, false);
    ////////////////////////////////////////////////////////
    // Publishers
    ////////////////////////////////////////////////////////
    pub_ = advertise<sensor_msgs::PointCloud2>(*pnh_, "output", 1);
    nonplane_pub_ = advertise<pcl::PointCloud<pcl::PointXYZRGB> >(*pnh_, "output_nonplane_cloud", 1);
    pub_indices_ = advertise<PCLIndicesMsg>(*pnh_, "output/indices", 1);
    if (!pnh_->getParam("max_queue_size", maximum_queue_size_)) {
      maximum_queue_size_ = 100;
    }

    pnh_->param("use_coefficients", use_coefficients_, false);
    pnh_->param("use_sensor_frame", use_sensor_frame_, false);
    if (use_sensor_frame_) {
      pnh_->param("sensor_frame", sensor_frame_, std::string("head_root"));
      tf_listener_ = jsk_pcl_ros::TfListenerSingleton::getInstance();
      if (use_coefficients_) {
	NODELET_WARN("'~use_sensor_frame' and '~use_coefficients' cannot be enabled at the same time."
		     " disabled '~use_coefficients'");
	use_coefficients_ = false;
      }
    } else if (!use_coefficients_) {
      NODELET_WARN("'~use_coefficients' and '~use_sensor_frame' are both disabled."
		                      " Normal axes of planes are estimated with PCA"
		   " and they may be flipped unintentionally.");
    }

    ////////////////////////////////////////////////////////
    // Dynamic Reconfigure
    ////////////////////////////////////////////////////////
    srv_ = boost::make_shared <dynamic_reconfigure::Server<Config> > (*pnh_);
    dynamic_reconfigure::Server<Config>::CallbackType f =
      boost::bind (&MultiPlaneExtraction::configCallback, this, _1, _2);
    srv_->setCallback (f);
    onInitPostProcess();
  }

  void MultiPlaneExtraction::subscribe()
  {
    ////////////////////////////////////////////////////////
    // Subscribe
    ////////////////////////////////////////////////////////

    sub_input_.subscribe(*pnh_, "input", 1);
    sub_polygons_.subscribe(*pnh_, "input_polygons", 1);

    if (use_coefficients_) {
      sub_coefficients_.subscribe(*pnh_, "input_coefficients", 1);
    } else {
      sub_polygons_.registerCallback(
				     boost::bind(&MultiPlaneExtraction::fillEmptyCoefficients, this, _1));
    }

    if (use_indices_) {
      sub_indices_.subscribe(*pnh_, "indices", 1);
    } else {
      sub_polygons_.registerCallback(
				     boost::bind(&MultiPlaneExtraction::fillEmptyIndices, this, _1));
    }

    if (use_async_) {
      async_ = boost::make_shared<message_filters::Synchronizer<ASyncPolicy> >(maximum_queue_size_);
      if (use_indices_) {
	if (use_coefficients_) {
	  async_->connectInput(sub_input_, sub_indices_, sub_coefficients_, sub_polygons_);
	} else {
	  async_->connectInput(sub_input_, sub_indices_, null_coefficients_, sub_polygons_);
	}
      } else {
	if (use_coefficients_) {
	  async_->connectInput(sub_input_, null_indices_, sub_coefficients_, sub_polygons_);
	} else {
	  async_->connectInput(sub_input_, null_indices_, null_coefficients_, sub_polygons_);
	}
      }
      async_->registerCallback(boost::bind(&MultiPlaneExtraction::extract, this, _1, _2, _3, _4));
    } else {
      sync_ = boost::make_shared<message_filters::Synchronizer<SyncPolicy> >(maximum_queue_size_);
      if (use_indices_) {
	if (use_coefficients_) {
	  sync_->connectInput(sub_input_, sub_indices_, sub_coefficients_, sub_polygons_);
	} else {
	  sync_->connectInput(sub_input_, sub_indices_, null_coefficients_, sub_polygons_);
	}
      } else {
	if (use_coefficients_) {
	  sync_->connectInput(sub_input_, null_indices_, sub_coefficients_, sub_polygons_);
	} else {
	  sync_->connectInput(sub_input_, null_indices_, null_coefficients_, sub_polygons_);
	}
      }
      sync_->registerCallback(boost::bind(&MultiPlaneExtraction::extract, this, _1, _2, _3, _4));
    }
  }

  void MultiPlaneExtraction::unsubscribe()
  {
    sub_input_.unsubscribe();
    if (use_indices_) {
      sub_indices_.unsubscribe();
    }
    sub_polygons_.unsubscribe();
    if (use_coefficients_) {
      sub_coefficients_.unsubscribe();
    }
  }

  void MultiPlaneExtraction::configCallback(Config& config, uint32_t level)
  {
    boost::mutex::scoped_lock lock(mutex_);
    min_height_ = config.min_height;
    max_height_ = config.max_height;
    maginify_ = config.maginify;
    keep_organized_ = config.keep_organized;
  }

  void MultiPlaneExtraction::updateDiagnostic(
					      diagnostic_updater::DiagnosticStatusWrapper &stat)
  {
    if (vital_checker_->isAlive()) {
      stat.summary(diagnostic_msgs::DiagnosticStatus::OK,
		   "MultiPlaneExtraction running");
      stat.add("Minimum Height", min_height_);
      stat.add("Maximum Height", max_height_);
      stat.add("Number of Planes", plane_counter_.mean());
    }
    else {
      jsk_topic_tools::addDiagnosticErrorSummary(
						 "MultiPlaneExtraction", vital_checker_, stat);
    }
  }

  void MultiPlaneExtraction::fillEmptyIndices(
					      const jsk_recognition_msgs::PolygonArray::ConstPtr &polygons)
  {
    jsk_recognition_msgs::ClusterPointIndices indices;
    indices.header = polygons->header;
    indices.cluster_indices.resize(polygons->polygons.size());
    null_indices_.add(
		      boost::make_shared<jsk_recognition_msgs::ClusterPointIndices>(indices));
  }

  void MultiPlaneExtraction::fillEmptyCoefficients(
						   const jsk_recognition_msgs::PolygonArray::ConstPtr &polygons)
  {
    jsk_recognition_msgs::ModelCoefficientsArray coeffs;
    coeffs.header = polygons->header;
    coeffs.coefficients.resize(polygons->polygons.size());
    null_coefficients_.add(
			   boost::make_shared<jsk_recognition_msgs::ModelCoefficientsArray>(coeffs));
  }

  void MultiPlaneExtraction::extract(const sensor_msgs::PointCloud2::ConstPtr& input,
				     const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices,
				     const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients,
				     const jsk_recognition_msgs::PolygonArray::ConstPtr& polygons)
  {
    boost::mutex::scoped_lock lock(mutex_);
    vital_checker_->poke();
    // check header
    if(!jsk_recognition_utils::isSameFrameId(input->header.frame_id,
					     indices->header.frame_id) ||
       !jsk_recognition_utils::isSameFrameId(input->header.frame_id,
					     coefficients->header.frame_id) ||
       !jsk_recognition_utils::isSameFrameId(input->header.frame_id,
					     polygons->header.frame_id)) {
      std::ostringstream oss;
      oss << "frame id does not match. cloud: " << input->header.frame_id
	  << ", polygons: " << polygons->header.frame_id;
      if (use_indices_) {
	oss << ", indices: " << indices->header.frame_id;
      }
      if (use_coefficients_) {
	oss << ", coefficients: " << coefficients->header.frame_id;
      }
      NODELET_ERROR_STREAM(oss.str());
      return;
    }

    // set viewpoint to determine normal axes of the planes
    Eigen::Vector3f viewpoint;
    if (use_sensor_frame_) {
      try {
	tf::StampedTransform transform
	  = jsk_pcl_ros::lookupTransformWithDuration(tf_listener_,
					input->header.frame_id,
					sensor_frame_,
					input->header.stamp,
					ros::Duration(5.0));
	Eigen::Affine3f sensor_pose;
	tf::transformTFToEigen(transform, sensor_pose);
	viewpoint = Eigen::Vector3f(sensor_pose.translation());
      }
      catch (tf2::ConnectivityException &e)
	{
	  NODELET_ERROR("Transform error: %s", e.what());
	}
      catch (tf2::InvalidArgumentException &e)
	{
	  NODELET_ERROR("Transform error: %s", e.what());
	}
      catch (...)
	{
	  NODELET_ERROR("Unknown transform error");
	}
    }

    // convert all to the pcl types
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr input_cloud(new pcl::PointCloud<pcl::PointXYZRGB>());
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr nonplane_cloud(new pcl::PointCloud<pcl::PointXYZRGB>());
    pcl::fromROSMsg(*input, *input_cloud);
    if (indices) {
      // concat indices into one PointIndices
      pcl::PointIndices::Ptr all_indices (new pcl::PointIndices);
      for (size_t i = 0; i < indices->cluster_indices.size(); i++) {
	std::vector<int> one_indices = indices->cluster_indices[i].indices;
	for (size_t j = 0; j < one_indices.size(); j++) {
	  all_indices->indices.push_back(one_indices[j]);
	}
      }


      pcl::ExtractIndices<pcl::PointXYZRGB> extract_nonplane;
      extract_nonplane.setNegative(true);
      extract_nonplane.setKeepOrganized(keep_organized_);
      extract_nonplane.setInputCloud(input_cloud);
      extract_nonplane.setIndices(all_indices);
      extract_nonplane.filter(*nonplane_cloud);
      sensor_msgs::PointCloud2 ros_result;
      pcl::toROSMsg(*nonplane_cloud, ros_result);
      ros_result.header = input->header;
      nonplane_pub_.publish(ros_result);
    }
    else {
      nonplane_cloud = input_cloud;
    }
    // for each plane, project nonplane_cloud to the plane and find the points
    // inside of the polygon

    std::set<int> result_set;
    plane_counter_.add(polygons->polygons.size());
    for (size_t plane_i = 0; plane_i < polygons->polygons.size(); plane_i++) {
      if (use_coefficients_) {
	// set viewpoint from coefficients
	for (size_t vec_i = 0; vec_i < 3; ++vec_i)
	  viewpoint[vec_i] = coefficients->coefficients[plane_i].values[vec_i];
      }
      pcl::ExtractPolygonalPrismData<pcl::PointXYZRGB> prism_extract;
      prism_extract.setViewPoint(viewpoint[0], viewpoint[1], viewpoint[2]);
      pcl::PointCloud<pcl::PointXYZRGB>::Ptr hull_cloud(new pcl::PointCloud<pcl::PointXYZRGB>());
      geometry_msgs::Polygon the_polygon = polygons->polygons[plane_i].polygon;
      if (the_polygon.points.size() <= 2) {
	NODELET_WARN("too small polygon");
	continue;
      }
      // compute centroid first
      Eigen::Vector3f centroid(0, 0, 0);
      for (size_t i = 0; i < the_polygon.points.size(); i++) {
	pcl::PointXYZRGB p;
	jsk_recognition_utils::pointFromXYZToXYZ<geometry_msgs::Point32, pcl::PointXYZRGB>(
											   the_polygon.points[i], p);
	centroid = centroid + p.getVector3fMap();
      }
      centroid = centroid / the_polygon.points.size();

      for (size_t i = 0; i < the_polygon.points.size(); i++) {
	pcl::PointXYZRGB p;
	jsk_recognition_utils::pointFromXYZToXYZ<geometry_msgs::Point32, pcl::PointXYZRGB>(
											   the_polygon.points[i], p);
	Eigen::Vector3f dir = (p.getVector3fMap() - centroid).normalized();
	p.getVector3fMap() = dir * maginify_ + p.getVector3fMap();
	hull_cloud->points.push_back(p);
      }

      pcl::PointXYZRGB p_last;
      jsk_recognition_utils::pointFromXYZToXYZ<geometry_msgs::Point32, pcl::PointXYZRGB>(
											 the_polygon.points[0], p_last);
      hull_cloud->points.push_back(p_last);

      prism_extract.setInputCloud(nonplane_cloud);
      prism_extract.setHeightLimits(min_height_, max_height_);
      prism_extract.setInputPlanarHull(hull_cloud);
      pcl::PointIndices output_indices;
      prism_extract.segment(output_indices);
      // append output to result_cloud
      for (size_t i = 0; i < output_indices.indices.size(); i++) {
	result_set.insert(output_indices.indices[i]);
      }
    }

    // convert std::set to PCLIndicesMsg
    //PCLIndicesMsg output_indices;
    pcl::PointCloud<pcl::PointXYZRGB> result_cloud;
    pcl::PointIndices::Ptr all_result_indices (new pcl::PointIndices());
    for (std::set<int>::iterator it = result_set.begin();
	 it != result_set.end();
	 it++) {
      all_result_indices->indices.push_back(*it);
    }

    pcl::ExtractIndices<pcl::PointXYZRGB> extract_all_indices;
    extract_all_indices.setKeepOrganized(keep_organized_);
    extract_all_indices.setInputCloud(nonplane_cloud);
    extract_all_indices.setIndices(all_result_indices);
    extract_all_indices.filter(result_cloud);

    sensor_msgs::PointCloud2 ros_result;
    pcl::toROSMsg(result_cloud, ros_result);
    ros_result.header = input->header;
    pub_.publish(ros_result);
    PCLIndicesMsg ros_indices;
    pcl_conversions::fromPCL(*all_result_indices, ros_indices);
    ros_indices.header = input->header;
    pub_indices_.publish(ros_indices);
    diagnostic_updater_->update();
  }
}

PLUGINLIB_EXPORT_CLASS (jisyupro::MultiPlaneExtraction, nodelet::Nodelet);
