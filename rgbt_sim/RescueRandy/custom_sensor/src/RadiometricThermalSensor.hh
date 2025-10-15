#ifndef RADIOMETRIC_THERMAL_SENSOR_HH_
#define RADIOMETRIC_THERMAL_SENSOR_HH_

#include <string>
#include <gz/sim/System.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/common/Console.hh>
#include <gz/transport/Node.hh>
#include <gz/msgs/image.pb.h>

namespace custom
{
  class RadiometricThermalSensor
      : public gz::sim::System,
        public gz::sim::ISystemConfigure,
        public gz::sim::ISystemPreUpdate
  {
    public: void Configure(const gz::sim::Entity &_entity,
                           const std::shared_ptr<const sdf::Element> &_sdf,
                           gz::sim::EntityComponentManager &_ecm,
                           gz::sim::EventManager &_eventMgr) override;

    public: void PreUpdate(const gz::sim::UpdateInfo &_info,
                           gz::sim::EntityComponentManager &_ecm) override;

    // callback for /thermal/radiometric topic
    private: void OnRadiometricImage(const gz::msgs::Image &_msg);

    // members
    private: gz::transport::Node node_;
    private: std::string topic_ = "/thermal/radiometric";
    private: std::string outDir_ = "/tmp/thermal_radiometric_output";
    private: double minK_ = 233.0;   // default -40°C
    private: double maxK_ = 373.0;   // default 100°C
    private: bool subscribed_ = false;
  };
}

#endif
