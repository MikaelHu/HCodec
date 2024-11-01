#include "stdafx.h"
#include "HDecoder.h"
#include <iostream>
#include <Windows.h>



HDecoder::HDecoder() {
}

HDecoder::~HDecoder() {
	SAFE_DEL(pFFDecoder_);
	SAFE_DEL(pIOStream_);
}

int HDecoder::Open() {
	return pFFDecoder_->Open();
}

int HDecoder::Open(const std::string& in_file) {
	return pFFDecoder_->Open(in_file);
}

int HDecoder::Close() {
	return pFFDecoder_->Close();
}



HVideoDecoder::HVideoDecoder(const FFVideoFormat& fmt) {
	pFFDecoder_ = new FFVideoDecoder(fmt);
}

HVideoDecoder::HVideoDecoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt){
	pFFDecoder_ = new FFVideoDecoder(codec_id, fmt);
}

HVideoDecoder::~HVideoDecoder() {
}

int HVideoDecoder::Decode(const std::string& out_file) {
	pIOStream_ = new std::ofstream(out_file, std::ios::out | std::ios::binary);
	if (!pIOStream_) {
		std::cerr << "[HVideoDecoder] unable to open the output file:" << out_file;
		return -1;
	}

	FFDecoder::ST_Dec_CB dec_cb = [&](uint8_t* out_data, int out_size) {
		pIOStream_->write((char*)out_data, out_size);
		return 0;
	};

	pFFDecoder_->Decode(dec_cb);
	pIOStream_->close();

	return 0;
}

int HVideoDecoder::Decode(ST_Dec_CB dec_cb) {
	return pFFDecoder_->Decode(dec_cb);
}



HAudioDecoder::HAudioDecoder(const FFAudioFormat& fmt) {
	pFFDecoder_ = new FFAudioDecoder(fmt);
}

HAudioDecoder::HAudioDecoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt) {
	pFFDecoder_ = new FFAudioDecoder(codec_id, fmt);
}

HAudioDecoder::~HAudioDecoder() {
}

int HAudioDecoder::Decode(const std::string& out_file) {
	pIOStream_ = new std::ofstream(out_file, std::ios::out | std::ios::binary);
	if (!pIOStream_) {
		std::cerr<<"[HAudioDecoder] unable to open the output file:" << out_file;
		return -1;
	}

	FFDecoder::ST_Dec_CB dec_cb = [&](uint8_t* out_data, int out_size) {
		pIOStream_->write((char*)out_data, out_size);
		return 0;
	};

	pFFDecoder_->Decode(dec_cb);	
	pIOStream_->close();

	return 0;
}

int HAudioDecoder::Decode(ST_Dec_CB dec_cb) {
	return pFFDecoder_->Decode(dec_cb);
}