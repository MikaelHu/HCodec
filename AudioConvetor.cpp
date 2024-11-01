#include "stdafx.h"
#include "AudioConvetor.h"



AudioConvetor::AudioConvetor(const FFAudioFormat& fmt): fmt_(fmt) {
	pFFEncoder_ = new FFAudioEncoder(eFF_CODEC_ID_AAC, fmt_);
	pFFDecoder_ = new FFAudioDecoder(eFF_CODEC_ID_AAC, fmt);
}

AudioConvetor::~AudioConvetor() {
	pFFEncoder_->Close();
	pFFDecoder_->Close();

	SAFE_DEL(fmtCtx_);
	SAFE_DEL(pFFEncoder_);
	SAFE_DEL(pFFDecoder_);
}

int AudioConvetor::Open() {
	av_register_all();

	return 0;
}

int AudioConvetor::Close() {
	avformat_close_input(&fmtCtx_);

	return 0;
}

int AudioConvetor::Encode(const std::string& in_file, const std::string& out_file) {

	return 0;
}

int AudioConvetor::Encode(uint8_t* pcm, int size_p, uint8_t** aac, int& size_a) {

	return 0;
}

int AudioConvetor::Decode(const std::string& in_file, const std::string& out_file) {
	if (avformat_open_input(&fmtCtx_, in_file.c_str(), nullptr, nullptr) < 0) {
		std::cout << "无法打开输入音频文件" << std::endl;
		return -1;
	}

	// 获取音频流信息
	if (avformat_find_stream_info(fmtCtx_, nullptr) < 0) {
		std::cout << "无法获取音频流信息" << std::endl;
		avformat_close_input(&fmtCtx_);
		return -1;
	}

	// 找到音频流索引
	int audioStreamIndex = -1;
	for (int i = 0; i < fmtCtx_->nb_streams; i++) {
		if (fmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIndex = i;
			break;
		}
	}

	// 创建输出音频文件
	std::ofstream outputFile(out_file, std::ios::binary);
	if (!outputFile.is_open()) {
		std::cout << "无法打开输出音频文件" << std::endl;
		return -1;
	}

	AVPacket packet;
	uint8_t out_data[1024 * 6] = { 0 };
	int out_size;
	while (av_read_frame(fmtCtx_, &packet) >= 0) {
		if (packet.stream_index == audioStreamIndex) {
			static int b_first = 1;
			if (b_first)
				b_first = 0;
				AVCodecParameters* codecParameters = fmtCtx_->streams[audioStreamIndex]->codecpar;
				eFFCodecID_t codec_id = FFCodec(eFF_MEDIA_AUDIO).get_FF_codecID(codecParameters->codec_id);
				FFAudioFormat out_fmt;
				int sample_rate = out_fmt.sample_rate_;
				eFFSampleFormat_t sample_fmt = out_fmt.sample_fmt_;
				eFFChannelLayout_t channel_layout = out_fmt.channel_layout_;
				int channels = out_fmt.channels_;
				int bitrate = out_fmt.bitrate_;
				int frame_size = out_fmt.frame_size_;
				out_fmt.set(sample_rate, sample_fmt, channel_layout, channels, bitrate, frame_size);
				pFFDecoder_->Open();
			}

			int ret = pFFDecoder_->Decode(packet.data, packet.size, out_data, out_size);

			if (ret == 0) {
				outputFile.write((char*)out_data, out_size);
			}
		}
	outputFile.close();

	return 0;
}

int AudioConvetor::Decode(uint8_t* aac, int size_a, uint8_t* pcm, int size_p) {

	return 0;
}