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

#include <fstream>
#include "FFMpeg/ffdecoder.h"
#include "Common.h"



class HCODEC_API HDecoder {
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Dec_CB;

public:
	HDecoder();
	virtual ~HDecoder();

	//return: -1 failure, 0 success
	int Open();
	int Open(const std::string& in_file);
	//return: -1 failure, 0 success
	int Close();
	//return 0:success, <0: failure
	virtual int Decode(const std::string& out_file) {
		return 0;
	}
	virtual int Decode(ST_Dec_CB dec_cb) {
		return 0;
	}
	virtual int Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
		return pFFDecoder_->Decode(in_data, in_size, out_data, out_size);
	}

protected:
	FFDecoder*			pFFDecoder_{ nullptr };
	std::ofstream*		pIOStream_{ nullptr };
};

class HCODEC_API HVideoDecoder : public HDecoder {
public:
	HVideoDecoder(const FFVideoFormat& fmt);
	HVideoDecoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt);
	virtual ~HVideoDecoder();

	virtual int Decode(const std::string& out_file);
	virtual int Decode(ST_Dec_CB dec_cb);
};

class HCODEC_API HAudioDecoder : public HDecoder {
public:
	HAudioDecoder(const FFAudioFormat& fmt);
	HAudioDecoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt);
	virtual ~HAudioDecoder();

	virtual int Decode(const std::string& out_file);
	virtual int Decode(ST_Dec_CB dec_cb);
};