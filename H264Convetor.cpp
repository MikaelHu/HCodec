#include "stdafx.h"
#include <assert.h>
#include <memory>
#include <iostream>
#include "H264Convetor.h"
#include "H264Parser.h"


H264Convetor::H264Convetor(const FFVideoFormat& enc_fmt, const FFVideoFormat& dec_fmt): encFmt_(enc_fmt), decFmt_(dec_fmt)
{
	pFFEncoder_ = new FFVideoEncoder(eFF_CODEC_ID_H264, encFmt_);
	pFFDecoder_ = new FFVideoDecoder(eFF_CODEC_ID_H264, decFmt_);
}

H264Convetor::~H264Convetor() {
	pFFEncoder_->Close();
	pFFDecoder_->Close();

	SAFE_DEL(pFFEncoder_);
	SAFE_DEL(pFFDecoder_);
	SAFE_DEL(pIOStream_);
}

int H264Convetor::Encode(uint8_t* in_data, int in_size, const std::string& out_file) {
	static int b_first = 1;
	if (b_first) {
		b_first = 0;
		if (pFFEncoder_->Open(out_file) < 0) {
			std::cerr << "[H264Convetor] unable to open the ffencoder:" << std::endl;
			return -1;
		}
	}

	return pFFEncoder_->Encode(in_data, in_size);
}

int H264Convetor::Encode(uint8_t* in_data, int in_size, uint8_t* out_data, int& out_size) {
	static int b_first_en = 1;

	if (b_first_en) {
		b_first_en = 0;

		if (pFFEncoder_->Open() < 0) {
			std::cerr << "[H264Convetor] unable to open the ffencoder:" << std::endl;
			return -1;
		}
	}

	return pFFEncoder_->Encode(in_data, in_size, out_data, out_size);
}

int H264Convetor::Decode(const std::string& in_file, const std::string& out_file) {
	pIOStream_ = new std::ofstream(out_file, std::ios::out | std::ios::binary);
	if (!pIOStream_) {
		std::cerr << "[H264Convetor] unable to open the output file:" << out_file;
		return -1;
	}

	if (pFFDecoder_->Open(in_file) < 0) {
		std::cerr << "[H264Convetor] unable to open the ffdecoder:" << std::endl;
		return -1;
	}

	int ret = pFFDecoder_->Decode([&](uint8_t* out_data, int out_size) {
		pIOStream_->write((char*)out_data, out_size);
		return 0;
	});
	pIOStream_->close();

	return ret;
}

int H264Convetor::Decode(const std::string& in_file, ST_Codec_CB codec_cb) {
	if (pFFDecoder_->Open(in_file) < 0) {
		std::cerr << "[H264Convetor] unable to open the ffdecoder:" << std::endl;
		return -1;
	}

	int ret = pFFDecoder_->Decode(codec_cb);

	return ret;
}

int H264Convetor::DecodeH264(const std::string& in_file, ST_Codec_CB codec_cb) {
	std::fstream ifs(in_file, std::ios::in | std::ios::binary);
	if (!ifs.is_open()) {
		std::cerr << "[H264Convetor] unable to open the in_file:" << std::endl;
		return -1;
	}

	ifs.seekg(0, std::ios::end);
	int size = ifs.tellg(); // 文件大小
	char* h264 = new char[size + 1];
	ifs.seekg(0, std::ios::beg);
	ifs.read(h264, size);

	const int OUT_SIZE = decFmt_.width_ * decFmt_.height_ * 3;
	uint8_t* out_data = new uint8_t[OUT_SIZE];
	const int NALU_SIZE = 1024 * 16;
	uint8_t* nalu = new uint8_t[NALU_SIZE];

	int pos = 0;
	while(pos < size){
		memset(nalu, 0, NALU_SIZE);
		int len = 0;
		int start = H264Parser().Parse((uint8_t*)(h264 + pos), size - pos, nalu, len);
		assert(len <= NALU_SIZE);
		if (start >= 0) {
			memset(out_data, 0, OUT_SIZE);
			int out_size = OUT_SIZE;
			int ret = Decode((uint8_t*)nalu, len, out_data, out_size);
			if (ret == 0) {
				codec_cb(out_data, out_size);
			}
			pos += start + len;
		}
		else
			break;
	}

	ifs.close();

	SAFE_DEL_A(h264);
	SAFE_DEL_A(out_data);
	SAFE_DEL_A(nalu);

	return 0;
}

int H264Convetor::Decode(uint8_t* ph264, int in_size, uint8_t* out_data, int& out_size) {
	static int b_first_de = 1;

	if (b_first_de) {
		b_first_de = 0;

		if (pFFDecoder_->Open() < 0) {
			std::cerr << "[H264Convetor] unable to open the ffdecoder:" << std::endl;
			return -1;
		}
	}

	return pFFDecoder_->Decode(ph264, in_size, out_data, out_size);
}