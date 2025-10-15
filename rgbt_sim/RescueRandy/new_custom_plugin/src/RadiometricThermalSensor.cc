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

namespace gz {
namespace sim {
namespace systems {

class RadiometricThermalSensor : public System {
public:
  RadiometricThermalSensor();
  ~RadiometricThermalSensor() override;

private:
  void OnThermalImage(const msgs::Image &_msg);
  bool SaveRadiometricData(const uint16_t *_data, unsigned int _width,
                           unsigned int _height, uint64_t _frameNum);

  transport::Node node;
  std::string savePath;
  float minTemp = 253.15f;
  float maxTemp = 320.0f;
  float resolution = 3.0f;
  uint64_t frameCounter = 0;

  std::unique_ptr<uint16_t[]> tempBuffer;
  std::unique_ptr<float[]> floatBuffer;
  unsigned int lastWidth = 0;
  unsigned int lastHeight = 0;
};

RadiometricThermalSensor::RadiometricThermalSensor()
    : savePath("./thermal_radiometric"),
      minTemp(253.15f),
      maxTemp(320.0f),
      resolution(3.0f),
      frameCounter(0),
      lastWidth(0),
      lastHeight(0)
{
  // Subscribe to thermal camera topic
  this->node.Subscribe("/thermal/image",
                       &RadiometricThermalSensor::OnThermalImage, this);
  gzdbg << "Subscribed to /thermal/image" << std::endl;

  // Create save directory
  system(("mkdir -p " + this->savePath).c_str());

  gzdbg << "RadiometricThermalSensor plugin initialized" << std::endl;
  gzdbg << "  Save path: " << this->savePath << std::endl;
  gzdbg << "  Min temp: " << this->minTemp << std::endl;
  gzdbg << "  Max temp: " << this->maxTemp << std::endl;
  gzdbg << "  Resolution: " << this->resolution << std::endl;
}

RadiometricThermalSensor::~RadiometricThermalSensor()
{
}

void RadiometricThermalSensor::OnThermalImage(const msgs::Image &_msg)
{
  unsigned int width = _msg.width();
  unsigned int height = _msg.height();
  unsigned int dataSize = width * height;

  // Allocate buffers if size changed
  if (width != this->lastWidth || height != this->lastHeight)
  {
    this->tempBuffer = std::make_unique<uint16_t[]>(dataSize);
    this->floatBuffer = std::make_unique<float[]>(dataSize);
    this->lastWidth = width;
    this->lastHeight = height;
  }

  // Copy thermal data
  const uint16_t *data = reinterpret_cast<const uint16_t *>(_msg.data().c_str());
  std::memcpy(this->tempBuffer.get(), data, dataSize * sizeof(uint16_t));

  // Convert uint16_t to float32 temperature in Kelvin
  for (unsigned int i = 0; i < dataSize; ++i)
  {
    this->floatBuffer[i] = this->minTemp + (static_cast<float>(this->tempBuffer[i]) * this->resolution);
  }

  // Save data
  this->SaveRadiometricData(this->tempBuffer.get(), width, height, this->frameCounter);

  if (this->frameCounter % 30 == 0)
  {
    gzdbg << "Saved radiometric frame: " << this->frameCounter << std::endl;
  }

  this->frameCounter++;
}

bool RadiometricThermalSensor::SaveRadiometricData(const uint16_t *_data,
                                                   unsigned int _width,
                                                   unsigned int _height,
                                                   uint64_t _frameNum)
{
  // Create filename
  std::string filename = this->savePath + "/thermal_" +
                         std::to_string(_frameNum) + ".raw";

  FILE *file = fopen(filename.c_str(), "wb");
  if (!file)
  {
    gzerr << "Failed to open file: " << filename << std::endl;
    return false;
  }

  unsigned int dataSize = _width * _height * sizeof(uint16_t);

  // Convert to float32 and write
  // Map uint16 range (0-65535) to temperature range (minTemp-maxTemp)
  std::unique_ptr<float[]> floatData = std::make_unique<float[]>(_width * _height);
  float tempRange = this->maxTemp - this->minTemp;
  for (unsigned int i = 0; i < _width * _height; ++i)
  {
    float normalized = static_cast<float>(_data[i]) / 65535.0f;
    floatData[i] = this->minTemp + (normalized * tempRange);
  }

  // Write header (width, height)
  fwrite(&_width, sizeof(unsigned int), 1, file);
  fwrite(&_height, sizeof(unsigned int), 1, file);

  // Write float32 data
  size_t written = fwrite(floatData.get(), sizeof(float), _width * _height, file);
  fclose(file);

  if (written != _width * _height)
  {
    gzerr << "Failed to write all data to file" << std::endl;
    return false;
  }

  return true;
}

}
}
}

GZ_ADD_PLUGIN(gz::sim::systems::RadiometricThermalSensor,
              gz::sim::System)

GZ_ADD_PLUGIN_ALIAS(gz::sim::systems::RadiometricThermalSensor,
                    "gz::sim::systems::RadiometricThermalSensor")