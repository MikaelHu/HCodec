#pragma once

#ifdef DLL_EXPORT
#ifdef BUILD_DLL
#define HCODEC_API __declspec(dllexport)
#else
#define HCODEC_API __declspec(dllimport)
#endif
#else
#define HCODEC_API
#endif

#include <functional>
#include "ffparam.h"
#include "ffcodec.h"
#include "../Common.h"



class HCODEC_API FFEncoder {
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Enc_CB;

public:
	FFEncoder::FFEncoder();
    FFEncoder(eFFCodecID_t codec_id);
    virtual ~FFEncoder();

	int setCodecID(eFFCodecID_t codec_id) {
		codec_id_ = codec_id;
		return 0;
	}

	FFCodec* GetFFCodec() { 
		return ffcodec_; 
	}

	//return 0:success, -1: failure
	virtual int Open() { return 0; }
	//return 0:success, -1: failure
	virtual int Open(const std::string& out_file) { return 0; }
	virtual int Open(AVFormatContext* av_fmt_ctx) { return 0; }
	//return 0:success, -1: failure
	virtual int Close() { return 0; }
	//return 0:success, <0: failure
	virtual int Encode(const uint8_t *in_data, int in_size) { return 0; }
	virtual int Encode(const uint8_t *in_data, int in_size, uint8_t* out_data, int &out_size) { return 0; }

protected:
	int Init();
	int OpenCodec();
	void LogPacket();
	int AddStream();

	static void AvLog_cb(void *avcl, int level, const char *fmt, va_list vl);

protected:
	eFFCodecID_t		codec_id_{ eFF_CODEC_ID_NONE };
	FFCodec*			ffcodec_{ nullptr };
	int64_t				frame_count_{ 0 };
	static FILE*		plogf_;
	std::string			strlogfile_;
};

class HCODEC_API FFVideoEncoder : public FFEncoder
{
public:
	// fmt: input raw vidoe data format.
	FFVideoEncoder(const FFVideoFormat &fmt);
	FFVideoEncoder(eFFCodecID_t codec_id, const FFVideoFormat &fmt);
	virtual ~FFVideoEncoder();

	int setFFFmt(FFVideoFormat &fmt) { ffFmt_ = fmt; return 0; }
	FFVideoFormat GetFFFmt() { return ffFmt_; }

	//return 0:success, -1: failure
	virtual int Open();
	//return 0:success, -1: failure
	virtual int Open(const std::string& out_file);
	virtual int Open(AVFormatContext* av_fmt_ctx);
	//return 0:success, -1: failure
	virtual int Close();
	//return 0:success, <0: failure
	virtual int Encode(const uint8_t *in_data, int in_size);
	virtual int Encode(const uint8_t *in_data, int in_size, uint8_t* out_data, int &out_size);

protected:
	int SetParams();
	int Flush(uint8_t* out_data, int &out_size);
	int DoEncode(uint8_t* out_data, int &out_size, AVFrame* frame);
	int PrepareFrame(const uint8_t *in_data, int in_size, AVFrame **frame);

protected:
	FFVideoFormat	ffFmt_;	//input video format
};

class HCODEC_API FFAudioEncoder : public FFEncoder
{
public:
	// fmt: input raw audio data format.
	FFAudioEncoder(const FFAudioFormat &fmt);
	FFAudioEncoder(eFFCodecID_t codec_id, const FFAudioFormat &fmt);
	virtual ~FFAudioEncoder();

	int setFFFmt(FFAudioFormat &fmt) { ffFmt_ = fmt; return 0; }
	FFAudioFormat GetFFFmt() { return ffFmt_; }

	//return 0:success, -1: failure
	virtual int Open();
	//return 0:success, -1: failure
	virtual int Open(const std::string& out_file);
	virtual int Open(AVFormatContext* av_fmt_ctx);
	//return 0:success, -1: failure
	virtual int Close();
	//return 0:success, <0: failure
	virtual int Encode(const uint8_t *in_data, int in_size);
	virtual int Encode(const uint8_t *in_data, int in_size, uint8_t* out_data, int &out_size);

protected:
	int SetParams();
	int Flush(uint8_t* out_data, int &out_size);
	int DoEncode(uint8_t* out_data, int &out_size, AVFrame* frame);
	int PrepareFrame(const uint8_t *in_data, int in_size, AVFrame **frame);

	// only support 2 channels convertion
	void F32leConvert2Fltp(float *f32le, float *fltp, int nb_samples);

protected:
	FFAudioFormat	ffFmt_;	//input audio format
};