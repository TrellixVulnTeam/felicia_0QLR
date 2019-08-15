#ifndef FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_
#define FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_

#include "third_party/chromium/base/callback.h"

#include "felicia/core/lib/base/export.h"
#include "felicia/core/lib/error/status.h"
#include "felicia/core/lib/unit/geometry/point.h"
#include "felicia/core/lib/unit/ui/color.h"
#include "felicia/drivers/pointcloud/pointcloud_frame_message.pb.h"

namespace felicia {
namespace drivers {

class EXPORT PointcloudFrame {
 public:
  PointcloudFrame();
  PointcloudFrame(size_t points_size, size_t colors_size);
  PointcloudFrame(std::vector<Point3f>&& points, std::vector<uint8_t>&& colors,
                  base::TimeDelta timestamp) noexcept;
  PointcloudFrame(PointcloudFrame&& other) noexcept;
  PointcloudFrame& operator=(PointcloudFrame&& other);

  void AddPoint(float x, float y, float z);
  void AddPoint(const Point3f& point);
  void AddPointAndColor(float x, float y, float z, uint8_t r, uint8_t g,
                        uint8_t b);

  void set_timestamp(base::TimeDelta time);
  base::TimeDelta timestamp() const;

  PointcloudFrameMessage ToPointcloudFrameMessage() const;
  Status FromPointcloudFrameMessage(const PointcloudFrameMessage& message);

 private:
  std::vector<Point3f> points_;
  std::vector<uint8_t> colors_;
  base::TimeDelta timestamp_;

  DISALLOW_COPY_AND_ASSIGN(PointcloudFrame);
};

typedef base::RepeatingCallback<void(PointcloudFrame)> PointcloudFrameCallback;

}  // namespace drivers
}  // namespace felicia

#endif  // FELICIA_DRIVERS_POINTCLOUD_POINTCLOUD_FRAME_H_