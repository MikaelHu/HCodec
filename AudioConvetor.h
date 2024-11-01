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

#include "stdafx.h"
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include "FFMpeg/ffencoder.h"
#include "FFMpeg/ffdecoder.h"
#include "Common.h"



class HCODEC_API AudioConvetor {
public:
	AudioConvetor(const FFAudioFormat& fmt);
	~AudioConvetor();

	int Open();
	int Close();

	int Encode(const std::string& in_file, const std::string& out_file);
	int Encode(uint8_t* pcm, int size_p, uint8_t** aac, int& size_a);
	int Decode(const std::string& in_file, const std::string& out_file);
	int Decode(uint8_t* aac, int size_a, uint8_t* pcm, int size_p);

protected:
	FFAudioFormat		fmt_;
	AVFormatContext*	fmtCtx_{ nullptr };
	FFEncoder*			pFFEncoder_{ nullptr };
	FFDecoder*			pFFDecoder_{ nullptr };
};