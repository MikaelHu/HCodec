#pragma once
/*File name: H264Convetor.h*/
/*Description: the definitions of a H264 convetor, which is used to convet video to H264 frame.*/
/*Author:Nico Hu*/
/*Date:2024/09/01*/

#ifdef DLL_EXPORT
#ifdef BUILD_DLL
#define HCODEC_API __declspec(dllexport)
#else
#define HCODEC_API __declspec(dllimport)
#endif
#else
#define HCODEC_API
#endif

#include <string>
#include <fstream>
#include "FFMpeg/ffencoder.h"
#include "FFMpeg/ffdecoder.h"
#include "Common.h"


#define MAX_AUDIO_PACKET_SIZE 1024
#define MAX_VIDEO_PACKET_SIZE (1024 * 32)

enum eFramePixelFormat_t {
	F_PIX_FMT_INVALID = 0,
	F_PIX_FMT_I420,
	F_PIX_FMT_RGB24,
	F_PIX_FMT_BGR24,
	F_PIX_FMT_NV21,

	F_PIX_FMT_NB
};

struct FrameInfo {
	uint8_t* data_;
	int		 width_;
	int		 height_;
	eFramePixelFormat_t	eframePixFormat_;
};


class HCODEC_API H264Convetor {
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Codec_CB;

public:
	H264Convetor(const FFVideoFormat& enc_fmt, const FFVideoFormat& dec_fmt);	// video format
	~H264Convetor();

	// in_data: yuv420p; out_file: h264 data file.
	int Encode(uint8_t* in_data, int in_size, const std::string& out_file);
	// in_data: yuv420p; out_data: h264 nalu;
	int Encode(uint8_t* in_data, int in_size, uint8_t* out_data, int& out_size);

	// in_file: h264 encapsulated file, such as mp4, mkv; out_file: raw video data file(yuv,bgr).
	int Decode(const std::string& in_file, const std::string& out_file);
	// in_file: h264 encapsulated file; out: raw data stream(rgb, yuv);
	int Decode(const std::string& in_file, ST_Codec_CB codec_cb);
	// in_file: h264 data file; out: raw data stream(bgr, yuv);
	int DecodeH264(const std::string& in_file, ST_Codec_CB codec_cb);
	// ph264: h264 nalu; out_data: raw data stream(bgr, yuv);
	int Decode(uint8_t* ph264, int in_size, uint8_t* out_data, int& out_size);

protected:
	FFVideoFormat	encFmt_;
	FFVideoFormat	decFmt_;
	std::ofstream*	pIOStream_{ nullptr };
	FFEncoder*		pFFEncoder_{ nullptr };
	FFDecoder*		pFFDecoder_{ nullptr };
};