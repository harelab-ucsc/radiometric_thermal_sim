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

#include <cmath>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>

namespace gz {
namespace sim {
namespace systems {

// Companion to RadiometricThermalSensor: saves the depth camera's frames as
// .raw files in the same header+float32 layout, so the same Python readers
// work for both. Unlike the thermal topic (uint16 counts that need a
// resolution factor to decode), the depth camera publishes R_FLOAT32 pixels
// that are already metric meters (z-depth, not Euclidean range), so there's
// no conversion step here at all -- just copy and save.
class RadiometricDepthSensor : public System,
                               public ISystemConfigure,
                               public ISystemPostUpdate {
public:
  RadiometricDepthSensor();
  ~RadiometricDepthSensor() override;

  void Configure(const Entity &_entity,
                 const std::shared_ptr<const sdf::Element> &_sdf,
                 EntityComponentManager &_ecm,
                 EventManager &_eventMgr) override;

  void PostUpdate(const UpdateInfo &_info,
                  const EntityComponentManager &_ecm) override;

private:
  void OnDepthImage(const msgs::Image &_msg);
  bool SaveDepthData(const std::vector<float> &_data,
                     unsigned int _width,
                     unsigned int _height,
                     uint64_t _frameNum);

  transport::Node node;
  std::string savePath;
  uint64_t frameCounter = 0;
  bool initialized = false;
};

RadiometricDepthSensor::RadiometricDepthSensor()
{
  std::cout << "RadiometricDepthSensor: Constructor called" << std::endl;
}

RadiometricDepthSensor::~RadiometricDepthSensor()
{
  std::cout << "RadiometricDepthSensor: Destructor called, saved "
            << this->frameCounter << " frames" << std::endl;
}

void RadiometricDepthSensor::Configure(const Entity &/*_entity*/,
                                       const std::shared_ptr<const sdf::Element> &_sdf,
                                       EntityComponentManager &/*_ecm*/,
                                       EventManager &/*_eventMgr*/)
{
  std::cout << "RadiometricDepthSensor: Configure called" << std::endl;

  // Read configuration parameters
  if (_sdf->HasElement("save_path"))
  {
    this->savePath = _sdf->Get<std::string>("save_path");
  }
  else
  {
    this->savePath = "./depth_output";
  }

  // Create save directory
  system(("mkdir -p " + this->savePath).c_str());

  // Subscribe to depth camera topic
  if (!this->node.Subscribe("/depth/image",
                            &RadiometricDepthSensor::OnDepthImage, this))
  {
    std::cerr << "RadiometricDepthSensor: Failed to subscribe to /depth/image" << std::endl;
  }
  else
  {
    std::cout << "RadiometricDepthSensor: Successfully subscribed to /depth/image" << std::endl;
  }

  std::cout << "RadiometricDepthSensor: Plugin configured" << std::endl;
  std::cout << "  Save path: " << this->savePath << std::endl;

  this->initialized = true;
}

void RadiometricDepthSensor::PostUpdate(const UpdateInfo &/*_info*/,
                                        const EntityComponentManager &/*_ecm*/)
{
  // This is called every simulation step
  // We don't need to do anything here, just keeping the plugin alive
}

void RadiometricDepthSensor::OnDepthImage(const msgs::Image &_msg)
{
  if (!this->initialized)
  {
    std::cerr << "RadiometricDepthSensor: Received image but not initialized!" << std::endl;
    return;
  }

  std::cout << "RadiometricDepthSensor: Received frame " << this->frameCounter << std::endl;

  unsigned int width = _msg.width();
  unsigned int height = _msg.height();
  unsigned int pixelCount = width * height;

  std::cout << "  Image size: " << width << "x" << height << std::endl;
  std::cout << "  Pixel format: " << _msg.pixel_format_type() << std::endl;

  if (_msg.pixel_format_type() != msgs::PixelFormatType::R_FLOAT32)
  {
    std::cerr << "  ERROR: Expected R_FLOAT32 pixel format, got "
              << _msg.pixel_format_type() << std::endl;
    this->frameCounter++;
    return;
  }

  // Make a complete copy of the data
  std::vector<float> depthData(pixelCount);

  const std::string& imageData = _msg.data();
  size_t expectedBytes = pixelCount * sizeof(float);

  std::cout << "  Data size: " << imageData.size() << " bytes (expected: "
            << expectedBytes << ")" << std::endl;

  if (imageData.size() != expectedBytes)
  {
    std::cerr << "  ERROR: Data size mismatch!" << std::endl;
    this->frameCounter++;
    return;
  }

  // Copy the data -- already float32 meters, inf means no hit within far clip
  std::memcpy(depthData.data(), imageData.data(), expectedBytes);

  // Calculate statistics over the finite pixels
  float minVal = 0.0f;
  float maxVal = 0.0f;
  double sum = 0.0;
  unsigned int finiteCount = 0;

  for (unsigned int i = 0; i < pixelCount; ++i)
  {
    if (!std::isfinite(depthData[i]))
      continue;
    if (finiteCount == 0 || depthData[i] < minVal) minVal = depthData[i];
    if (finiteCount == 0 || depthData[i] > maxVal) maxVal = depthData[i];
    sum += depthData[i];
    finiteCount++;
  }

  if (finiteCount > 0)
  {
    std::cout << "  Depth range: [" << minVal << "m, " << maxVal << "m], "
              << finiteCount << "/" << pixelCount << " finite pixels, average "
              << (sum / finiteCount) << "m" << std::endl;
  }
  else
  {
    std::cerr << "  WARNING: No finite depth values in this frame" << std::endl;
  }

  // Save data
  if (this->SaveDepthData(depthData, width, height, this->frameCounter))
  {
    std::cout << "  Successfully saved frame " << this->frameCounter << std::endl;
  }
  else
  {
    std::cerr << "  ERROR: Failed to save frame " << this->frameCounter << std::endl;
  }

  this->frameCounter++;
}

bool RadiometricDepthSensor::SaveDepthData(const std::vector<float> &_data,
                                           unsigned int _width,
                                           unsigned int _height,
                                           uint64_t _frameNum)
{
  std::string filename = this->savePath + "/depth_" +
                         std::to_string(_frameNum) + ".raw";

  FILE *file = fopen(filename.c_str(), "wb");
  if (!file)
  {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return false;
  }

  // Same layout as the radiometric thermal files: width/height header,
  // then float32 payload
  fwrite(&_width, sizeof(unsigned int), 1, file);
  fwrite(&_height, sizeof(unsigned int), 1, file);

  size_t pixelCount = _width * _height;
  size_t written = fwrite(_data.data(), sizeof(float), pixelCount, file);
  fclose(file);

  return (written == pixelCount);
}

}
}
}

GZ_ADD_PLUGIN(gz::sim::systems::RadiometricDepthSensor,
              gz::sim::System,
              gz::sim::ISystemConfigure,
              gz::sim::ISystemPostUpdate)

GZ_ADD_PLUGIN_ALIAS(gz::sim::systems::RadiometricDepthSensor,
                    "gz::sim::systems::RadiometricDepthSensor")
