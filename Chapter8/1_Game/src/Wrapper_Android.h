#pragma once

#ifndef LOGI
#  include <android/log.h>
#  define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Game1", __VA_ARGS__))
#endif

