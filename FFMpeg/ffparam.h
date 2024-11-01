#pragma once
#include "ffheader.h"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

enum eFFMedia_t {
	eFF_MEDIA_NONE = -1,
    eFF_MEDIA_VIDEO,
    eFF_MEDIA_AUDIO,

	eFF_MEDIA_NB
};

enum eFFPixelFormat_t {
    eFF_PIX_FMT_NONE = -1,
    eFF_PIX_FMT_YUV420P,
    eFF_PIX_FMT_RGB24,
    eFF_PIX_FMT_BGR24,
    eFF_PIX_FMT_NV21,
	eFF_PIX_FMT_NV12,
	eFF_PIX_FMT_YUYV422,
	eFF_PIX_FMT_YUV422P,
	eFF_PIX_FMT_YUV444P,
	eFF_PIX_FMT_YUV410P,
	eFF_PIX_FMT_YUV411P,
	eFF_PIX_FMT_BGR8,
	eFF_PIX_FMT_BGR4,
	eFF_PIX_FMT_RGB8,
	eFF_PIX_FMT_RGB4,
	eFF_PIX_FMT_GRAY8,
	eFF_PIX_FMT_ARGB,
	eFF_PIX_FMT_RGBA,
	eFF_PIX_FMT_ABGR,
	eFF_PIX_FMT_BGRA,

    eFF_PIX_FMT_NB
};

enum eFFSampleFormat_t {
    eFF_SAMPLE_FMT_NONE = -1,
    eFF_SAMPLE_FMT_U8,   // pcm u8
    eFF_SAMPLE_FMT_S16,  // pcm s16
    eFF_SAMPLE_FMT_S32,  // pcm s32
	eFF_SAMPLE_FMT_S64,	 // pcm s64
    eFF_SAMPLE_FMT_FLT,  // pcm float
    eFF_SAMPLE_FMT_DBL,  // pcm double
    // planar
    eFF_SAMPLE_FMT_U8P,
    eFF_SAMPLE_FMT_S16P,
    eFF_SAMPLE_FMT_S32P,
	eFF_SAMPLE_FMT_S64P,
    eFF_SAMPLE_FMT_FLTP,
    eFF_SAMPLE_FMT_DBLP,

    eFF_SAMPLE_FMT_NB    // Number of sample formats
};

enum eFFCodecID_t {
    eFF_CODEC_ID_NONE = -1,
	// audio
	eFF_CODEC_ID_FIRST_AUDIO,
    eFF_CODEC_ID_MP2,
	eFF_CODEC_ID_MP3,
	eFF_CODEC_ID_AAC,
	eFF_CODEC_ID_AAC_LATM,
	eFF_CODEC_ID_AC3,
	eFF_CODEC_ID_FLAC,
	eFF_CODEC_ID_WMAV1,
	eFF_CODEC_ID_WMAV2,
	eFF_CODEC_ID_OPUS,
	eFF_CODEC_ID_ALAC,
	// video
	eFF_CODEC_ID_FLV1,
	eFF_CODEC_ID_MJPEG,
	eFF_CODEC_ID_MPEG4,
    eFF_CODEC_ID_VP8,
	eFF_CODEC_ID_H263,
    eFF_CODEC_ID_H264,
	eFF_CODEC_ID_H265,
	eFF_CODEC_ID_JPEG2000,
	eFF_CODEC_ID_WMV3,

    eFF_CODEC_ID_NB
};

enum eFFChannelLayout_t {
	eFF_CH_LAYOUT_NONE = -1,
	eFF_CH_LAYOUT_MONO,
	eFF_CH_LAYOUT_STEREO,
	eFF_CH_LAYOUT_2POINT1,
	eFF_CH_LAYOUT_SURROUND,
	eFF_CH_LAYOUT_3POINT1,
	eFF_CH_LAYOUT_QUAD,
	eFF_CH_LAYOUT_5POINT0,
	eFF_CH_LAYOUT_6POINT0,

	eFF_CH_LAYOUT_NB
};


class FFAudioFormat {
public:
    FFAudioFormat() {
        reset();
    }
    FFAudioFormat(int sample_rate, eFFSampleFormat_t sample_fmt, eFFChannelLayout_t channel_layout, int channels, int bitrate, int frame_size) {
        set(sample_rate, sample_fmt, channel_layout, channels, bitrate, frame_size);
    }

    void reset() {
        set(0, eFF_SAMPLE_FMT_NONE, eFF_CH_LAYOUT_NONE, 0, 0, 0);
    }

    void set(int sample_rate, eFFSampleFormat_t sample_fmt, eFFChannelLayout_t channel_layout, int channels, int bitrate, int frame_size) {
        sample_rate_ = sample_rate;
        sample_fmt_ = sample_fmt;
        channels_ = channels;
		channel_layout_ = channel_layout;
        bitrate_ = bitrate;
		frame_size_ = frame_size;
    }

public:
    int sample_rate_{ 0 };
    int channels_{ 0 };
	eFFChannelLayout_t channel_layout_{ eFF_CH_LAYOUT_NONE };
    int bitrate_{ 0 };
	int frame_size_{ 0 };	// frame_size or nb_samples: AAC-1024 MP3-1152
    eFFSampleFormat_t sample_fmt_{ eFF_SAMPLE_FMT_NONE };
};

class FFVideoFormat {
public:
    FFVideoFormat() {
        reset();
    }
    FFVideoFormat(int width, int height, eFFPixelFormat_t pix_fmt, int bitrate, int fps) {
        set(width, height, pix_fmt, bitrate, fps);
    }

    void reset() {
        set(0, 0, eFF_PIX_FMT_NONE, 0, 0);
    }

    void set(int width, int height, eFFPixelFormat_t pix_fmt, int bitrate, int fps, int gop_size = 12, int max_b_frames = 2) {
        width_ = width;
        height_ = height;
        pix_fmt_ = pix_fmt;
        bitrate_ = bitrate;
        fps_ = fps;

		data_.gop_size_ = gop_size;			//12
		data_.max_b_frames_ = max_b_frames;	//2
    }

public:
    typedef struct CodecData {
		int gop_size_{ 0 };
        int max_b_frames_{ 0 };
    }CodecData_t;

public:
    int					width_{ 0 };
    int					height_{ 0 };
	eFFPixelFormat_t	pix_fmt_{ eFF_PIX_FMT_NONE };
    int					bitrate_{ 0 };
    int					fps_{ 0 };
	CodecData_t			data_;
};