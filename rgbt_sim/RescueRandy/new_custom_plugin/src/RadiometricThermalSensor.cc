/*
 * Copyright (C) 2025
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gz/sim/System.hh>
#include <gz/plugin/Register.hh>
#include <gz/transport/Node.hh>
#include <gz/msgs/image.pb.h>
#include <gz/common/Console.hh>

#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>

namespace gz {
namespace sim {
namespace systems {

class RadiometricThermalSensor : public System,
                                  public ISystemConfigure,
                                  public ISystemPostUpdate {
public:
  RadiometricThermalSensor();
  ~RadiometricThermalSensor() override;

  void Configure(const Entity &_entity,
                 const std::shared_ptr<const sdf::Element> &_sdf,
                 EntityComponentManager &_ecm,
                 EventManager &_eventMgr) override;

  void PostUpdate(const UpdateInfo &_info,
                  const EntityComponentManager &_ecm) override;

private:
  void OnThermalImage(const msgs::Image &_msg);
  bool SaveRadiometricData(const std::vector<uint16_t> &_data, 
                           unsigned int _width,
                           unsigned int _height, 
                           uint64_t _frameNum);

  transport::Node node;
  std::string savePath;
  float minTemp = 253.15f;
  float maxTemp = 373.0f;
  float resolution = 3.0f;
  uint64_t frameCounter = 0;
  bool initialized = false;
};

RadiometricThermalSensor::RadiometricThermalSensor()
{
  std::cout << "RadiometricThermalSensor: Constructor called" << std::endl;
}

RadiometricThermalSensor::~RadiometricThermalSensor()
{
  std::cout << "RadiometricThermalSensor: Destructor called, saved " 
            << this->frameCounter << " frames" << std::endl;
}

void RadiometricThermalSensor::Configure(const Entity &/*_entity*/,
                                         const std::shared_ptr<const sdf::Element> &_sdf,
                                         EntityComponentManager &/*_ecm*/,
                                         EventManager &/*_eventMgr*/)
{
  std::cout << "RadiometricThermalSensor: Configure called" << std::endl;

  // Read configuration parameters
  if (_sdf->HasElement("save_path"))
  {
    this->savePath = _sdf->Get<std::string>("save_path");
  }
  else
  {
    this->savePath = "./thermal_radiometric";
  }

  if (_sdf->HasElement("min_temp"))
  {
    this->minTemp = _sdf->Get<float>("min_temp");
  }

  if (_sdf->HasElement("max_temp"))
  {
    this->maxTemp = _sdf->Get<float>("max_temp");
  }

  if (_sdf->HasElement("resolution"))
  {
    this->resolution = _sdf->Get<float>("resolution");
  }

  // Create save directory
  system(("mkdir -p " + this->savePath).c_str());

  // Subscribe to thermal camera topic
  if (!this->node.Subscribe("/thermal/image",
                            &RadiometricThermalSensor::OnThermalImage, this))
  {
    std::cerr << "RadiometricThermalSensor: Failed to subscribe to /thermal/image" << std::endl;
  }
  else
  {
    std::cout << "RadiometricThermalSensor: Successfully subscribed to /thermal/image" << std::endl;
  }

  std::cout << "RadiometricThermalSensor: Plugin configured" << std::endl;
  std::cout << "  Save path: " << this->savePath << std::endl;
  std::cout << "  Min temp: " << this->minTemp << "K" << std::endl;
  std::cout << "  Max temp: " << this->maxTemp << "K" << std::endl;
  std::cout << "  Resolution: " << this->resolution << "K" << std::endl;

  this->initialized = true;
}

void RadiometricThermalSensor::PostUpdate(const UpdateInfo &/*_info*/,
                                          const EntityComponentManager &/*_ecm*/)
{
  // This is called every simulation step
  // We don't need to do anything here, just keeping the plugin alive
}

void RadiometricThermalSensor::OnThermalImage(const msgs::Image &_msg)
{
  if (!this->initialized)
  {
    std::cerr << "RadiometricThermalSensor: Received image but not initialized!" << std::endl;
    return;
  }

  std::cout << "RadiometricThermalSensor: Received frame " << this->frameCounter << std::endl;

  unsigned int width = _msg.width();
  unsigned int height = _msg.height();
  unsigned int pixelCount = width * height;

  std::cout << "  Image size: " << width << "x" << height << std::endl;
  std::cout << "  Pixel format: " << _msg.pixel_format_type() << std::endl;

  // Make a complete copy of the data
  std::vector<uint16_t> thermalData(pixelCount);
  
  const std::string& imageData = _msg.data();
  size_t expectedBytes = pixelCount * sizeof(uint16_t);
  
  std::cout << "  Data size: " << imageData.size() << " bytes (expected: " 
            << expectedBytes << ")" << std::endl;

  if (imageData.size() != expectedBytes)
  {
    std::cerr << "  ERROR: Data size mismatch!" << std::endl;
    this->frameCounter++;
    return;
  }

  // Copy the data
  std::memcpy(thermalData.data(), imageData.data(), expectedBytes);

  // Calculate statistics
  uint16_t minVal = 65535;
  uint16_t maxVal = 0;
  uint64_t sum = 0;
  
  for (unsigned int i = 0; i < pixelCount; ++i)
  {
    if (thermalData[i] < minVal) minVal = thermalData[i];
    if (thermalData[i] > maxVal) maxVal = thermalData[i];
    sum += thermalData[i];
  }
  
  float avgVal = static_cast<float>(sum) / pixelCount;
  
  std::cout << "  Raw uint16 range: [" << minVal << ", " << maxVal << "]" << std::endl;
  std::cout << "  Raw uint16 average: " << avgVal << std::endl;
  
  // Convert to temperature
  float minTempK = this->minTemp + (static_cast<float>(minVal) / 65535.0f) * (this->maxTemp - this->minTemp);
  float maxTempK = this->minTemp + (static_cast<float>(maxVal) / 65535.0f) * (this->maxTemp - this->minTemp);
  
  std::cout << "  Temperature range: [" << minTempK << "K, " << maxTempK << "K]" << std::endl;

  // Save data
  if (this->SaveRadiometricData(thermalData, width, height, this->frameCounter))
  {
    std::cout << "  Successfully saved frame " << this->frameCounter << std::endl;
  }
  else
  {
    std::cerr << "  ERROR: Failed to save frame " << this->frameCounter << std::endl;
  }

  this->frameCounter++;
}

bool RadiometricThermalSensor::SaveRadiometricData(const std::vector<uint16_t> &_data,
                                                   unsigned int _width,
                                                   unsigned int _height,
                                                   uint64_t _frameNum)
{
  std::string filename = this->savePath + "/thermal_" +
                         std::to_string(_frameNum) + ".raw";

  FILE *file = fopen(filename.c_str(), "wb");
  if (!file)
  {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return false;
  }

  // Convert to float32
  size_t pixelCount = _width * _height;
  std::vector<float> floatData(pixelCount);
  float tempRange = this->maxTemp - this->minTemp;
  
  for (size_t i = 0; i < pixelCount; ++i)
  {
    // Normalize uint16 (0-65535) to temperature range
    float normalized = static_cast<float>(_data[i]) / 65535.0f;
    floatData[i] = this->minTemp + (normalized * tempRange);
  }

  // Write header (width, height)
  fwrite(&_width, sizeof(unsigned int), 1, file);
  fwrite(&_height, sizeof(unsigned int), 1, file);

  // Write float32 data
  size_t written = fwrite(floatData.data(), sizeof(float), pixelCount, file);
  fclose(file);

  return (written == pixelCount);
}

}
}
}

GZ_ADD_PLUGIN(gz::sim::systems::RadiometricThermalSensor,
              gz::sim::System,
              gz::sim::ISystemConfigure,
              gz::sim::ISystemPostUpdate)

GZ_ADD_PLUGIN_ALIAS(gz::sim::systems::RadiometricThermalSensor,
                    "gz::sim::systems::RadiometricThermalSensor")