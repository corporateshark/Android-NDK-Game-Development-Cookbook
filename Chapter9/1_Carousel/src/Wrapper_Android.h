#ifndef _Wrapper_Android_h_
#define _Wrapper_Android_h_

#include "Wrapper_Callbacks.h"

#ifndef LOGI
#  include <android/log.h>
#  define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "App13", __VA_ARGS__))
#endif

#endif
