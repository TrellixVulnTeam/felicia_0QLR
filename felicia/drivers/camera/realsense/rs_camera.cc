#include "felicia/drivers/camera/realsense/rs_camera.h"

#include "third_party/chromium/base/strings/strcat.h"

#include "felicia/drivers/camera/camera_errors.h"
#include "felicia/drivers/camera/realsense/rs_pixel_format.h"

namespace felicia {

RsCamera::RsCamera(const CameraDescriptor& camera_descriptor)
    : DepthCameraInterface(camera_descriptor) {}

RsCamera::~RsCamera() = default;

Status RsCamera::Init() {
  if (!camera_state_.IsStopped()) {
    return camera_state_.InvalidStateError();
  }

  Status s = RsCamera::CreateDevice(camera_descriptor_, &device_);
  if (!s.ok()) return s;

  std::vector<::rs2::sensor> sensors = device_.query_sensors();
  for (auto&& sensor : sensors) {
    std::string module_name = sensor.get_info(RS2_CAMERA_INFO_NAME);
    if ("Stereo Module" == module_name) {
      sensors_[DEPTH] = sensor;
      sensors_[INFRA1] = sensor;
      sensors_[INFRA2] = sensor;
    } else if ("Coded-Light Depth Sensor" == module_name) {
      sensors_[DEPTH] = sensor;
      sensors_[INFRA1] = sensor;
    } else if ("RGB Camera" == module_name) {
      sensors_[COLOR] = sensor;
    } else if ("Wide FOV Camera" == module_name) {
      sensors_[FISHEYE] = sensor;
      LOG(ERROR) << "Not Implemented yet for module : " << module_name;
    } else if ("Motion Module" == module_name) {
      sensors_[GYRO] = sensor;
      sensors_[ACCEL] = sensor;
      LOG(ERROR) << "Not Implemented yet for module : " << module_name;
    } else if ("Tracking Module" == module_name) {
      sensors_[GYRO] = sensor;
      sensors_[ACCEL] = sensor;
      sensors_[POSE] = sensor;
      sensors_[FISHEYE1] = sensor;
      sensors_[FISHEYE2] = sensor;
      LOG(ERROR) << "Not Implemented yet for module : " << module_name;
    }
  }

  camera_state_.ToInitialized();

  return CreateCapabilityMap(device_, &capability_map_);
}

Status RsCamera::Start(const CameraFormat& requested_color_format,
                       const CameraFormat& requested_depth_format,
                       CameraFrameCallback camera_frame_callback,
                       CameraFrameCallback depth_frame_callback,
                       StatusCallback status_callback) {
  if (!camera_state_.IsInitialized()) {
    return camera_state_.InvalidStateError();
  }

  std::function<void(rs2::frame)> frame_callback_function =
      [this](rs2::frame frame) { OnFrame(frame); };

  for (auto& sensor : sensors_) {
    try {
      if (sensor.first == COLOR || sensor.first == DEPTH) {
        const RsCapability& found_capability = GetBestMatchedCapability(
            requested_color_format, capability_map_[sensor.first]);
        sensor.second.open(
            sensor.second.get_stream_profiles()[found_capability.stream_index]);
        sensor.second.start(frame_callback_function);
      }
    } catch (::rs2::error e) {
      return Status(error::UNAVAILABLE, e.what());
    }
  }

  camera_state_.ToStarted();

  return Status::OK();
}

Status RsCamera::Start(const CameraFormat& requested_color_format,
                       const CameraFormat& requested_depth_format,
                       CameraFrameCallback synched_frame_callback,
                       StatusCallback status_callback) {
  if (!camera_state_.IsInitialized()) {
    return camera_state_.InvalidStateError();
  }

  std::function<void(::rs2::frame)> frame_callback_function = syncer_;
  auto frame_callback_inner = [this](::rs2::frame frame) { OnFrame(frame); };
  syncer_.start(frame_callback_inner);

  for (auto& sensor : sensors_) {
    try {
      if (sensor.first == COLOR || sensor.first == DEPTH) {
        const RsCapability& found_capability = GetBestMatchedCapability(
            requested_color_format, capability_map_[sensor.first]);
        sensor.second.open(
            sensor.second.get_stream_profiles()[found_capability.stream_index]);
        sensor.second.start(frame_callback_function);
      }
    } catch (::rs2::error e) {
      return Status(error::UNAVAILABLE, e.what());
    }
  }

  camera_state_.ToStarted();

  return Status::OK();
}

Status RsCamera::Stop() {
  for (auto& sensor : sensors_) {
    if (sensor.first == COLOR || sensor.first == DEPTH) {
      try {
        sensor.second.stop();
        sensor.second.close();
      } catch (::rs2::error e) {
        return Status(error::UNAVAILABLE, e.what());
      }
    }
  }

  return Status::OK();
}

void RsCamera::OnFrame(::rs2::frame frame) { LOG(INFO) << "OnFrame"; }

// static
Status RsCamera::CreateDevice(const CameraDescriptor& camera_descriptor,
                              ::rs2::device* device) {
  ::rs2::context context;
  auto list = context.query_devices();
  for (auto&& dev : list) {
    const char* physical_port = dev.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT);
    if (physical_port == camera_descriptor.device_id()) {
      *device = dev;
      return Status::OK();
    }
  }

  return Status(error::NOT_FOUND,
                ::base::StrCat({"No mathcing device with ",
                                camera_descriptor.ToString()}));
}

// static
Status RsCamera::CreateCapabilityMap(::rs2::device device,
                                     RsCapabilityMap* rs_capability_map) {
  std::vector<::rs2::sensor> sensors = device.query_sensors();
  for (auto&& sensor : sensors) {
    std::vector<::rs2::stream_profile> profiles = sensor.get_stream_profiles();
    for (size_t i = 0; i < profiles.size(); ++i) {
      ::rs2::stream_profile profile = profiles[i];
      if (profile.is<::rs2::video_stream_profile>()) {
        auto video_profile = profile.as<rs2::video_stream_profile>();

        if (video_profile.stream_type() == RS2_STREAM_INFRARED ||
            video_profile.stream_type() == RS2_STREAM_FISHEYE) {
          LOG(ERROR) << "Not supported yet for the stream type : "
                     << rs2_stream_to_string(video_profile.stream_type());
          continue;
        }

        RsStreamInfo steram_info{video_profile.stream_type(),
                                 video_profile.stream_index()};
        if (rs_capability_map->find(steram_info) == rs_capability_map->end()) {
          (*rs_capability_map)[steram_info] = RsCapabilityList{};
        }

        (*rs_capability_map)[steram_info].emplace_back(
            i, CameraFormat{video_profile.width(), video_profile.height(),
                            FromRs2Format(video_profile.format()),
                            static_cast<float>(video_profile.fps())});
      }
    }
  }

  if ((*rs_capability_map)[COLOR].empty() ||
      (*rs_capability_map)[DEPTH].empty()) {
    return errors::NoVideoCapbility();
  }

  return Status::OK();
}

}  // namespace felicia