#include "stdafx.h"
#include "HMuxer.h"
#include "fflog.h"
#include "../libMemoryPool/libMemoryPool/MemoryPool.h"
#include <Windows.h>



char av_error_m[AV_TS_MAX_STRING_SIZE] = { 0 };
#define av_ts2str(ts) av_ts_make_string(av_error_m, ts)
#define av_ts2timestr(ts, tb) av_ts_make_time_string(av_error_m, ts, tb)
//############################################################################
//################HMuxerBase#################
//############################################################################
FILE* HMuxerBase::plogf_ = nullptr;

HMuxerBase::HMuxerBase(const FFVideoFormat &vfmt, const FFAudioFormat &afmt) : ffVFmt_(vfmt), ffAFmt_(afmt) {
	
}

HMuxerBase::~HMuxerBase(){
	if (outfmtCtx_) {
		avformat_close_input(&outfmtCtx_);
		avformat_free_context(outfmtCtx_);
		outfmtCtx_ = nullptr;
	}

	SAFE_DEL(vMemPool_);
	SAFE_DEL(aMemPool_);
	SAFE_DEL(workThread_);

	if (plogf_) {
		fclose(plogf_);
		plogf_ = nullptr;
	}
}

int HMuxerBase::Init() {
	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback(AvLog_cb);
	strlogfile_ = "HMuxerBase.txt";

	if (!plogf_)
		fopen_s(&plogf_, strlogfile_.c_str(), "a+");

	return 0;
}

void HMuxerBase::AvLog_cb(void *avcl, int level, const char *fmt, va_list vl)
{
	if (plogf_) {
		SYSTEMTIME sys;
		GetLocalTime(&sys);
		char szTime[128] = { 0 };
		sprintf_s(szTime, "I:%4d-%02d-%02d %02d:%02d:%02d ms:%03d:", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
		fwrite(szTime, strlen(szTime), 1, plogf_);
		vfprintf(plogf_, fmt, vl);
		fflush(plogf_);
	}
}

int HMuxerBase::Mux(const uint8_t *data, int size, int type) {
	int ret = -1;
	switch (type) {
	case 0: {
		std::lock_guard<std::mutex> locker(vMtx_);
		RawData_t raw_data;
		raw_data.data = vMemPool_->Alloc();
		memcpy(raw_data.data, data, size * sizeof(uint8_t));
		raw_data.size = size;
		vQueue_.emplace(raw_data);
		ret = 0;
		break;
	}
	case 1: {
		std::lock_guard<std::mutex> locker(aMtx_);
		RawData_t raw_data;
		raw_data.data = aMemPool_->Alloc();
		memcpy(raw_data.data, data, size * sizeof(uint8_t));
		raw_data.size = size;
		aQueue_.emplace(raw_data);
		ret = 0;
		break;
	}
	default: {
		LOGE("[HMuxer] Mux input with wrong data type! \n");
		break;
	}
	}

	return ret;
}



//############################################################################
//################HMuxer#################
//############################################################################
HMuxer::HMuxer(const FFVideoFormat &vfmt, const FFAudioFormat &afmt): HMuxerBase(vfmt, afmt){
	pFFVEncoder_ = new FFVideoEncoder(ffVFmt_);
	pFFAEncoder_ = new FFAudioEncoder(ffAFmt_);

	int vm_size = ffVFmt_.width_ * ffVFmt_.height_ * 3 >> 1;
	vMemPool_ = new MemoryPool<uint8_t, uint32_t>(vm_size, MEM_NUM);
	int am_size = ffAFmt_.frame_size_ * ffAFmt_.channels_ * 2;
	aMemPool_ = new MemoryPool<uint8_t>(am_size, MEM_NUM);
}

HMuxer::~HMuxer() {
	SAFE_DEL(pFFVEncoder_);
	SAFE_DEL(pFFAEncoder_);
}

int HMuxer::Init() {
	//init video output stream
	vOStream_.st = pFFVEncoder_->GetFFCodec()->avStream_;
	vOStream_.enc = pFFVEncoder_->GetFFCodec()->avctx_;
	vOStream_.next_pts = 1;
	vOStream_.samples_count = 0;
	vOStream_.frame = pFFVEncoder_->GetFFCodec()->frame_;
	vOStream_.tmp_frame = pFFVEncoder_->GetFFCodec()->frame_t_;
	vOStream_.sws_ctx = pFFVEncoder_->GetFFCodec()->swsctx_;
	vOStream_.swr_ctx = pFFVEncoder_->GetFFCodec()->swrctx_;

	//init audio output stream
	aOStream_.st = pFFAEncoder_->GetFFCodec()->avStream_;
	aOStream_.enc = pFFAEncoder_->GetFFCodec()->avctx_;
	aOStream_.next_pts = 0;
	aOStream_.samples_count = 0;
	aOStream_.frame = pFFAEncoder_->GetFFCodec()->frame_;
	aOStream_.tmp_frame = pFFAEncoder_->GetFFCodec()->frame_t_;
	aOStream_.sws_ctx = pFFAEncoder_->GetFFCodec()->swsctx_;
	aOStream_.swr_ctx = pFFAEncoder_->GetFFCodec()->swrctx_;

	return 0;
}

int HMuxer::Open(const string& out_file) {
	int ret = -1;
	// open the output ctx
	ret = avformat_alloc_output_context2(&outfmtCtx_, NULL, NULL, out_file.c_str());
	if (ret < 0) {
		LOGE("[HMuxer] unable to open the output ctx with out file:" << out_file);
		RETURN_V_IF(ret >= 0, -1);
	}

	// video
	ret = pFFVEncoder_->Open(outfmtCtx_);
	RETURN_V_IF(ret == 0, -1);
	// audio
	ret = pFFAEncoder_->Open(outfmtCtx_);
	RETURN_V_IF(ret == 0, -1);

	ret = Init();
	RETURN_V_IF(ret == 0, -1);

	ret = avformat_write_header(outfmtCtx_, NULL);
	if (!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT)) {
		LOGE("[HMuxer] avformat_write_header fail! \n");
		RETURN_V_IF(!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT), -1);
	}

	workThread_ = new thread([this]() {
		while (!bExit_) {
			// do muxing
			DoMux();
			Sleep(6);
		}
	});

	return ret;
}

int HMuxer::DoMux() {
	int data_type = GetMuxDataType();
	
	// mux video frame
	if (data_type == 0) {
		std::lock_guard<std::mutex> locker(vMtx_);
		if (vQueue_.empty())
			return 1;

		RawData_t raw_data;
		raw_data = vQueue_.front();
		vQueue_.pop();

		int ret = pFFVEncoder_->Encode(raw_data.data, raw_data.size);
		if (ret != 0) {
			LOGE("[HMuxer] Video encode err: %d!\n", ret);
			printf("[HMuxer] Video encode err: %d!\n", ret);
		}
		else {
			if (!vOStream_.frame)
				vOStream_.frame = pFFVEncoder_->GetFFCodec()->frame_;

			if(vOStream_.frame)
				vOStream_.frame->pts = vOStream_.next_pts++;
		}

		vMemPool_->Free(raw_data.data);
	}
	// mux audio frame
	else if(data_type == 1){
		std::lock_guard<std::mutex> locker(aMtx_);
		if (aQueue_.empty())
			return 1;

		RawData_t raw_data;
		raw_data = aQueue_.front();
		aQueue_.pop();

		int ret = pFFAEncoder_->Encode(raw_data.data, raw_data.size);
		if (ret != 0) {
			LOGE("[HMuxer] Audio encode err: %d!\n", ret);
			printf("[HMuxer] Audio encode err: %d!\n", ret);
		}
		else {
			if (!aOStream_.frame) {
				aOStream_.frame = pFFAEncoder_->GetFFCodec()->frame_;
				if (aOStream_.frame)
					aOStream_.next_pts = aOStream_.frame->nb_samples;
			}

			if (aOStream_.frame) {
				aOStream_.frame->pts = aOStream_.next_pts;
				aOStream_.next_pts += aOStream_.frame->nb_samples;
			}
		}		

		aMemPool_->Free(raw_data.data);
	}

	return 0;
}

int HMuxer::GetMuxDataType() {
	int type = -1;
	AVCodecContext *vc = vOStream_.enc;
	AVCodecContext *ac = aOStream_.enc;
	if (av_compare_ts(vOStream_.next_pts, vc->time_base, STREAM_DURATION, AVRational{ 1, 1 }) < 0 &&
		av_compare_ts(vOStream_.next_pts, vc->time_base, aOStream_.next_pts, ac->time_base) <= 0)
		type = 0;
	else if (av_compare_ts(aOStream_.next_pts, ac->time_base, STREAM_DURATION, AVRational{ 1, 1 }) < 0)
		type = 1;

	return type;
}

void HMuxer::Close() {
	bExit_ = 1;
	if (workThread_->joinable()) {
		workThread_->join();
	}

	pFFVEncoder_->GetFFCodec()->outfmtCtx_ = nullptr;
	pFFAEncoder_->GetFFCodec()->outfmtCtx_ = nullptr;

	if (outfmtCtx_) {
		int ret = av_write_trailer(outfmtCtx_);
		if (ret < 0) {
			printf("[HMuxer] av_write_trailer fail! \n");
		}
	}

	pFFVEncoder_->Close();
	pFFAEncoder_->Close();

	ffVFmt_.reset();
	ffAFmt_.reset();
}


//############################################################################
//################HReMuxer#################
//############################################################################
HReMuxer::HReMuxer(const FFVideoFormat &vfmt, const FFAudioFormat &afmt) : HMuxerBase(vfmt, afmt) {
	av_register_all();

	int vm_size = 1024 * 16;
	vMemPool_ = new MemoryPool<uint8_t, uint32_t>(vm_size, MEM_NUM);
	int am_size = ffAFmt_.frame_size_ >> 1;
	aMemPool_ = new MemoryPool<uint8_t>(am_size, MEM_NUM);
}

HReMuxer::~HReMuxer() {
	if (avpkt_) {
		av_packet_unref(avpkt_);
		avpkt_ = nullptr;
	}
}

int HReMuxer::Init() {
	avpkt_ = av_packet_alloc();
	av_init_packet(avpkt_);
	avpkt_->data = nullptr;
	avpkt_->size = 0;

	return 0;
}

int HReMuxer::Open(const string& out_file) {
	int ret = -1;
	// open the output ctx
	ret = avformat_alloc_output_context2(&outfmtCtx_, NULL, NULL, out_file.c_str());
	if (ret < 0) {
		LOGE("[HReMuxer] unable to open the output ctx with out file:" << out_file);
		RETURN_V_IF(ret >= 0, -1);
	}

	ret = AddStream(0);
	if (ret < 0) {
		LOGE("[HReMuxer] unable to add video stream:");
		RETURN_V_IF(ret == 0, -1);
	}
	ret = AddStream(1);
	if (ret < 0) {
		LOGE("[HReMuxer] unable to add audio stream:");
		RETURN_V_IF(ret == 0, -1);
	}

	Init();

	ret = avformat_write_header(outfmtCtx_, NULL);
	if (!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT)) {
		LOGE("[HReMuxer] avformat_write_header fail! \n");
		RETURN_V_IF(!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT), -1);
	}

	workThread_ = new thread([this]() {
		while (!bExit_) {
			// do muxing
			DoMux();
			Sleep(6);
		}
	});

	return 0;
}

int HReMuxer::DoMux() {
	int data_type = GetMuxDataType();

	// mux video frame
	if (data_type == 0) {
		std::lock_guard<std::mutex> locker(vMtx_);
		if (vQueue_.empty())
			return 1;

		RawData_t raw_data;
		raw_data = vQueue_.front();
		vQueue_.pop();

		avpkt_->data = raw_data.data;
		avpkt_->size = raw_data.size;
		avpkt_->pts = vOStream_.next_pts;
		avpkt_->dts = vOStream_.next_pts;

		int ret = WriteFrame(avpkt_, 0);
		if (ret != 0) {
			LOGE("[HReMuxer] write video packet err: %d!\n", ret);
			printf("[HReMuxer] write video packet: %d!\n", ret);
		}
		else {
			vOStream_.next_pts++;
		}

		vMemPool_->Free(raw_data.data);
	}
	// mux audio frame
	else if (data_type == 1) {
		std::lock_guard<std::mutex> locker(aMtx_);
		if (aQueue_.empty())
			return 1;

		RawData_t raw_data;
		raw_data = aQueue_.front();
		aQueue_.pop();

		avpkt_->data = raw_data.data;
		avpkt_->size = raw_data.size;
		avpkt_->pts = vOStream_.next_pts;
		avpkt_->dts = vOStream_.next_pts;

		int ret = WriteFrame(avpkt_, 0);
		if (ret != 0) {
			LOGE("[HReMuxer] write audio packet err: %d!\n", ret);
			printf("[HReMuxer] write audio packet: %d!\n", ret);
		}
		else {
			aOStream_.next_pts += ffAFmt_.frame_size_;
		}

		aMemPool_->Free(raw_data.data);
	}

	return 0;
}

int HReMuxer::GetMuxDataType() {
	int type = -1;
	AVStream *vst = vOStream_.st;
	AVStream *ast = aOStream_.st;
	if (av_compare_ts(vOStream_.next_pts, vst->time_base, STREAM_DURATION, AVRational{ 1, 1 }) < 0 &&
		av_compare_ts(vOStream_.next_pts, vst->time_base, aOStream_.next_pts, ast->time_base) <= 0)
		type = 0;
	else if (av_compare_ts(aOStream_.next_pts, ast->time_base, STREAM_DURATION, AVRational{ 1, 1 }) < 0)
		type = 1;

	return type;
}

int HReMuxer::WriteFrame(AVPacket *packet, int type) {
	if (outfmtCtx_) {
		if (type == 0) {
			av_packet_rescale_ts(packet, vOStream_.st->time_base, vOStream_.st->time_base);
			avpkt_->stream_index = vOStream_.st->index;
		}
		else if (type == 1) {
			av_packet_rescale_ts(packet, aOStream_.st->time_base, aOStream_.st->time_base);
			packet->stream_index = aOStream_.st->index;
		}

#ifdef _DEBUG
		LogPacket(packet);
#endif

		int ret = av_interleaved_write_frame(outfmtCtx_, packet);
		if (ret < 0) {
			printf("[HReMuxer] av_interleaved_write_frame fail \n");
		}
		av_packet_unref(packet);

		return ret;
	}

	return -1;
}

void HReMuxer::Close() {
	bExit_ = 1;
	if (workThread_->joinable()) {
		workThread_->join();
	}

	if (outfmtCtx_) {
		int ret = av_write_trailer(outfmtCtx_);
		if (ret < 0) {
			printf("[HReMuxer] av_write_trailer fail! \n");
		}
	}

	ffVFmt_.reset();
	ffAFmt_.reset();
}

int HReMuxer::AddStream(int type) {
	// output stream
	int ret = avio_open(&outfmtCtx_->pb, outfmtCtx_->filename, AVIO_FLAG_WRITE);
	if (ret < 0) {
		printf("[HReMuxer] avio_open out file: %s, fail! \n", outfmtCtx_->filename);
		RETURN_V_IF(ret >= 0, -1);
	}

	if (type == 0) {
		vOStream_.st = avformat_new_stream(outfmtCtx_, NULL);
		vOStream_.st->id = outfmtCtx_->nb_streams - 1;
		vOStream_.st->time_base = AVRational{ 1, ffVFmt_.fps_ };
	}
	else if (type == 1) {
		aOStream_.st = avformat_new_stream(outfmtCtx_, NULL);
		aOStream_.st->id = outfmtCtx_->nb_streams - 1;
		aOStream_.st->time_base = AVRational{ 1, ffAFmt_.sample_rate_ };
	}

	//ret = avcodec_parameters_from_context(ffcodec_->avStream_->codecpar, ffcodec_->avctx_);
	//if (ret < 0) {
	//	printf("[HReMuxer] avcodec_parameters_from_context fail! \n");
	//	RETURN_V_IF(ret >= 0, -1);
	//}

	/* Some formats want stream headers to be separate. */
	/*if (ffcodec_->outfmtCtx_->oformat->flags & AVFMT_GLOBALHEADER)
	ffcodec_->avctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;*/

	return ret;
}

void HReMuxer::LogPacket(AVPacket *packet) {
	AVRational *time_base = &(outfmtCtx_->streams[packet->stream_index]->time_base);

	printf("[HReMuxer] pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		av_ts2str(packet->pts), av_ts2timestr(packet->pts, time_base),
		av_ts2str(packet->dts), av_ts2timestr(packet->dts, time_base),
		av_ts2str(packet->duration), av_ts2timestr(packet->duration, time_base),
		packet->stream_index);
}