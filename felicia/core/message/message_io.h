#ifndef FELICIA_CORE_MESSAGE_MESSAGE_IO_H_
#define FELICIA_CORE_MESSAGE_MESSAGE_IO_H_

#include "felicia/core/message/message_io_error.h"

namespace felicia {

template <typename T, typename SFINAE = void>
class MessageIO;

}  // namespace felicia

#include "felicia/core/message/protobuf_message_io.h"
#include "felicia/core/message/ros_header_io.h"
#include "felicia/core/message/ros_message_io.h"
#include "felicia/core/message/serialized_message_io.h"

#endif  // FELICIA_CORE_MESSAGE_MESSAGE_IO_H_