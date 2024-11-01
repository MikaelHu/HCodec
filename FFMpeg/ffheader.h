#pragma once

// ffmpeg headers
extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/frame.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavutil/timestamp.h"
};



// ffmpeg version
#define CHECK_LIBAVFORMAT   (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,56,100))
#define CHECK_LIBAVCODEC    (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,64,100))
#define CHECK_LIBAVUTIL     (LIBAVUTIL_VERSION_INT < AV_VERSION_INT(55,34,100))
#define CHECK_LIBSWSCALE    (LIBSWSCALE_VERSION_INT < AV_VERSION_INT(4,2,100))
#if CHECK_LIBAVFORMAT || CHECK_LIBAVCODEC || CHECK_LIBAVUTIL || CHECK_LIBSWSCALE
#error "The version of avformat/avcodec/avutil/swscale is low!"
#endif

// for windows dll
#ifdef FF_DLL 
#ifdef DLL_FILE
#   define FF_EXPORT _declspec(dllexport)
#else
#   define FF_EXPORT _declspec(dllimport)
#endif
#else
#   define FF_EXPORT
#endif

// misc headers
#include <string>