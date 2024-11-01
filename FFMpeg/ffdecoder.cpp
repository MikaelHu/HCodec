#include "stdafx.h"
#include "ffdecoder.h"
#include "fflog.h"
#include <Windows.h>



char av_error_d[AV_TS_MAX_STRING_SIZE] = { 0 };
#define av_ts2str(ts) av_ts_make_string(av_error_d, ts)
#define av_ts2timestr(ts, tb) av_ts_make_time_string(av_error_d, ts, tb)
//############################################################################
//################FFDecoder#################
//############################################################################
FILE* FFDecoder::plogf_ = nullptr;

FFDecoder::FFDecoder() {
	Init();
}

FFDecoder::FFDecoder(eFFCodecID_t codec_id): codec_id_(codec_id){
	Init();
}

FFDecoder::~FFDecoder() {
	if (plogf_) {
		fclose(plogf_);
		plogf_ = nullptr;
	}

	SAFE_DEL(ffcodec_);
}

int FFDecoder::Init() {
	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback(AvLog_cb);
	strlogfile_ = "FFmpegDeCodecLog.txt";

	if (!plogf_)
		fopen_s(&plogf_, strlogfile_.c_str(), "a+");

	return 0;
}

void FFDecoder::AvLog_cb(void *avcl, int level, const char *fmt, va_list vl)
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

int FFDecoder::OpenCodec() {
    RETURN_V_IF(ffcodec_, -1);

    AVCodecID av_codec_id = ffcodec_->get_AV_codecID(codec_id_);
    RETURN_V_IF(av_codec_id != AV_CODEC_ID_NONE, -1);

    ffcodec_->codec_ = avcodec_find_decoder(av_codec_id);
    RETURN_V_IF(ffcodec_->codec_, -1);

    ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
    RETURN_V_IF(ffcodec_->avctx_, -1);

    int nret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
    RETURN_V_IF(nret == 0, -1);

    return 0;
}

void FFDecoder::LogPacket() {
	AVFormatContext *ctx = ffcodec_->infmtCtx_;
	AVPacket *pkt = ffcodec_->avpkt_;
	AVRational *time_base = &(ctx->streams[pkt->stream_index]->time_base);

	printf("[FFDecoder] pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		pkt->stream_index);
}


//############################################################################
//################FFVideoDecoder#################
//############################################################################
FFVideoDecoder::FFVideoDecoder(const FFVideoFormat& fmt) : FFDecoder(), ffFmt_(fmt) {
}

FFVideoDecoder::FFVideoDecoder(eFFCodecID_t codec_id, const FFVideoFormat& fmt):
	FFDecoder(codec_id), ffFmt_(fmt) {
}

FFVideoDecoder::~FFVideoDecoder() {
}

int FFVideoDecoder::Open() {
	RETURN_V_IF(!ffcodec_, 0);

	ffcodec_ = new FFCodec(eFF_MEDIA_VIDEO);
	RETURN_V_IF(ffcodec_, -1);

	int nret = OpenCodec();
	if (nret != 0) {
		SAFE_DEL(ffcodec_);
		LOGE("[FFVideoDecoder] fail to open ff_codec_id=" << codec_id_ << ", return=" << nret);
	}

	return Init();
}

int FFVideoDecoder::Open(const std::string& in_file) {
	RETURN_V_IF(!ffcodec_, 0);

	ffcodec_ = new FFCodec(eFF_MEDIA_VIDEO);
	RETURN_V_IF(ffcodec_, -1);

	// open the video file
	if (avformat_open_input(&ffcodec_->infmtCtx_, in_file.c_str(), nullptr, nullptr) < 0) {
		LOGE("[FFVideoDecoder] unable to open the video file:" << in_file);
		return -1;
	}

	// get video stream info
	if (avformat_find_stream_info(ffcodec_->infmtCtx_, nullptr) < 0) {
		LOGE("[FFVideoDecoder] unable to get the video stream info!");
		return -1;
	}

	// get video stream index
	for (int i = 0; i < ffcodec_->infmtCtx_->nb_streams; i++) {
		if (ffcodec_->infmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			streamIndex_ = i;
			break;
		}
	}
	if (streamIndex_ < 0) {
		LOGE("[FFVideoDecoder] unable to get the video stream index!");
		return -1;
	}

	// get vedio decoder params
	AVCodecParameters* codecParameters = ffcodec_->infmtCtx_->streams[streamIndex_]->codecpar;
	codec_id_ = FFCodec().get_FF_codecID(codecParameters->codec_id);

	ffcodec_->codec_ = avcodec_find_decoder(codecParameters->codec_id);
	if (!ffcodec_->codec_) {
		LOGE("[FFVideoDecoder] unable to open codec!");
		return -1;
	}

	ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
	if (!ffcodec_->avctx_) {
		LOGE("[FFVideoDecoder] unable to alloc avctx!");
		return -1;
	}

	avcodec_parameters_to_context(ffcodec_->avctx_, codecParameters);

	int nret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
	if (nret) {
		LOGE("[FFVideoDecoder] unable to open codex!");
		return -1;
	}

	return Init();
}

int FFVideoDecoder::Init() {
	// check output format
	AVPixelFormat out_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);
	if (out_pix_fmt == AV_PIX_FMT_NONE) {
		LOGE("[FFVideoDecoder] unsupported output ff_pix_fmt=" << ffFmt_.pix_fmt_);
		return -1;
	}
	if (!sws_isSupportedOutput(out_pix_fmt)) {
		LOGE("[FFVideoDecoder] av_sws unsupported output ff_pix_fmt=" << ffFmt_.pix_fmt_ << ", pix_fmt=" << out_pix_fmt);
		return -1;
	}

	ffcodec_->frame_ = av_frame_alloc();
	ffcodec_->avpkt_ = av_packet_alloc();
	av_init_packet(ffcodec_->avpkt_);
	ffcodec_->avpkt_->data = nullptr;
	ffcodec_->avpkt_->size = 0;

	return 0;
}

int FFVideoDecoder::Close() {
	ffFmt_.reset();

	return 0;
}

int FFVideoDecoder::Decode(ST_Dec_CB dec_cb) {
	if (ffFmt_.width_ == 0 || ffFmt_.height_ == 0) {
		ffFmt_.width_ = ffcodec_->avctx_->width;
		ffFmt_.height_ = ffcodec_->avctx_->height;
	}
	const int BUFFSIZE = ffFmt_.width_ * ffFmt_.height_ * 3;
	uint8_t* out_data = new uint8_t[BUFFSIZE];

	while (av_read_frame(ffcodec_->infmtCtx_, ffcodec_->avpkt_) >= 0) {
		if (ffcodec_->avpkt_->stream_index == streamIndex_) {
			LogPacket();

			memset(out_data, 0, BUFFSIZE);
			int out_size = BUFFSIZE;
			if (!Decode(ffcodec_->avpkt_->data, ffcodec_->avpkt_->size, out_data, out_size)) {
				dec_cb(out_data, out_size);
			}
		}
		av_packet_unref(ffcodec_->avpkt_);
	}

//	// 1 获取相应的比特流过滤器
//	// FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加SPS/PPS等信息。
//	// FLV封装时，可以把多个NALU放在一个VIDEO TAG中,结构为4B NALU长度+NALU1+4B NALU长度+NALU2+...,
//	// 需要做的处理把4B长度换成00000001或者000001
//	const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
//	AVBSFContext *bsf_ctx = NULL;
//	// 2 初始化过滤器上下文
//	av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;
//	// 3 添加解码器属性
//	avcodec_parameters_copy(bsf_ctx->par_in, infmtCtx_->streams[videoStreamIndex]->codecpar);
//	av_bsf_init(bsf_ctx);
//
//	while (av_read_frame(infmtCtx_, &packet) >= 0)
//	{
//		if (packet.stream_index == videoStreamIndex)
//		{
//#if 1
//			int input_size = packet.size;
//			int out_pkt_count = 0;
//			// bitstreamfilter内部去维护内存空间
//			if (av_bsf_send_packet(bsf_ctx, &packet) != 0) 
//			{
//				av_packet_unref(&packet); 
//				continue;       // 继续送
//			}
//
//			while (av_bsf_receive_packet(bsf_ctx, &packet) == 0)
//			{
//				out_pkt_count++;
//				memset(out_data, 0, BUFFSIZE);
//				int out_size = BUFFSIZE;
//				if (!Decode(packet.data, packet.size, out_data, out_size)) {
//					dec_cb(out_data, out_size);
//				}
//			}
//
//			av_packet_unref(&packet);
//
//			if (out_pkt_count >= 2)
//			{
//				printf("cur pkt(size:%d) only get 1 out packet, it get %d packets\n",
//					input_size, out_pkt_count);
//			}
//
//#else       // TS流可以直接写入
//			size_t size = fwrite(packet.data, 1, packet.size, outfp);
//			if (size != packet.size)
//			{
//				printf("fwrite failed-> write:%u, packet_size:%u\n", size, packet.size);
//			}
//			av_packet_unref(&packet);
//#endif
//		}
//		else
//		{
//			av_packet_unref(&packet);        // 释放内存
//		}
//	}
//
//	av_bsf_free(&bsf_ctx);

	SAFE_DEL_A(out_data);

	return 0;
}

int FFVideoDecoder::Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
	RETURN_V_IF(ffcodec_, -1);
	RETURN_V_IF(out_data, -1);

	// flush
	int ret = -1;
	if (!in_data) {
		ret = Flush(out_data, out_size);
		RETURN_V_IF(ret == 0, ret);
	}
	
	// decode
	ret = DoDecode(in_data, in_size, out_data, out_size);
	RETURN_V_IF(ret == 0, ret);

	return 0;
}

int FFVideoDecoder::Flush(uint8_t* out_data, int &out_size) {
	return DoDecode(nullptr, 0, out_data, out_size);
}

int FFVideoDecoder::DoDecode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
	// send packet and decode
	ffcodec_->avpkt_->data = (uint8_t *)in_data;
	ffcodec_->avpkt_->size = in_size;

	int ret = avcodec_send_packet(ffcodec_->avctx_, ffcodec_->avpkt_);
	if (ret != 0) {
		LOGE("[FFVideoDecoder] avcodec_send_packet fail return=" << ret);
		if (ret == AVERROR(EAGAIN))
			return -1;
		else if (ret == AVERROR_EOF)
			return -1;
		else if (ret == AVERROR(EINVAL))
			return -1;
		else if (ret == AVERROR(ENOMEM))
			return -1;
		else if (ret < 0) {
			fprintf(stderr, "[FFVideoDecoder] Error decoding video frame\n");
			return -1;
		}

		return -1;
	}

	AVPixelFormat out_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);

	// recieve frame
	int check = 0;
	while (ret >= 0) {
		ret = avcodec_receive_frame(ffcodec_->avctx_, ffcodec_->frame_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			LOGE("[FFVideoDecoder] avcodec_receive_frame, return=" << ret);
			break;
		}
		else if (ret < 0) {
			fprintf(stderr, "[FFVideoDecoder] Error during decoding\n");
			break;
		}

		if (!sws_isSupportedInput(ffcodec_->avctx_->pix_fmt)) {
			LOGE("[FFVideoDecoder] av_sws unsupported decoded pix_fmt=" << ffcodec_->avctx_->pix_fmt);
			break;
		}

		/*int ret = av_image_fill_arrays(ffcodec_->frame_t_->data, ffcodec_->frame_t_->linesize,
			in_data, in_pix_fmt, ffFmt_.width_, ffFmt_.height_, 1);
		RETURN_V_IF(ret > 0 && ret <= in_size, -1);*/

		// prepare output data (linesize and buffer)
		int dst_linesize[4] = { 0 };
		ret = av_image_fill_linesizes(dst_linesize, out_pix_fmt, ffFmt_.width_);
		RETURN_V_IF(ret >= 0, -1);

		uint8_t *dst_data[4] = { 0 };
		ret = av_image_fill_pointers(dst_data, out_pix_fmt, ffFmt_.height_, out_data, dst_linesize);
		RETURN_V_IF(ret > 0 && ret <= out_size, -1);
		out_size = ret; //actual output size if success

		// sws convert
		if (!ffcodec_->swsctx_) {
			sws_freeContext(ffcodec_->swsctx_);
			ffcodec_->swsctx_ = sws_getContext(ffcodec_->avctx_->width, ffcodec_->avctx_->height, ffcodec_->avctx_->pix_fmt,
				ffFmt_.width_, ffFmt_.height_, out_pix_fmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
			RETURN_V_IF(ffcodec_->swsctx_, -1);
		}

		ret = sws_scale(ffcodec_->swsctx_, ffcodec_->frame_->data, ffcodec_->frame_->linesize, \
			0, ffcodec_->frame_->height, dst_data, dst_linesize);
		RETURN_V_IF(ret == ffFmt_.height_, -1);

#if _DEBUG
		printf("[FFVideoDecoder] pts:%lld\t packet size:%d\n", ffcodec_->avpkt_->pts, ffcodec_->avpkt_->size);
#endif

		av_frame_unref(ffcodec_->frame_);

		check = 1;
	}

	return check? 0 : ret;
}


//############################################################################
//################FFAudioDecoder#################
//############################################################################
FFAudioDecoder::FFAudioDecoder(const FFAudioFormat& fmt) : FFDecoder(), ffFmt_(fmt) {
}
FFAudioDecoder::FFAudioDecoder(eFFCodecID_t codec_id, const FFAudioFormat& fmt): 
	FFDecoder(codec_id), ffFmt_(fmt){
}

FFAudioDecoder::~FFAudioDecoder() {
}

int FFAudioDecoder::Init() {
	//set out audio params
	uint64_t out_ch_layout = ffcodec_->get_channel_layout(ffFmt_.channel_layout_);
	int out_nb_samples = ffFmt_.frame_size_;
	AVSampleFormat out_sample_fmt = ffcodec_->get_AV_sample_format(ffFmt_.sample_fmt_);
	int out_sample_rate = ffFmt_.sample_rate_;
	int out_nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);
	// get out buffer size
	outbufsize_ = av_samples_get_buffer_size(nullptr, out_nb_channels, out_nb_samples, out_sample_fmt, 1);

	// fix: some Codec's Context information is missed
	if(!ffcodec_->avctx_->channel_layout)
		ffcodec_->avctx_->channel_layout = av_get_default_channel_layout(out_nb_channels);

	// set swr convert params
	ffcodec_->avctx_->sample_rate = ffFmt_.sample_rate_;	//make no change to the sample rate
	ffcodec_->swrctx_ = swr_alloc();
	ffcodec_->swrctx_ = swr_alloc_set_opts(ffcodec_->swrctx_, out_ch_layout, out_sample_fmt, out_sample_rate,
		ffcodec_->avctx_->channel_layout, ffcodec_->avctx_->sample_fmt, ffcodec_->avctx_->sample_rate, 0, nullptr);

	swr_init(ffcodec_->swrctx_);

	ffcodec_->frame_ = av_frame_alloc();
	ffcodec_->avpkt_ = av_packet_alloc();
	av_init_packet(ffcodec_->avpkt_);
	ffcodec_->avpkt_->data = nullptr;
	ffcodec_->avpkt_->size = 0;

	return 0;
}

int FFAudioDecoder::Open() {
	RETURN_V_IF(!ffcodec_, 0);

	ffcodec_ = new FFCodec(eFF_MEDIA_AUDIO);
	RETURN_V_IF(ffcodec_, -1);

	int nret = OpenCodec();
	if (nret != 0) {
		LOGE("[FFAudioDecoder] fail to open ff_codec_id=" << codec_id_ << ", return=" << nret);
		return -1;
	}

	return Init();
}

int FFAudioDecoder::Open(const std::string& in_file) {
	RETURN_V_IF(!ffcodec_, 0);

	ffcodec_ = new FFCodec(eFF_MEDIA_AUDIO);
	RETURN_V_IF(ffcodec_, -1);

	// open the audio file
	if (avformat_open_input(&ffcodec_->infmtCtx_, in_file.c_str(), nullptr, nullptr) < 0) {
		LOGE("[FFAudioDecoder] unable to open the audio file:" << in_file);
		return -1;
	}

	// get audio stream info
	if (avformat_find_stream_info(ffcodec_->infmtCtx_, nullptr) < 0) {
		LOGE("[FFAudioDecoder] unable to get the audio stream info!");
		return -1;
	}

	// get audio stream index
	for (int i = 0; i < ffcodec_->infmtCtx_->nb_streams; i++) {
		if (ffcodec_->infmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			streamIndex_ = i;
			break;
		}
	}
	if (streamIndex_ < 0) {
		LOGE("[FFAudioDecoder] unable to get the audio stream index!");
		return -1;
	}

	// get audio decoder params
	AVCodecParameters* codecParameters = ffcodec_->infmtCtx_->streams[streamIndex_]->codecpar;
	codec_id_ = FFCodec().get_FF_codecID(codecParameters->codec_id);

	ffcodec_->codec_ = avcodec_find_decoder(codecParameters->codec_id);
	if (!ffcodec_->codec_) {
		LOGE("[FFAudioDecoder] unable to open codec!");
		return -1;
	}

	ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
	if (!ffcodec_->avctx_) {
		LOGE("[FFAudioDecoder] unable to alloc avctx!");
		return -1;
	}

	avcodec_parameters_to_context(ffcodec_->avctx_, codecParameters);

	int nret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
	if (nret) {
		LOGE("[FFAudioDecoder] unable to open codex!");
		return -1;
	}
	
	return Init();
}

int FFAudioDecoder::Close() {
	ffFmt_.reset();

	return 0;
}

int FFAudioDecoder::Decode(ST_Dec_CB dec_cb) {
	const int BUFFSIZE = ffFmt_.channels_ * ffFmt_.sample_rate_;
	uint8_t* out_data = new uint8_t[BUFFSIZE];

	while (av_read_frame(ffcodec_->infmtCtx_, ffcodec_->avpkt_) >= 0) {
		if (ffcodec_->avpkt_->stream_index == streamIndex_) {
			LogPacket();

			memset(out_data, 0, BUFFSIZE);
			int out_size = BUFFSIZE;
			if (!Decode(ffcodec_->avpkt_->data, ffcodec_->avpkt_->size, out_data, out_size)) {
				dec_cb(out_data, out_size);
			}
		}
		av_packet_unref(ffcodec_->avpkt_);
	}

	 //1 获取相应的比特流过滤器
	 //FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加SPS/PPS等信息。
	 //FLV封装时，可以把多个NALU放在一个VIDEO TAG中,结构为4B NALU长度+NALU1+4B NALU长度+NALU2+...,
	 //需要做的处理把4B长度换成00000001或者000001

//	const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
//	AVBSFContext *bsf_ctx = NULL;
//	// 2 初始化过滤器上下文
//	av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;
//	 // 3 添加解码器属性
//	avcodec_parameters_copy(bsf_ctx->par_in, infmtCtx_->streams[audioStreamIndex]->codecpar);
//	av_bsf_init(bsf_ctx);
//
//	while (av_read_frame(infmtCtx_, &packet) >= 0)
//	{
//		if (packet.stream_index == audioStreamIndex)
//		{
//#if 1
//			int input_size = packet.size;
//			int out_pkt_count = 0;
//			// bitstreamfilter内部去维护内存空间
//			if (av_bsf_send_packet(bsf_ctx, &packet) != 0)
//			{
//				av_packet_unref(&packet);
//				continue;       // 继续送
//			}
//
//			while (av_bsf_receive_packet(bsf_ctx, &packet) == 0)
//			{
//				out_pkt_count++;
//				memset(out_data, 0, BUFFSIZE);
//				int out_size = BUFFSIZE;
//				if (!Decode(packet.data, packet.size, out_data, out_size)) {
//					dec_cb(out_data, out_size);
//				}
//			}
//
//			av_packet_unref(&packet);
//
//			if (out_pkt_count >= 2)
//			{
//				printf("cur pkt(size:%d) only get 1 out packet, it get %d packets\n",
//					input_size, out_pkt_count);
//			}
//
//#else       // TS流可以直接写入
//			size_t size = fwrite(packet.data, 1, packet.size, outfp);
//			if (size != packet.size)
//			{
//				printf("fwrite failed-> write:%u, packet_size:%u\n", size, packet.size);
//			}
//			av_packet_unref(&packet);
//#endif
//		}
//		else
//		{
//			av_packet_unref(&packet);        // 释放内存
//		}
//	}
//
//	av_bsf_free(&bsf_ctx);

	SAFE_DEL_A(out_data);

	return 0;
}

int FFAudioDecoder::Decode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
	RETURN_V_IF(ffcodec_, -1);
	RETURN_V_IF(in_data && out_data, -1);

	// flush
	int ret = -1;
	if (!in_data) {
		ret = Flush(out_data, out_size);
		RETURN_V_IF(ret == 0, ret);
	}

	// prepare input
	ret = DoDecode(in_data, in_size, out_data, out_size);
	RETURN_V_IF(ret == 0, ret);

	return ret;
}

int FFAudioDecoder::Flush(uint8_t* out_data, int &out_size) {
	return DoDecode(nullptr, 0, out_data, out_size);
}

int FFAudioDecoder::DoDecode(const uint8_t* in_data, int in_size, uint8_t* out_data, int &out_size) {
	// send packet and decode
	ffcodec_->avpkt_->data = (uint8_t *)in_data;
	ffcodec_->avpkt_->size = in_size;

	int ret = avcodec_send_packet(ffcodec_->avctx_, ffcodec_->avpkt_);
	if (ret != 0) {
		LOGE("[FFAudioDecoder] avcodec_send_packet fail return=" << iret);
		if (ret == AVERROR(EAGAIN))
			return -1;
		else if (ret == AVERROR_EOF)
			return -1;
		else if (ret == AVERROR(EINVAL))
			return -1;
		else if (ret == AVERROR(ENOMEM))
			return -1;
		else if (ret < 0) {
			fprintf(stderr, "[FFAudioDecoder] Error decoding audio frame\n");
			return -1;
		}

		return -1;
	}

	// recieve frame
	int check = 0;
	while (ret >= 0) {
		ret = avcodec_receive_frame(ffcodec_->avctx_, ffcodec_->frame_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			LOGE("[FFAudioDecoder] avcodec_receive_frame, return=" << ret);
			break;
		}
		else if (ret < 0) {
			fprintf(stderr, "[FFAudioDecoder] Error during decoding\n");
			break;
		}

		int numBytes = av_get_bytes_per_sample(ffcodec_->avctx_->sample_fmt);
		out_size = ffcodec_->frame_->nb_samples * ffcodec_->avctx_->channels * numBytes;

		if (ret == 0) {
			ret = swr_convert(ffcodec_->swrctx_, &out_data, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)ffcodec_->frame_->data, ffcodec_->frame_->nb_samples);
#if _DEBUG
			printf("[FFAudioDecoder] pts:%lld\t packet size:%d\n", ffcodec_->avpkt_->pts, ffcodec_->avpkt_->size);
#endif
		}

		check = 1;
	}

	return check ? 0 : ret;
}