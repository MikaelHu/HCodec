#pragma once
#include "ffparam.h"


// for video pixel format
typedef struct Pix_Fmt {
	enum eFFPixelFormat_t e_pix_fmt;
	enum AVPixelFormat av_pix_fmt;
	int bpp;
	const char *name;
}Pix_Fmt_t;

const Pix_Fmt_t v_pix_fmts[] = {
	{ eFF_PIX_FMT_NONE, AV_PIX_FMT_NONE, 0, "" },

	{ eFF_PIX_FMT_YUV420P,  AV_PIX_FMT_YUV420P,  12, "YUV420P" },
	{ eFF_PIX_FMT_RGB24, AV_PIX_FMT_RGB24,    24, "RGB24" },
	{ eFF_PIX_FMT_BGR24, AV_PIX_FMT_BGR24,    24, "BGR24" },
	{ eFF_PIX_FMT_NV21,  AV_PIX_FMT_NV21,     12, "NV21" },
	{ eFF_PIX_FMT_NV12,  AV_PIX_FMT_NV12,     12, "NV12" },
	{ eFF_PIX_FMT_YUYV422,  AV_PIX_FMT_YUYV422,  16, "YUYV422" },
	{ eFF_PIX_FMT_YUV422P,  AV_PIX_FMT_YUV422P,  16, "YUV422P" },
	{ eFF_PIX_FMT_YUV444P,  AV_PIX_FMT_YUV444P,  24, "YUV444P" },
	{ eFF_PIX_FMT_YUV410P,  AV_PIX_FMT_YUV410P,  9, "YUV410P" },
	{ eFF_PIX_FMT_YUV411P,  AV_PIX_FMT_YUV411P,  12, "YUV411P" },
	{ eFF_PIX_FMT_BGR8,  AV_PIX_FMT_BGR8,  8, "BGR8" },
	{ eFF_PIX_FMT_BGR4,  AV_PIX_FMT_BGR4,  4, "BGR4" },
	{ eFF_PIX_FMT_RGB8,  AV_PIX_FMT_RGB8,  8, "RGB8" },
	{ eFF_PIX_FMT_RGB4,  AV_PIX_FMT_RGB4,  4, "RGB4" },
	{ eFF_PIX_FMT_GRAY8,  AV_PIX_FMT_GRAY8,  8, "GRAY8" },
	{ eFF_PIX_FMT_ARGB,  AV_PIX_FMT_ARGB,  32, "ARGB" },
	{ eFF_PIX_FMT_RGBA,  AV_PIX_FMT_RGBA,  32, "RGBA" },
	{ eFF_PIX_FMT_ABGR,  AV_PIX_FMT_ABGR,  32, "ABGR" },
	{ eFF_PIX_FMT_BGRA,  AV_PIX_FMT_BGRA,  32, "BGRA" },
};

// for video codec id
typedef struct Codec_ID {
	enum eFFCodecID_t codec_id;
	enum AVCodecID av_codec_id;
	const char *fourcc;
}Codec_ID_t;

const Codec_ID_t av_codec_ids[] = {
	// audio
	{ eFF_CODEC_ID_FIRST_AUDIO, AV_CODEC_ID_FIRST_AUDIO, "first_audio" },
	{ eFF_CODEC_ID_MP2,  AV_CODEC_ID_MP2,  "mp2" },
	{ eFF_CODEC_ID_MP3,  AV_CODEC_ID_MP3,  "mp3" },
	{ eFF_CODEC_ID_AAC,  AV_CODEC_ID_AAC,  "aac" },
	{ eFF_CODEC_ID_AAC_LATM,  AV_CODEC_ID_AAC_LATM,  "aac_latm" },
	{ eFF_CODEC_ID_AC3,  AV_CODEC_ID_AC3,  "ac3" },
	{ eFF_CODEC_ID_FLAC, AV_CODEC_ID_FLAC, "flac" },
	{ eFF_CODEC_ID_WMAV1, AV_CODEC_ID_WMAV1, "wmav1" },
	{ eFF_CODEC_ID_WMAV2, AV_CODEC_ID_WMAV2, "wmav2" },
	{ eFF_CODEC_ID_OPUS, AV_CODEC_ID_OPUS, "opus" },
	{ eFF_CODEC_ID_ALAC, AV_CODEC_ID_ALAC, "alac" },
	//video
	{ eFF_CODEC_ID_FLV1,  AV_CODEC_ID_FLV1,  "flv1" },
	{ eFF_CODEC_ID_MJPEG,  AV_CODEC_ID_MJPEG,  "mjpeg" },
	{ eFF_CODEC_ID_MPEG4,  AV_CODEC_ID_MPEG4,  "mpeg4" },
	{ eFF_CODEC_ID_VP8,  AV_CODEC_ID_VP8,  "vp8" },
	{ eFF_CODEC_ID_H263,  AV_CODEC_ID_H263,  "h263" },
	{ eFF_CODEC_ID_H264, AV_CODEC_ID_H264, "h264" },
	{ eFF_CODEC_ID_H265,  AV_CODEC_ID_H265,  "h265" },
	{ eFF_CODEC_ID_JPEG2000,  AV_CODEC_ID_JPEG2000,  "jpeg2000" },
	{ eFF_CODEC_ID_WMV3,  AV_CODEC_ID_WMV3,  "wmv3" },
};

// for audio sample format
typedef struct Sample_Fmt {
	enum eFFSampleFormat_t e_sample_fmt;
	enum AVSampleFormat av_sample_fmt;
	const char *fmt_be, *fmt_le;
} Sample_Fmt_t;

const Sample_Fmt_t a_sample_fmts[] = {
	{ eFF_SAMPLE_FMT_NONE, AV_SAMPLE_FMT_NONE, "", "" },

	{ eFF_SAMPLE_FMT_U8,  AV_SAMPLE_FMT_U8,  "u8",    "u8" },
	{ eFF_SAMPLE_FMT_S16, AV_SAMPLE_FMT_S16, "s16be", "s16le" },
	{ eFF_SAMPLE_FMT_S32, AV_SAMPLE_FMT_S32, "s32be", "s32le" },
	{ eFF_SAMPLE_FMT_S64, AV_SAMPLE_FMT_S64, "s64be", "s64le" },
	{ eFF_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
	{ eFF_SAMPLE_FMT_DBL, AV_SAMPLE_FMT_DBL, "f64be", "f64le" },

	{ eFF_SAMPLE_FMT_U8P,  AV_SAMPLE_FMT_U8P,  "u8p",    "u8p" },
	{ eFF_SAMPLE_FMT_S16P, AV_SAMPLE_FMT_S16P, "s16pbe", "s16ple" },
	{ eFF_SAMPLE_FMT_S32P, AV_SAMPLE_FMT_S32P, "s32pbe", "s32ple" },
	{ eFF_SAMPLE_FMT_S64P, AV_SAMPLE_FMT_S64P, "s64pbe", "s64ple" },
	{ eFF_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_FLTP, "f32pbe", "f32ple" },
	{ eFF_SAMPLE_FMT_DBLP, AV_SAMPLE_FMT_DBLP, "f64pbe", "f64ple" },
};

// for audio channel layout
typedef struct Channel_Layout{
	enum eFFChannelLayout_t ch_layout;
	int av_ch_layout;
	const char *layout;
}Channel_Layout_t;

const Channel_Layout_t a_channel_layouts[] = {
	{ eFF_CH_LAYOUT_MONO, AV_CH_LAYOUT_MONO, "mono" },
	{ eFF_CH_LAYOUT_STEREO, AV_CH_LAYOUT_STEREO, "stereo" },
	{ eFF_CH_LAYOUT_2POINT1, AV_CH_LAYOUT_2POINT1, "2point1" },
	{ eFF_CH_LAYOUT_SURROUND, AV_CH_LAYOUT_SURROUND, "srround" },
	{ eFF_CH_LAYOUT_3POINT1, AV_CH_LAYOUT_3POINT1, "3point1" },
	{ eFF_CH_LAYOUT_QUAD, AV_CH_LAYOUT_QUAD, "quad" },
	{ eFF_CH_LAYOUT_5POINT0, AV_CH_LAYOUT_5POINT0, "5point0" },
	{ eFF_CH_LAYOUT_6POINT0, AV_CH_LAYOUT_6POINT0, "6point0" },
};

class FFCodec {
public:
	FFCodec() = default;
	FFCodec(eFFMedia_t etype);
	~FFCodec();

	// get an AV pixel format
	AVPixelFormat get_AV_pixel_format(eFFPixelFormat_t pix_fmt);
	// get a FF pixel format
	eFFPixelFormat_t get_FF_pixel_format(AVPixelFormat pix_fmt);
	// get an AV codec id
	AVCodecID get_AV_codecID(eFFCodecID_t codec_id);
	// get a FF codec id
	eFFCodecID_t get_FF_codecID(AVCodecID codec_id);
	// get an AV sample format
	AVSampleFormat get_AV_sample_format(eFFSampleFormat_t fmt);
	// get a FF sample format
	eFFSampleFormat_t get_FF_sample_format(AVSampleFormat fmt);

	// for audio codec
	// check if a given sample format is supported by the encoder 
	int check_sample_fmt(enum AVSampleFormat sample_fmt);
	// pick the first one of supported sample fmts
	AVSampleFormat select_sample_fmt();
	// check if a given sample rate is supported by the encoder 
	int check_sample_rate(int sample_rate);
	// get the highest one of supported sample rates 
	int get_sample_rate();

	// check if a given channel layout is supported by the encoder 
	int check_channel_layout(const uint64_t av_channel_layout);
	// get layout with the highest channel count 
	int get_channel_layout();
	// get av audio channel layout
	int get_channel_layout(enum eFFChannelLayout_t ch_layout);

	// get layout with the profile 
	int get_profile();

	// for video codec
	// check if a given pixel format is supported by the encoder 
	int check_pix_fmt(enum AVPixelFormat pix_fmt);
	// get the first one of supported pix_fmt 
	AVPixelFormat get_pix_fmt();

protected:
	int open();
	int close();

public:
	eFFMedia_t			eFFmedia_t_{ eFF_MEDIA_NONE };
	AVFormatContext		*infmtCtx_{ nullptr };
	AVFormatContext		*outfmtCtx_{ nullptr };
	AVStream			*avStream_{ nullptr };
	AVCodec				*codec_{ nullptr };
    AVCodecContext		*avctx_{ nullptr };
    AVFrame				*frame_{ nullptr };		// for pre-allocated buffer
    AVFrame				*frame_t_{ nullptr };	// for self-allocated buffer
    AVPacket			*avpkt_;
    SwsContext			*swsctx_{ nullptr };
	SwrContext			*swrctx_{ nullptr };
};