#include "stdafx.h"
#include "HDeMuxer.h"
#include <Windows.h>
#include "fflog.h"



char av_error_dm[AV_TS_MAX_STRING_SIZE] = { 0 };
#define av_ts2str(ts) av_ts_make_string(av_error_dm, ts)
#define av_ts2timestr(ts, tb) av_ts_make_time_string(av_error_dm, ts, tb)
//############################################################################
//################HDeMuxer#################
//############################################################################
FILE* HDeMuxer::plogf_ = nullptr;

HDeMuxer::HDeMuxer() {
	av_register_all();

	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback(AvLog_cb);
	strlogfile_ = "HDeMuxer.txt";

	if (!plogf_)
		fopen_s(&plogf_, strlogfile_.c_str(), "a+");
}

HDeMuxer::~HDeMuxer() {
	if (plogf_) {
		fclose(plogf_);
		plogf_ = nullptr;
	}
}

int HDeMuxer::Init() {
	avpkt_ = av_packet_alloc();
	av_init_packet(avpkt_);
	avpkt_->data = nullptr;
	avpkt_->size = 0;

	return 0;
}

void HDeMuxer::AvLog_cb(void *avcl, int level, const char *fmt, va_list vl)
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

int HDeMuxer::Open(const string& in_file) {
	// open the video file
	if (avformat_open_input(&infmtCtx_, in_file.c_str(), nullptr, nullptr) < 0) {
		LOGE("[HDeMuxer] unable to open the media file: " << in_file);
		return -1;
	}

	// get video stream info
	if (avformat_find_stream_info(infmtCtx_, nullptr) < 0) {
		LOGE("[HDeMuxer] unable to get the media stream info!");
		return -1;
	}

	// get video stream index
	for (int i = 0; i < infmtCtx_->nb_streams; i++) {
		if (infmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			vStreamIndex_ = i;
		}
		if (infmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			aStreamIndex_ = i;
		}
	}
	if (vStreamIndex_ < 0 && aStreamIndex_ < 0) {
		LOGE("[HDeMuxer] unable to get the media stream index!");
		return -1;
	}

	// get vedio decoder params
	AVCodecParameters* codecParameters = infmtCtx_->streams[vStreamIndex_]->codecpar;

	codec_ = avcodec_find_decoder(codecParameters->codec_id);
	if (!codec_) {
		LOGE("[HDeMuxer] unable to open codec!");
		return -1;
	}

	avctx_ = avcodec_alloc_context3(codec_);
	if (!avctx_) {
		LOGE("[HDeMuxer] unable to alloc avctx!");
		return -1;
	}

	avcodec_parameters_to_context(avctx_, codecParameters);

	int nret = avcodec_open2(avctx_, codec_, nullptr);
	if (nret) {
		LOGE("[HDeMuxer] unable to open codex!");
		return -1;
	}

	Init();

	return 0;
}

int HDeMuxer::DeMux(ST_Dmx_CB dmx_cb) {
	while(av_read_frame(infmtCtx_, avpkt_) >= 0) {
		LogPacket();

		if (avpkt_->stream_index == vStreamIndex_) {
			ProcessH264Pkt(avpkt_);
			dmx_cb(avpkt_->data, avpkt_->size, 0);
		}
		if (avpkt_->stream_index == aStreamIndex_) {
			dmx_cb(avpkt_->data, avpkt_->size, 1);
		}

		av_packet_unref(avpkt_);
	}

	//1 获取相应的比特流过滤器
	//FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加SPS/PPS等信息。
	//FLV封装时，可以把多个NALU放在一个VIDEO TAG中,结构为4B NALU长度+NALU1+4B NALU长度+NALU2+...,
	//需要做的处理把4B长度换成00000001或者000001

//	const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
//	AVBSFContext *bsf_ctx = NULL;
//	// 2 初始化过滤器上下文
//	av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;
//	// 3 添加解码器属性
//	avcodec_parameters_copy(bsf_ctx->par_in, infmtCtx_->streams[aStreamIndex_]->codecpar);
//	av_bsf_init(bsf_ctx);
//	
//	while (av_read_frame(infmtCtx_, avpkt_) >= 0)
//	{
//		if (avpkt_->stream_index == vStreamIndex_)
//		{
//#if 1
//			int input_size = avpkt_->size;
//			int out_pkt_count = 0;
//			// bitstreamfilter内部去维护内存空间
//			if (av_bsf_send_packet(bsf_ctx, avpkt_) != 0)
//			{
//				av_packet_unref(avpkt_);
//				continue;       // 继续送
//			}
//	
//			while (av_bsf_receive_packet(bsf_ctx, avpkt_) == 0)
//			{
//				out_pkt_count++;
//				dmx_cb(avpkt_->data, avpkt_->size, 0);
//			}
//	
//			av_packet_unref(avpkt_);
//	
//			if (out_pkt_count >= 2)
//				printf("[HDeMuxer] cur pkt(size:%d) only get 1 out packet, it get %d packets\n",
//					input_size, out_pkt_count);
//	
//#else       // TS流可以直接写入
//			size_t size = fwrite(avpkt_->data, 1, avpkt_->size, outfp);
//			if (size != avpkt_->size)
//				printf("[HDeMuxer] fwrite failed-> write:%u, packet_size:%u\n", size, avpkt_->size);
//
//			av_packet_unref(avpkt_);
//#endif
//		}
//		else
//			av_packet_unref(avpkt_);        // 释放内存
//	}
//	
//	av_bsf_free(&bsf_ctx);

	return 0;
}

void HDeMuxer::Close() {
	if (avpkt_) {
		av_packet_unref(avpkt_);
		avpkt_ = nullptr;
	}
	if (infmtCtx_) {
		avformat_close_input(&infmtCtx_);
		avformat_free_context(infmtCtx_);
		infmtCtx_ = nullptr;
	}
}

int HDeMuxer::ProcessH264Pkt(AVPacket* pkt) {
	if (pkt->stream_index == 0)
	{// video stream
		AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
		int a;
		while (bsfc) {
			AVPacket new_pkt = *pkt;
			a = av_bitstream_filter_filter(bsfc, avctx_, NULL,
				&new_pkt.data, &new_pkt.size,
				pkt->data, pkt->size,
				pkt->flags & AV_PKT_FLAG_KEY);
			if (a == 0 && new_pkt.data != pkt->data && new_pkt.destruct) {
				uint8_t *t = (uint8_t*)(new_pkt.size + FF_INPUT_BUFFER_PADDING_SIZE); //the new should be a subset of the old so cannot overflow
				if (t) {
					memcpy(t, new_pkt.data, new_pkt.size);
					memset(t + new_pkt.size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
					new_pkt.data = t;
					a = 1;
				}
				else
					a = AVERROR(ENOMEM);
			}
			if (a > 0 && pkt->data != new_pkt.data) {
				av_free_packet(pkt);
				new_pkt.destruct = av_destruct_packet;
			}
			else if (a < 0) {
				envir() << "!!!!!!!!!!av_bitstream_filter_filter failed" << ",res=" << a << "\n";
			}
			*pkt = new_pkt;

			bsfc = bsfc->next;
		}
	}

	return 0;
}

void HDeMuxer::LogPacket() {
	AVFormatContext *ctx = infmtCtx_;
	AVPacket *pkt = avpkt_;
	AVRational *time_base = &(ctx->streams[pkt->stream_index]->time_base);

	printf("[HDeMuxer] pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		pkt->stream_index);
}