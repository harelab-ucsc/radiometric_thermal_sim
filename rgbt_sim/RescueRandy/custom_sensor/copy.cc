#include "RadiometricThermalSensor.hh"
#include <gz/plugin/Register.hh>
#include <gz/common/Console.hh>
#include <gz/common/Image.hh>
#include <filesystem>
#include <gz/common/Image.hh>
#include <fstream>

using namespace custom;

//////////////////////////////////////////////////
void RadiometricThermalSensor::Configure(const gz::sim::Entity &,
                                         const std::shared_ptr<const sdf::Element> &_sdf,
                                         gz::sim::EntityComponentManager &,
                                         gz::sim::EventManager &)
{
  // Pull optional SDF params:
  // <topic>/thermal/radiometric</topic>
  // <output_dir>/tmp/thermal_radiometric_output</output_dir>
  // <min_temp>233.0</min_temp>   <max_temp>373.0</max_temp>  (Kelvin)
  if (_sdf)
  {
    if (_sdf->HasElement("topic"))
      this->topic_ = _sdf->Get<std::string>("topic");
    if (_sdf->HasElement("output_dir"))
      this->outDir_ = _sdf->Get<std::string>("output_dir");
    if (_sdf->HasElement("min_temp"))
      this->minK_ = _sdf->Get<double>("min_temp");
    if (_sdf->HasElement("max_temp"))
      this->maxK_ = _sdf->Get<double>("max_temp");
  }

  std::error_code ec;
  std::filesystem::create_directories(this->outDir_, ec);

  gzmsg << "[RadiometricThermalSensor] GZ Sim 9 plugin configured.\n"
        << "  topic      : " << this->topic_ << "\n"
        << "  output_dir : " << this->outDir_ << "\n"
        << "  minK/maxK  : " << this->minK_ << " / " << this->maxK_ << std::endl;

  // Subscribe once
  if (!this->subscribed_)
  {
    this->node_.Subscribe(this->topic_,
      &RadiometricThermalSensor::OnRadiometricImage, this);
    this->subscribed_ = true;
    gzmsg << "[RadiometricThermalSensor] Subscribed to " << this->topic_ << std::endl;
  }
}

//////////////////////////////////////////////////
void RadiometricThermalSensor::PreUpdate(const gz::sim::UpdateInfo &,
                                         gz::sim::EntityComponentManager &)
{
  // Nothing required per tick for now.
}

//////////////////////////////////////////////////

void RadiometricThermalSensor::OnRadiometricImage(const gz::msgs::Image &_msg)
{
  const unsigned int w = _msg.width();
  const unsigned int h = _msg.height();
  const size_t N = static_cast<size_t>(w) * static_cast<size_t>(h);

  auto fmt = _msg.pixel_format_type();
  std::vector<float> kelvin(N);

  if (fmt == gz::msgs::PixelFormatType::L_INT16)
  {
    const int16_t *raw = reinterpret_cast<const int16_t*>(_msg.data().data());
    constexpr double denom = 65535.0;
    for (size_t i = 0; i < N; ++i)
    {
      double norm01 = (static_cast<double>(raw[i]) + 32768.0) / denom;
      kelvin[i] = static_cast<float>(
          this->minK_ + norm01 * (this->maxK_ - this->minK_));
    }

    std::vector<uint8_t> preview8(N);
    for (size_t i = 0; i < N; ++i)
    {
      // simple linear scale from -32768..32767 → 0..255
      preview8[i] = static_cast<uint8_t>(
          (static_cast<double>(raw[i]) + 32768.0) / 65535.0 * 255.0);
    }

    gz::common::Image preview;
    preview.SetFromData(reinterpret_cast<const unsigned char*>(preview8.data()),
                        w, h, gz::common::Image::PixelFormatType::L_INT8);
    preview.SavePNG(this->outDir_ + "/radiometric_preview.png");

  }
  else
  {
    gzerr << "[RadiometricThermalSensor] Unsupported pixel_format_type="
          << fmt << " (expect L_INT16). Skipping frame.\n";
    return;
  }

  // Save float32 radiometric values to .bin
  const auto sec  = _msg.header().stamp().sec();
  const auto nsec = _msg.header().stamp().nsec();
  const std::string out = this->outDir_ + "/radiometric_" +
                          std::to_string(sec) + "_" +
                          std::to_string(nsec) + ".bin";

  std::ofstream f(out, std::ios::binary);
  f.write(reinterpret_cast<const char*>(kelvin.data()), kelvin.size() * sizeof(float));
  f.close();

  gzmsg << "[RadiometricThermalSensor] Wrote float32 radiometric data: "
        << out << " (" << w << "x" << h << ")\n";
}





//////////////////////////////////////////////////
GZ_ADD_PLUGIN(custom::RadiometricThermalSensor,
              gz::sim::System,
              custom::RadiometricThermalSensor::ISystemConfigure,
              custom::RadiometricThermalSensor::ISystemPreUpdate)

