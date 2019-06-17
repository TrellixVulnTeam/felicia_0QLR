#include "felicia/drivers/camera/depth_camera_interface.h"

namespace felicia {

DepthCameraInterface::DepthCameraInterface(
    const CameraDescriptor& camera_descriptor)
    : CameraInterfaceBase(camera_descriptor) {}

}  // namespace felicia