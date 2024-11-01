#include "stdafx.h"
#include "HEncoder.h"
#include <iostream>



HEncoder::HEncoder() {
}

HEncoder::~HEncoder() {
	SAFE_DEL(pFFEncoder_);
}

int HEncoder::Open() {
	return pFFEncoder_->Open();
}

int HEncoder::Open(const std::string& out_file) {
	return pFFEncoder_->Open(out_file);
}

int HEncoder::Close() {
	return pFFEncoder_->Close();

}



HVideoEncoder::HVideoEncoder(const FFVideoFormat& fmt) {
	pFFEncoder_ = new FFVideoEncoder(fmt);
}

HVideoEncoder::HVideoEncoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt) {
	pFFEncoder_ = new FFVideoEncoder(codec_id, fmt);
}

HVideoEncoder::~HVideoEncoder() {
}

int HVideoEncoder::Encode(const uint8_t *in_data, int in_size)
{
	return pFFEncoder_->Encode(in_data, in_size);
}



HAudioEncoder::HAudioEncoder(const FFAudioFormat& fmt) {
	pFFEncoder_ = new FFAudioEncoder(fmt);
}

HAudioEncoder::HAudioEncoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt) {
	pFFEncoder_ = new FFAudioEncoder(codec_id, fmt);
}

HAudioEncoder::~HAudioEncoder() {
}

int HAudioEncoder::Encode(const uint8_t *in_data, int in_size)
{
	return pFFEncoder_->Encode(in_data, in_size);
}
