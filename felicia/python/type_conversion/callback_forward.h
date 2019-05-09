#ifndef FELICIA_PYTHON_TYPE_CONVERSION_CALLBACK_FORWARD_H_
#define FELICIA_PYTHON_TYPE_CONVERSION_CALLBACK_FORWARD_H_

#include "felicia/core/lib/error/status.h"

namespace felicia {

template <typename Signature>
class PyCallback;

using PyStatusCallback = PyCallback<void(const Status&)>;
using PyClosure = PyCallback<void()>;

}  // namespace felicia

#endif  // FELICIA_PYTHON_TYPE_CONVERSION_CALLBACK_FORWARD_H_