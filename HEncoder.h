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

#include "FFMpeg/ffencoder.h"
#include "Common.h"


class HCODEC_API HEncoder {
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Enc_CB;

public:
	HEncoder();
	virtual ~HEncoder();

	//return: -1 failure, 0 success
	int Open();
	//return 0:success, -1: failure
	virtual int Open(const std::string& out_file);
	//return: -1 failure, 0 success
	int Close();
	//return 0:success, <0: failure
	virtual int Encode(const uint8_t *in_data, int in_size) { return 0; }
	virtual int Encode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
		return pFFEncoder_->Encode(in_data, in_size, out_data, out_size);
	}

protected:
	FFEncoder*			pFFEncoder_{ nullptr };
};

class HCODEC_API HVideoEncoder : public HEncoder {
public:
	HVideoEncoder(const FFVideoFormat& fmt);
	HVideoEncoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt);
	virtual ~HVideoEncoder();

	virtual int Encode(const uint8_t *in_data, int in_size);
};

class HCODEC_API HAudioEncoder : public HEncoder {
public:
	HAudioEncoder(const FFAudioFormat& fmt);
	HAudioEncoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt);
	virtual ~HAudioEncoder();

	virtual int Encode(const uint8_t *in_data, int in_size);
};