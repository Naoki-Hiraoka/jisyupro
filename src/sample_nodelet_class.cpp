/*
 * sample_nodelet_class.cpp
 *
 *  Created on: 2016/09/18
 *      Author: cryborg21
 */
#include <pluginlib/class_list_macros.h>
#include <nodelet/nodelet.h>

namespace jisyupro_ns
{
class SampleNodeletClass : public nodelet::Nodelet
{
public:
    SampleNodeletClass();
    ~SampleNodeletClass();

    virtual void onInit();
};
} // namespace sample_nodelet_ns


namespace jisyupro_ns
{
SampleNodeletClass::SampleNodeletClass()
{
  ROS_INFO("SampleNodeletClass Constructor");
}

SampleNodeletClass::~SampleNodeletClass()
{
  ROS_INFO("SampleNodeletClass Destructor");
}

void SampleNodeletClass::onInit()
{
    NODELET_INFO("SampleNodeletClass - %s", __FUNCTION__);
}
} // namespace sample_nodelet_ns

PLUGINLIB_EXPORT_CLASS(jisyupro_ns::SampleNodeletClass, nodelet::Nodelet)
