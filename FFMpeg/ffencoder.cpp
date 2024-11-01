#include "stdafx.h"
#include "ffencoder.h"
#include "fflog.h"
#include "../AACParser.h"
#include <Windows.h>




char av_error_e[AV_TS_MAX_STRING_SIZE] = { 0 };
#define av_ts2str(ts) av_ts_make_string(av_error_e, ts)
#define av_ts2timestr(ts, tb) av_ts_make_time_string(av_error_e, ts, tb)
//############################################################################
//################FFEncoder#################
//############################################################################
FILE* FFEncoder::plogf_ = nullptr;

FFEncoder::FFEncoder() {
	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback(AvLog_cb);
	strlogfile_ = "FFmpegEnCodecLog.txt";

	if (!plogf_)
		fopen_s(&plogf_, strlogfile_.c_str(), "a+");
}

FFEncoder::FFEncoder(eFFCodecID_t codec_id): codec_id_(codec_id){
	av_log_set_level(AV_LOG_DEBUG);
	av_log_set_callback(AvLog_cb);
	strlogfile_ = "FFmpegEnCodecLog.txt";

	if (!plogf_)
		fopen_s(&plogf_, strlogfile_.c_str(), "a+");
}

FFEncoder::~FFEncoder() {
	if (plogf_) {
		fclose(plogf_);
		plogf_ = nullptr;
	}

	SAFE_DEL(ffcodec_);
}

void FFEncoder::AvLog_cb(void *avcl, int level, const char *fmt, va_list vl)
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

int FFEncoder::Init() {
	ffcodec_->avpkt_ = av_packet_alloc();
	av_init_packet(ffcodec_->avpkt_);
	ffcodec_->avpkt_->data = nullptr;
	ffcodec_->avpkt_->size = 0;

	return 0;
}

int FFEncoder::OpenCodec() {
	RETURN_V_IF(ffcodec_, -1);

	AVCodecID av_codec_id = ffcodec_->get_AV_codecID(codec_id_);
	RETURN_V_IF(av_codec_id != AV_CODEC_ID_NONE, -1);

	ffcodec_->codec_ = avcodec_find_encoder(av_codec_id);
	//ffcodec_->codec_ = avcodec_find_decoder_by_name("libfdk_aac");

	RETURN_V_IF(ffcodec_->codec_, -1);
	ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
	RETURN_V_IF(ffcodec_->avctx_, -1);

	return 0;
}

void FFEncoder::LogPacket() {
	AVFormatContext *ctx = ffcodec_->outfmtCtx_;
	AVPacket *pkt = ffcodec_->avpkt_;
	AVRational *time_base = &(ctx->streams[pkt->stream_index]->time_base);

	printf("[FFEncoder] pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		pkt->stream_index);
}

int FFEncoder::AddStream() {
	// output stream
	int ret = avio_open(&ffcodec_->outfmtCtx_->pb, ffcodec_->outfmtCtx_->filename, AVIO_FLAG_WRITE);
	if (ret < 0) {
		printf("[FFEncoder] avio_open out file : %s, fail! \n", ffcodec_->outfmtCtx_->filename);
		RETURN_V_IF(ret >= 0, -1);
	}

	ffcodec_->avStream_ = avformat_new_stream(ffcodec_->outfmtCtx_, ffcodec_->codec_);
	ffcodec_->avStream_->id = ffcodec_->outfmtCtx_->nb_streams - 1;
	ffcodec_->avStream_->time_base = ffcodec_->avctx_->time_base;

	ret = avcodec_parameters_from_context(ffcodec_->avStream_->codecpar, ffcodec_->avctx_);
	if (ret < 0) {
		printf("[FFEncoder] avcodec_parameters_from_context fail! \n");
		RETURN_V_IF(ret >= 0, -1);
	}

	/* Some formats want stream headers to be separate. */
	/*if (ffcodec_->outfmtCtx_->oformat->flags & AVFMT_GLOBALHEADER)
	ffcodec_->avctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;*/

	return ret;
}


//############################################################################
//################FFVideoEncoder#################
//############################################################################
FFVideoEncoder::FFVideoEncoder(const FFVideoFormat &fmt) :
	FFEncoder(), ffFmt_(fmt) {
	ffcodec_ = new FFCodec(eFF_MEDIA_VIDEO);
}

FFVideoEncoder::FFVideoEncoder(eFFCodecID_t codec_id, const FFVideoFormat &fmt): 
	FFEncoder(codec_id), ffFmt_(fmt){
	ffcodec_ = new FFCodec(eFF_MEDIA_VIDEO);
}

FFVideoEncoder::~FFVideoEncoder() {
}

int FFVideoEncoder::SetParams() {
	// set in ctx params
	ffcodec_->avctx_->bit_rate = ffFmt_.bitrate_;
	ffcodec_->avctx_->width = ffFmt_.width_;
	ffcodec_->avctx_->height = ffFmt_.height_;
	ffcodec_->avctx_->time_base = AVRational{ 1, ffFmt_.fps_ };
	ffcodec_->avctx_->framerate = AVRational{ ffFmt_.fps_, 1 };

	ffcodec_->avctx_->gop_size = ffFmt_.data_.gop_size_;
	ffcodec_->avctx_->max_b_frames = ffFmt_.data_.max_b_frames_;

	AVPixelFormat pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);
	if (!ffcodec_->check_pix_fmt(pix_fmt)) {
		ffcodec_->avctx_->pix_fmt = ffcodec_->get_pix_fmt();
		LOGW("[FFVideoEncoder] unsupported (ff_pix_fmt=" << ffFmt_.pix_fmt_ << ", pix_fmt=" << pix_fmt << ")" << ", and select from codec=" << ffcodec_->avctx_->pix_fmt);
	}
	else {
		ffcodec_->avctx_->pix_fmt = pix_fmt;
	}

	// for codec private data
	if (ffcodec_->avctx_->codec_id == AV_CODEC_ID_H264) {
		av_opt_set(ffcodec_->avctx_->priv_data, "preset", "fast", 0);
	}
	/* just for testing, we also add B-frames */
	if (ffcodec_->avctx_->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
		ffcodec_->avctx_->max_b_frames = 2;
	}
	/* Needed to avoid using macroblocks in which some coeffs overflow.
	* This does not happen with normal video, it just happens here as
	* the motion of the chroma plane does not match the luma plane. */
	if (ffcodec_->avctx_->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
		ffcodec_->avctx_->mb_decision = 2;
	}

	return 0;
}

int FFVideoEncoder::Open() {
	int ret = OpenCodec();
	if (ret != 0) {
		LOGE("[FFVideoEncoder] fail to open ff_codec_id=" << codec_id_ << ", return=" << ret);
		RETURN_V_IF(ret == 0, -1);
	}

	ret = SetParams();
	RETURN_V_IF(ret == 0, -1);

	AVPixelFormat in_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);
	// convert pix_fmt
	if (!sws_isSupportedInput(in_pix_fmt) || !sws_isSupportedOutput(ffcodec_->avctx_->pix_fmt)) {
		LOGE("[FFVideoEncoder] av_sws unsupported from av_pix_fmt=" << in_pix_fmt << " to av_pix_fmt=" << ffcodec_->avctx_->pix_fmt);
		return -1;
	}

	ffcodec_->swsctx_ = sws_getContext(ffFmt_.width_, ffFmt_.height_, in_pix_fmt,
		ffcodec_->avctx_->width, ffcodec_->avctx_->height, ffcodec_->avctx_->pix_fmt, SWS_FAST_BILINEAR,
		nullptr, nullptr, nullptr);
	RETURN_V_IF(ffcodec_->swsctx_, -1);

	ret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
	if (ret < 0) {
		printf("[FFVideoEncoder] avcodec_open2 fail! \n");
		RETURN_V_IF(ret == 0, -1);
	}

	Init();

	return ret;
}

int FFVideoEncoder::Open(const std::string& out_file) {
	int ret = -1;
	// open the output ctx
	ret = avformat_alloc_output_context2(&ffcodec_->outfmtCtx_, NULL, NULL, out_file.c_str());
	if (ret < 0) {
		LOGE("[FFVideoEncoder] unable to open the output ctx with out file:" << out_file);
		RETURN_V_IF(ret >= 0, -1);
	}
	
	ret = Open(ffcodec_->outfmtCtx_);
	RETURN_V_IF(ret >= 0, -1);

	ret = avformat_write_header(ffcodec_->outfmtCtx_, NULL);
	if (!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT)) {
		printf("[FFVideoEncoder] avformat_write_header fail! \n");
		RETURN_V_IF(!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT), -1);
	}

	return ret;
}

int FFVideoEncoder::Open(AVFormatContext* av_fmt_ctx) {
	int ret = -1;

	ffcodec_->outfmtCtx_ = av_fmt_ctx;

	codec_id_ = FFCodec().get_FF_codecID(ffcodec_->outfmtCtx_->oformat->video_codec);

	ffcodec_->codec_ = avcodec_find_encoder(ffcodec_->outfmtCtx_->oformat->video_codec);
	if (!ffcodec_->codec_) {
		printf("[FFVideoEncoder] avcodec_find_encoder fail! \n");
		RETURN_V_IF(ffcodec_->codec_, -1);
	}
	ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
	RETURN_V_IF(ffcodec_->avctx_, -1);

	ret = SetParams();
	RETURN_V_IF(ret == 0, -1);

	ret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, NULL);
	if (ret < 0) {
		printf("[FFVideoEncoder] avcodec_open2 fail! \n");
		RETURN_V_IF(ret == 0, -1);
	}

	ret = AddStream();
	RETURN_V_IF(ret == 0, -1);

	AVPixelFormat in_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);
	// convert pix_fmt
	if (!sws_isSupportedInput(in_pix_fmt) || !sws_isSupportedOutput(ffcodec_->avctx_->pix_fmt)) {
		LOGE("[FFVideoEncoder] av_sws unsupported from av_pix_fmt=" << in_pix_fmt << " to av_pix_fmt=" << ffcodec_->avctx_->pix_fmt);
		return -1;
	}

	ffcodec_->swsctx_ = sws_getContext(ffFmt_.width_, ffFmt_.height_, in_pix_fmt,
		ffcodec_->avctx_->width, ffcodec_->avctx_->height, ffcodec_->avctx_->pix_fmt, SWS_FAST_BILINEAR,
		nullptr, nullptr, nullptr);
	RETURN_V_IF(ffcodec_->swsctx_, -1);

	Init();

	return ret;
}

int FFVideoEncoder::Close() {
	if (ffcodec_ && ffcodec_->outfmtCtx_) {
		int ret = av_write_trailer(ffcodec_->outfmtCtx_);
		if (ret < 0) {
			printf("[FFVideoEncoder] av_write_trailer fail! \n");
		}
	}

	ffFmt_.reset();

	return 0;
}

int FFVideoEncoder::Encode(const uint8_t *in_data, int in_size) {
	const int EN_BUFSIZE = 16;
	uint8_t enc_buf[EN_BUFSIZE] = { 0 };
	int out_size = EN_BUFSIZE;
	return Encode(in_data, in_size, enc_buf, out_size);
}

int FFVideoEncoder::Encode(const uint8_t *in_data, int in_size, uint8_t* out_data, int &out_size) {
	RETURN_V_IF(ffcodec_, -1);
	RETURN_V_IF(out_data, -1);

	// flush delayed frames
	if (!in_data) {
		int ret = Flush(out_data, out_size);
		RETURN_V_IF(ret == 0, ret);
	}

	// check input format
	AVPixelFormat in_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);
	if (in_pix_fmt == AV_PIX_FMT_NONE) {
		LOGE("[FFVideoEncoder] unsupported format ff_pix_fmt=" << ffFmt_.pix_fmt_);
		return -1;
	}

	// prepare input frame
	AVFrame* frame = nullptr;
	int ret = PrepareFrame(in_data, in_size, &frame);
	RETURN_V_IF(ret == 0, ret);

	frame->pts = frame_count_++;

	return DoEncode(out_data, out_size, frame);
}

int FFVideoEncoder::Flush(uint8_t* out_data, int &out_size) {
	return DoEncode(out_data, out_size, nullptr);
}

int FFVideoEncoder::DoEncode(uint8_t* out_data, int &out_size, AVFrame* frame) {
	int ret = avcodec_send_frame(ffcodec_->avctx_, frame);
	if (ret != 0) {
		LOGE("[FFVideoEncoder] codec_send_frame fail ret=" << ret);
		if (ret == AVERROR(EAGAIN))
			return -1;
		else if (ret == AVERROR_EOF)
			return -1;
		else if (ret == AVERROR(EINVAL))
			return -1;
		else if (ret == AVERROR(ENOMEM))
			return -1;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding video frame\n");
			return -1;
		}

		return -1;
	}

	// read all the available output packets in general there may be any number of them 
	int offset = 0;
	while (ret >= 0) {
		ret = avcodec_receive_packet(ffcodec_->avctx_, ffcodec_->avpkt_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return offset > 0 ? 0 : -1;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding video frame\n");
			return -1;
		}

		if (ffcodec_->outfmtCtx_) {
			offset += ffcodec_->avpkt_->size;
			
			av_packet_rescale_ts(ffcodec_->avpkt_, ffcodec_->avctx_->time_base, ffcodec_->avStream_->time_base);
			ffcodec_->avpkt_->stream_index = ffcodec_->avStream_->index;

#ifdef _DEBUG
			LogPacket();
#endif

			ret = av_interleaved_write_frame(ffcodec_->outfmtCtx_, ffcodec_->avpkt_);
			if (ret < 0) {
				printf("[FFVideoEncoder] av_interleaved_write_frame fail \n");
			}
		}
		else {
			memcpy(out_data + offset, ffcodec_->avpkt_->data, ffcodec_->avpkt_->size);
			offset += ffcodec_->avpkt_->size;
			out_size = offset;
		}
		
		av_packet_unref(ffcodec_->avpkt_);
	}

	return offset? 0: ret;
}

int FFVideoEncoder::PrepareFrame(const uint8_t* in_data, int in_size, AVFrame** frame) {
	AVPixelFormat in_pix_fmt = ffcodec_->get_AV_pixel_format(ffFmt_.pix_fmt_);

	if (!ffcodec_->frame_) {
		ffcodec_->frame_ = av_frame_alloc();
		ffcodec_->frame_->format = ffcodec_->avctx_->pix_fmt;
		ffcodec_->frame_->width = ffcodec_->avctx_->width;
		ffcodec_->frame_->height = ffcodec_->avctx_->height;

		/* allocate the data buffers */
		int ret = av_frame_get_buffer(ffcodec_->frame_, 1);
		RETURN_V_IF(ret == 0, -1);
	}

	if (in_pix_fmt != ffcodec_->avctx_->pix_fmt ||
		ffFmt_.width_ != ffcodec_->avctx_->width ||
		ffFmt_.height_ != ffcodec_->avctx_->height) {
		// prepare sws output frame
		if (!ffcodec_->frame_t_) {
			ffcodec_->frame_t_ = av_frame_alloc();
			ffcodec_->frame_t_->width = ffFmt_.width_;
			ffcodec_->frame_t_->height = ffFmt_.height_;
			ffcodec_->frame_t_->format = in_pix_fmt;

			/* allocate the data buffers */
			int ret = av_frame_get_buffer(ffcodec_->frame_t_, 1);
			RETURN_V_IF(ret == 0, -1);
		}

		// prepare sws input data
		int ret = av_image_fill_arrays(ffcodec_->frame_t_->data, ffcodec_->frame_t_->linesize,
			in_data, in_pix_fmt, ffFmt_.width_, ffFmt_.height_, 1);
		RETURN_V_IF(ret > 0 && ret <= in_size, -1);

		// sws convert (for input frame)
		ret = sws_scale(ffcodec_->swsctx_, ffcodec_->frame_t_->data, ffcodec_->frame_t_->linesize, 0, ffcodec_->frame_t_->height, //输入
			ffcodec_->frame_->data, ffcodec_->frame_->linesize);	//输出
		RETURN_V_IF(ret == ffcodec_->avctx_->height, -1);
	}
	else {
		// prepare raw input data
		int ret = av_image_fill_arrays(ffcodec_->frame_->data, ffcodec_->frame_->linesize,
			in_data, in_pix_fmt, ffFmt_.width_, ffFmt_.height_, 1);
		RETURN_V_IF(ret >= 0 && ret <= in_size, -1);
	}

	*frame = ffcodec_->frame_;

	return 0;
}

//############################################################################
//################FFAudioEncoder#################
//############################################################################
FFAudioEncoder::FFAudioEncoder(const FFAudioFormat &fmt) :
	FFEncoder(), ffFmt_(fmt) {
	ffcodec_ = new FFCodec(eFF_MEDIA_AUDIO);
}

FFAudioEncoder::FFAudioEncoder(eFFCodecID_t codec_id, const FFAudioFormat &fmt) : 
	FFEncoder(codec_id), ffFmt_(fmt) {
	ffcodec_ = new FFCodec(eFF_MEDIA_AUDIO);
}

FFAudioEncoder::~FFAudioEncoder() {
}

int FFAudioEncoder::SetParams() {
	// set out codec params
	const int64_t in_bit_rate = ffFmt_.bitrate_;
	AVSampleFormat in_sample_fmt = ffcodec_->get_AV_sample_format(ffFmt_.sample_fmt_);
	const uint64_t in_ch_layout = ffcodec_->get_channel_layout(ffFmt_.channel_layout_);
	const int in_sample_rate = ffFmt_.sample_rate_;

	if (!ffcodec_->check_sample_fmt(in_sample_fmt)) {
		ffcodec_->avctx_->sample_fmt = ffcodec_->select_sample_fmt();
		LOGW("[FFAudioEncoder] unsupported (ff_sample_fmt=" << ffFmt_.sample_fmt_ << ", sample_fmt=" << in_sample_fmt << ")"
			<< ", and select from codec=" << ffcodec_->avctx_->sample_fmt);
	}
	else {
		ffcodec_->avctx_->sample_fmt = in_sample_fmt;
	}

	if (!ffcodec_->check_sample_rate(ffFmt_.sample_rate_)) {
		ffcodec_->avctx_->sample_rate = ffcodec_->get_sample_rate();
		LOGW("[FFAudioEncoder] unsupported sample_rate=" << ffFmt_.sample_rate_ << ", and select from codec=" << ffcodec_->avctx_->sample_rate);
	}
	else {
		ffcodec_->avctx_->sample_rate = ffFmt_.sample_rate_;
	}

	ffcodec_->avctx_->time_base = AVRational{ 1, ffcodec_->avctx_->sample_rate };
	ffcodec_->avctx_->bit_rate = in_bit_rate;
	//ffcodec_->avctx_->profile = ffcodec_->get_profile();

	if (!ffcodec_->check_channel_layout(in_ch_layout)) {
		ffcodec_->avctx_->channel_layout = ffcodec_->get_channel_layout();
		LOGW("[FFAudioEncoder] unsupported channel_layout=" << ffFmt_.sample_rate_ << ", and select from codec=" << ffcodec_->avctx_->sample_rate);
	}
	else {
		ffcodec_->avctx_->channel_layout = in_ch_layout;
	}

	ffcodec_->avctx_->channels = av_get_channel_layout_nb_channels(ffcodec_->avctx_->channel_layout);
	if (ffFmt_.channels_ != ffcodec_->avctx_->channels) {
		LOGW("[FFAudioEncoder] require channels=" << ffFmt_.channels_ << ", but select from codec=" << ffcodec_->avctx_->channels);
	}

	return 0;
}

int FFAudioEncoder::Open() {
	int ret = OpenCodec();
	if (ret != 0) {
		LOGE("[FFAudioEncoder] fail to open ff_codec_id=" << codec_id_ << ", return=" << ret);
		RETURN_V_IF(ret == 0, -1);
	}

	ret = SetParams();
	RETURN_V_IF(ret == 0, -1);

	const int64_t in_bit_rate = ffFmt_.bitrate_;
	AVSampleFormat in_sample_fmt = ffcodec_->get_AV_sample_format(ffFmt_.sample_fmt_);
	const uint64_t in_ch_layout = ffcodec_->get_channel_layout(ffFmt_.channel_layout_);
	const int in_sample_rate = ffFmt_.sample_rate_;

	ffcodec_->swrctx_ = swr_alloc_set_opts(ffcodec_->swrctx_,
		ffcodec_->avctx_->channel_layout, ffcodec_->avctx_->sample_fmt, ffcodec_->avctx_->sample_rate, // 输出
		in_ch_layout, in_sample_fmt, in_sample_rate, // 输入
		0, 0);
	ret = swr_init(ffcodec_->swrctx_);
	RETURN_V_IF(ret == 0, -1);

	ret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
	RETURN_V_IF(ret == 0, -1);

	Init();

	return ret;
}

int FFAudioEncoder::Open(const std::string& out_file) {
	int ret = -1;
	// open the output ctx
	ret = avformat_alloc_output_context2(&ffcodec_->outfmtCtx_, NULL, NULL, out_file.c_str());
	if (ret < 0) {
		LOGE("[FFAudioEncoder] unable to open the output ctx with out file:" << out_file);
		RETURN_V_IF(ret >= 0, -1);
	}

	ret = Open(ffcodec_->outfmtCtx_);
	RETURN_V_IF(ret >= 0, -1);
	
	ret = avformat_write_header(ffcodec_->outfmtCtx_, NULL);
	if (!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT)) {
		printf("[FFAudioEncoder] avformat_write_header fail! \n");
		RETURN_V_IF(!(ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT), -1);
	}

	return ret;
}

int FFAudioEncoder::Open(AVFormatContext* av_fmt_ctx) {
	int ret = -1;

	ffcodec_->outfmtCtx_ = av_fmt_ctx;

	codec_id_ = FFCodec().get_FF_codecID(ffcodec_->outfmtCtx_->oformat->audio_codec);

	ffcodec_->codec_ = avcodec_find_encoder(ffcodec_->outfmtCtx_->oformat->audio_codec);
	if (!ffcodec_->codec_) {
		printf("[FFAudioEncoder] avcodec_find_encoder fail! \n");
		RETURN_V_IF(ffcodec_->codec_, -1);
	}
	ffcodec_->avctx_ = avcodec_alloc_context3(ffcodec_->codec_);
	RETURN_V_IF(ffcodec_->avctx_, -1);

	ret = SetParams();
	RETURN_V_IF(ret == 0, -1);

	ret = avcodec_open2(ffcodec_->avctx_, ffcodec_->codec_, nullptr);
	if (ret < 0) {
		printf("[FFAudioEncoder] avcodec_open2 fail! \n");
		RETURN_V_IF(ret >= 0, -1);
	}

	ret = AddStream();
	RETURN_V_IF(ret == 0, -1);

	const int64_t in_bit_rate = ffFmt_.bitrate_;
	AVSampleFormat in_sample_fmt = ffcodec_->get_AV_sample_format(ffFmt_.sample_fmt_);
	const uint64_t in_ch_layout = ffcodec_->get_channel_layout(ffFmt_.channel_layout_);
	const int in_sample_rate = ffFmt_.sample_rate_;

	ffcodec_->swrctx_ = swr_alloc_set_opts(ffcodec_->swrctx_,
		ffcodec_->avctx_->channel_layout, ffcodec_->avctx_->sample_fmt, ffcodec_->avctx_->sample_rate, // 输出
		in_ch_layout, in_sample_fmt, in_sample_rate, // 输入
		0, 0);
	ret = swr_init(ffcodec_->swrctx_);
	RETURN_V_IF(ret == 0, -1);

	Init();

	return ret;
}

int FFAudioEncoder::Close() {
	if (ffcodec_ && ffcodec_->outfmtCtx_) {
		int ret = av_write_trailer(ffcodec_->outfmtCtx_);
		if (ret < 0) {
			printf("[FFAudioEncoder] av_write_trailer fail! \n");
		}
	}

	ffFmt_.reset();

	return 0;
}

int FFAudioEncoder::Encode(const uint8_t *in_data, int in_size) {
	const int EN_BUFSIZE = 16;
	uint8_t enc_buf[EN_BUFSIZE] = { 0 };
	int out_size = EN_BUFSIZE;

	return Encode(in_data, in_size, enc_buf, out_size);
}

int FFAudioEncoder::Encode(const uint8_t *in_data, int in_size, uint8_t* out_data, int &out_size) {
	RETURN_V_IF(ffcodec_, -1);
	RETURN_V_IF(out_data, -1);

	/*int buf_size = av_samples_get_buffer_size(nullptr, ffcodec_->avctx_->channels, ffcodec_->avctx_->frame_size, ffcodec_->avctx_->sample_fmt, 0);*/

	// flush delayed frames
	int ret = -1;
	if (!in_data) {
		ret = Flush(out_data, out_size);
		RETURN_V_IF(ret == 0, ret);
	}

	// prepare input frame
	AVFrame* frame = nullptr;
	ret = PrepareFrame(in_data, in_size, &frame);
	RETURN_V_IF(ret == 0, ret);

	//frame->pts = frame_count_;
	frame->pts = av_rescale_q(frame_count_, AVRational{ 1, ffcodec_->avctx_->sample_rate }, ffcodec_->avctx_->time_base);
	frame_count_ += frame->nb_samples;

	return DoEncode(out_data, out_size, frame);
}

int FFAudioEncoder::Flush(uint8_t* out_data, int &out_size) {
	return DoEncode(out_data, out_size, nullptr);
}

int FFAudioEncoder::DoEncode(uint8_t* out_data, int &out_size, AVFrame* frame) {
	int ret = avcodec_send_frame(ffcodec_->avctx_, frame);
	if (ret != 0) {
		LOGE("[FFAudioEncoder] codec_send_frame fail ret=" << ret);
		if (ret == AVERROR(EAGAIN))
			return -1;
		else if (ret == AVERROR_EOF)
			return -1;
		else if (ret == AVERROR(EINVAL))
			return -1;
		else if (ret == AVERROR(ENOMEM))
			return -1;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding audio frame\n");
			return -1;
		}

		return -1;
	}

	/* read all the available output packets (in general there may be any
	* number of them */
	int offset = 0;
	while (ret >= 0) {
		ret = avcodec_receive_packet(ffcodec_->avctx_, ffcodec_->avpkt_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return offset > 0 ? 0 : -1;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding audio frame\n");
			return -1;
		}
		
		if (ffcodec_->outfmtCtx_) {
			offset += ffcodec_->avpkt_->size;

			av_packet_rescale_ts(ffcodec_->avpkt_, ffcodec_->avctx_->time_base, ffcodec_->avStream_->time_base);
			ffcodec_->avpkt_->stream_index = ffcodec_->avStream_->index;

#ifdef _DEBUG
			LogPacket();
#endif
			ret = av_interleaved_write_frame(ffcodec_->outfmtCtx_, ffcodec_->avpkt_);
			if (ret < 0) {
				printf("[FFAudioEncoder] av_interleaved_write_frame fail \n");
			}
		}
		else {
			memcpy(out_data, ffcodec_->avpkt_->data, ffcodec_->avpkt_->size);
			offset = out_size = ffcodec_->avpkt_->size;
		}
		
		av_packet_unref(ffcodec_->avpkt_);
	}

	return offset ? 0 : ret;
}

int FFAudioEncoder::PrepareFrame(const uint8_t *in_data, int in_size, AVFrame **pframe) {
	const int64_t in_bit_rate = ffFmt_.bitrate_;
	AVSampleFormat in_sample_fmt = ffcodec_->get_AV_sample_format(ffFmt_.sample_fmt_);
	const uint64_t in_ch_layout = ffcodec_->get_channel_layout(ffFmt_.channel_layout_);
	const int in_sample_rate = ffFmt_.sample_rate_;
	const int in_frame_size = ffFmt_.frame_size_;

	if (!ffcodec_->frame_) {
		ffcodec_->frame_ = av_frame_alloc();

		if (!ffcodec_->frame_) {
			fprintf(stderr, "Could not allocate audio frame\n");
			return -1;
		}

		ffcodec_->frame_->nb_samples = ffcodec_->avctx_->frame_size;
		ffcodec_->frame_->format = ffcodec_->avctx_->sample_fmt;
		ffcodec_->frame_->channel_layout = ffcodec_->avctx_->channel_layout;
		ffcodec_->frame_->sample_rate = ffcodec_->avctx_->sample_rate;

		/* allocate the data buffers */
		int ret = av_frame_get_buffer(ffcodec_->frame_, 0);
		if (ret < 0) {
			fprintf(stderr, "Could not allocate audio data buffers\n");
			return -1;
		}
	}

	if (in_sample_fmt != ffcodec_->avctx_->sample_fmt ||
		in_ch_layout != ffcodec_->avctx_->channel_layout ||
		in_frame_size != ffcodec_->avctx_->frame_size) {
		if (!ffcodec_->frame_t_) {
			ffcodec_->frame_t_ = av_frame_alloc();

			if (!ffcodec_->frame_t_) {
				fprintf(stderr, "Could not allocate audio frame\n");
				return -1;
			}

			// set input and output frame size the same.
			ffcodec_->frame_t_->nb_samples = in_frame_size;
			ffcodec_->frame_t_->format = in_sample_fmt;
			ffcodec_->frame_t_->channel_layout = in_ch_layout;
			ffcodec_->frame_t_->sample_rate = in_sample_rate;

			/* allocate the data buffers */
			int ret = av_frame_get_buffer(ffcodec_->frame_t_, 0);
			if (ret < 0) {
				fprintf(stderr, "Could not allocate audio data buffers\n");
				return -1;
			}
		}

		int nb_samples = in_size / (av_get_bytes_per_sample((AVSampleFormat)ffcodec_->frame_t_->format)*ffcodec_->frame_t_->channels);
		ffcodec_->frame_t_->nb_samples = nb_samples;
		ffcodec_->frame_->nb_samples = nb_samples;

		av_samples_fill_arrays(ffcodec_->frame_t_->data, ffcodec_->frame_t_->linesize, in_data, ffcodec_->frame_t_->channels, ffcodec_->frame_t_->nb_samples, (AVSampleFormat)ffcodec_->frame_t_->format, 0);


		// 音频重采样
		int ret = swr_convert(ffcodec_->swrctx_,
			ffcodec_->frame_->data, ffcodec_->frame_->nb_samples, // 输出
			(const uint8_t**)ffcodec_->frame_t_->data, ffcodec_->frame_t_->nb_samples); // 输入
		if (ret <= 0) {
			LOGE("[FFAudioEncoder] swr_convert fail ret=" << ret);
			RETURN_V_IF(ret == ffcodec_->frame_->nb_samples, -1);
		}
	}
	else {
		av_samples_fill_arrays(ffcodec_->frame_->data, ffcodec_->frame_->linesize, in_data, ffcodec_->frame_->channels, ffcodec_->frame_->nb_samples, (AVSampleFormat)ffcodec_->frame_->format, 0);
	}	

	/*ret = avcodec_fill_audio_frame(ffcodec_->frame_, ffcodec_->avctx_->channels, ffcodec_->avctx_->sample_fmt, in_data, in_size, 0);
	RETURN_V_IF(ret >= 0, -1);*/
	
	*pframe = ffcodec_->frame_;
	return 0;
}

void FFAudioEncoder::F32leConvert2Fltp(float *f32le, float *fltp, int nb_samples) {
	float *fltp_l = fltp;   // 左通道
	float *fltp_r = fltp + nb_samples;   // 右通道
	for (int i = 0; i < nb_samples; i++) {
		fltp_l[i] = f32le[i * 2];     // 0 1   - 2 3
		fltp_r[i] = f32le[i * 2 + 1];   // 可以尝试注释左声道或者右声道听听声音
	}
}