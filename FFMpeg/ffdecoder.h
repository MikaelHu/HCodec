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



class HCODEC_API FFDecoder {
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Dec_CB;

public:
	FFDecoder();
    FFDecoder(eFFCodecID_t codec_id);
    virtual ~FFDecoder();

	int setCodecID(eFFCodecID_t codec_id) { 
		codec_id_ = codec_id; 
		return 0;
	}

	//return: -1 failure, 0 success
	virtual int Open() { return 0; }
	virtual int Open(const std::string& in_file) { return 0; }
	//return: -1 failure, 0 success
	virtual int Close() { return 0; }
	//return 0:success, <0: failure
	virtual int Decode(ST_Dec_CB dec_cb) { return 0; }
	virtual int Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
		return 0;
	}

protected:
	int Init();
    int OpenCodec();
	static void AvLog_cb(void *avcl, int level, const char *fmt, va_list vl);
	void LogPacket();

protected:
	eFFCodecID_t		codec_id_{ eFF_CODEC_ID_NONE };
	FFCodec*			ffcodec_{ nullptr };
	int					streamIndex_{ -1 };
	static FILE*		plogf_;
	std::string			strlogfile_;
};

class HCODEC_API FFVideoDecoder: public FFDecoder {
public:
	FFVideoDecoder(const FFVideoFormat& fmt);
	FFVideoDecoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt);
	virtual ~FFVideoDecoder();

	int setFFFmt(FFVideoFormat &fmt) { ffFmt_ = fmt; return 0; }
	FFVideoFormat GetFFFmt() { return ffFmt_; }

	virtual int Open();
	virtual int Open(const std::string& in_file);
	virtual int Close();
	virtual int Decode(ST_Dec_CB dec_cb);
	virtual int Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size);

protected:
	int Init();
	int Flush(uint8_t* out_data, int &out_size);
	int DoDecode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size);

protected:
	FFVideoFormat	ffFmt_;	//output video format
};

class HCODEC_API FFAudioDecoder : public FFDecoder {
public:
	FFAudioDecoder(const FFAudioFormat& fmt);
	FFAudioDecoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt);
	virtual ~FFAudioDecoder();

	int setFFFmt(FFAudioFormat &fmt) { ffFmt_ = fmt; return 0; }
	FFAudioFormat GetFFFmt() { return ffFmt_; }

	virtual int Open();
	virtual int Open(const std::string& in_file);
	virtual int Close();
	virtual int Decode(ST_Dec_CB dec_cb);
	virtual int Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size);

	int GetOutbufSize() { return outbufsize_; }

protected:
	int Init();
	int Flush(uint8_t* out_data, int &out_size);
	int DoDecode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size);

protected:
	int	outbufsize_{ 0 };
	FFAudioFormat	ffFmt_;	//output audio format
};