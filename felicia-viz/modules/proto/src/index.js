import FeliciaProtoRoot from './felicia-proto-root';
import { IMAGE_WITH_BOUNDING_BOXES_MESSAGE } from './messages/bounding-box';
import { CAMERA_FORMAT_MESSAGE } from './messages/camera-format-message';
import { CAMERA_FRAME_MESSAGE } from './messages/camera-frame-message';
import { DEPTH_CAMERA_FRAME_MESSAGE } from './messages/depth-camera-frame-message';
import {
  SIZEI_MESSAGE,
  SIZEF_MESSAGE,
  SIZED_MESSAGE,
  QUATERNIONF_MESSAGE,
  QUATERNIOND_MESSAGE,
  VECTORI_MESSAGE,
  VECTORF_MESSAGE,
  VECTORD_MESSAGE,
  VECTOR3I_MESSAGE,
  VECTOR3F_MESSAGE,
  VECTOR3D_MESSAGE,
  POINTI_MESSAGE,
  POINTF_MESSAGE,
  POINTD_MESSAGE,
  POINT3I_MESSAGE,
  POINT3F_MESSAGE,
  POINT3D_MESSAGE,
  POSEF_MESSAGE,
  POSED_MESSAGE,
  POSE3F_MESSAGE,
  POSE3D_MESSAGE,
  POSEF_WITH_TIMESTAMP_MESSAGE,
  POSED_WITH_TIMESTAMP_MESSAGE,
  POSE3F_WITH_TIMESTAMP_MESSAGE,
  POSE3D_WITH_TIMESTAMP_MESSAGE,
  RECTI_MESSAGE,
  RECTF_MESSAGE,
  RECTD_MESSAGE,
  QUADI_MESSAGE,
  QUADF_MESSAGE,
  QUADD_MESSAGE,
} from './messages/geometry';
import { IMAGE_WITH_HUMANS_MESSAGE } from './messages/human';
import { IMU_FRAME_MESSAGE } from './messages/imu-frame-message';
import { COLOR3U_MESSAGE, COLOR3F_MESSAGE, COLOR4U_MESSAGE, COLOR4F_MESSAGE } from './messages/ui';
import { LIDAR_FRAME_MESSAGE } from './messages/lidar-frame-message';
import { OCCUPANCY_GRID_MAP_MESSAGE, POINTCLOUD_MESSAGE } from './messages/map-message';

export const TOPIC_INFO = 'felicia.TopicInfo';

// enums
export const ChannelDefType = FeliciaProtoRoot.lookupEnum('felicia.ChannelDef.Type');

const PROTO_TYPES = {};
PROTO_TYPES[TOPIC_INFO] = FeliciaProtoRoot.lookupType(TOPIC_INFO);
// bounding-box
PROTO_TYPES[IMAGE_WITH_BOUNDING_BOXES_MESSAGE] = FeliciaProtoRoot.lookupType(
  IMAGE_WITH_BOUNDING_BOXES_MESSAGE
);
// camera-format-message
PROTO_TYPES[CAMERA_FORMAT_MESSAGE] = FeliciaProtoRoot.lookupType(CAMERA_FORMAT_MESSAGE);
// camera-frame-message
PROTO_TYPES[CAMERA_FRAME_MESSAGE] = FeliciaProtoRoot.lookupType(CAMERA_FRAME_MESSAGE);
// depth-camera-frame-message
PROTO_TYPES[DEPTH_CAMERA_FRAME_MESSAGE] = FeliciaProtoRoot.lookupType(DEPTH_CAMERA_FRAME_MESSAGE);
// geometry
PROTO_TYPES[SIZEI_MESSAGE] = FeliciaProtoRoot.lookupType(SIZEI_MESSAGE);
PROTO_TYPES[SIZEF_MESSAGE] = FeliciaProtoRoot.lookupType(SIZEF_MESSAGE);
PROTO_TYPES[SIZED_MESSAGE] = FeliciaProtoRoot.lookupType(SIZED_MESSAGE);
PROTO_TYPES[QUATERNIONF_MESSAGE] = FeliciaProtoRoot.lookupType(QUATERNIONF_MESSAGE);
PROTO_TYPES[QUATERNIOND_MESSAGE] = FeliciaProtoRoot.lookupType(QUATERNIOND_MESSAGE);
PROTO_TYPES[VECTORI_MESSAGE] = FeliciaProtoRoot.lookupType(VECTORI_MESSAGE);
PROTO_TYPES[VECTORF_MESSAGE] = FeliciaProtoRoot.lookupType(VECTORF_MESSAGE);
PROTO_TYPES[VECTORD_MESSAGE] = FeliciaProtoRoot.lookupType(VECTORD_MESSAGE);
PROTO_TYPES[VECTOR3I_MESSAGE] = FeliciaProtoRoot.lookupType(VECTOR3I_MESSAGE);
PROTO_TYPES[VECTOR3F_MESSAGE] = FeliciaProtoRoot.lookupType(VECTOR3F_MESSAGE);
PROTO_TYPES[VECTOR3D_MESSAGE] = FeliciaProtoRoot.lookupType(VECTOR3D_MESSAGE);
PROTO_TYPES[POINTI_MESSAGE] = FeliciaProtoRoot.lookupType(POINTI_MESSAGE);
PROTO_TYPES[POINTF_MESSAGE] = FeliciaProtoRoot.lookupType(POINTF_MESSAGE);
PROTO_TYPES[POINTD_MESSAGE] = FeliciaProtoRoot.lookupType(POINTD_MESSAGE);
PROTO_TYPES[POINT3I_MESSAGE] = FeliciaProtoRoot.lookupType(POINT3I_MESSAGE);
PROTO_TYPES[POINT3F_MESSAGE] = FeliciaProtoRoot.lookupType(POINT3F_MESSAGE);
PROTO_TYPES[POINT3D_MESSAGE] = FeliciaProtoRoot.lookupType(POINT3D_MESSAGE);
PROTO_TYPES[POSEF_MESSAGE] = FeliciaProtoRoot.lookupType(POSEF_MESSAGE);
PROTO_TYPES[POSED_MESSAGE] = FeliciaProtoRoot.lookupType(POSED_MESSAGE);
PROTO_TYPES[POSE3F_MESSAGE] = FeliciaProtoRoot.lookupType(POSE3F_MESSAGE);
PROTO_TYPES[POSE3D_MESSAGE] = FeliciaProtoRoot.lookupType(POSE3D_MESSAGE);
PROTO_TYPES[POSEF_WITH_TIMESTAMP_MESSAGE] = FeliciaProtoRoot.lookupType(
  POSEF_WITH_TIMESTAMP_MESSAGE
);
PROTO_TYPES[POSED_WITH_TIMESTAMP_MESSAGE] = FeliciaProtoRoot.lookupType(
  POSED_WITH_TIMESTAMP_MESSAGE
);
PROTO_TYPES[POSE3F_WITH_TIMESTAMP_MESSAGE] = FeliciaProtoRoot.lookupType(
  POSE3F_WITH_TIMESTAMP_MESSAGE
);
PROTO_TYPES[POSE3D_WITH_TIMESTAMP_MESSAGE] = FeliciaProtoRoot.lookupType(
  POSE3D_WITH_TIMESTAMP_MESSAGE
);
PROTO_TYPES[RECTI_MESSAGE] = FeliciaProtoRoot.lookupType(RECTI_MESSAGE);
PROTO_TYPES[RECTF_MESSAGE] = FeliciaProtoRoot.lookupType(RECTF_MESSAGE);
PROTO_TYPES[RECTD_MESSAGE] = FeliciaProtoRoot.lookupType(RECTD_MESSAGE);
PROTO_TYPES[QUADI_MESSAGE] = FeliciaProtoRoot.lookupType(QUADI_MESSAGE);
PROTO_TYPES[QUADF_MESSAGE] = FeliciaProtoRoot.lookupType(QUADF_MESSAGE);
PROTO_TYPES[QUADD_MESSAGE] = FeliciaProtoRoot.lookupType(QUADD_MESSAGE);
// human
PROTO_TYPES[IMAGE_WITH_HUMANS_MESSAGE] = FeliciaProtoRoot.lookupType(IMAGE_WITH_HUMANS_MESSAGE);
// imu-frame-message
PROTO_TYPES[IMU_FRAME_MESSAGE] = FeliciaProtoRoot.lookupType(IMU_FRAME_MESSAGE);
// ui
PROTO_TYPES[COLOR3U_MESSAGE] = FeliciaProtoRoot.lookupType(COLOR3U_MESSAGE);
PROTO_TYPES[COLOR3F_MESSAGE] = FeliciaProtoRoot.lookupType(COLOR3F_MESSAGE);
PROTO_TYPES[COLOR4U_MESSAGE] = FeliciaProtoRoot.lookupType(COLOR4U_MESSAGE);
PROTO_TYPES[COLOR4F_MESSAGE] = FeliciaProtoRoot.lookupType(COLOR4F_MESSAGE);
// lidar-frame-message
PROTO_TYPES[LIDAR_FRAME_MESSAGE] = FeliciaProtoRoot.lookupType(LIDAR_FRAME_MESSAGE);
// map-message
PROTO_TYPES[OCCUPANCY_GRID_MAP_MESSAGE] = FeliciaProtoRoot.lookupType(OCCUPANCY_GRID_MAP_MESSAGE);
PROTO_TYPES[POINTCLOUD_MESSAGE] = FeliciaProtoRoot.lookupType(POINTCLOUD_MESSAGE);

export default PROTO_TYPES;
