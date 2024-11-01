#include "stdafx.h"
#include "ffcodec.h"


// ffmpeg libs
#pragma comment(lib, "avformat.lib")  
#pragma comment(lib, "avcodec.lib")  
#pragma comment(lib, "avutil.lib") 
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")


FFCodec::FFCodec(eFFMedia_t etype) {
	eFFmedia_t_ = etype;
	open();
}

FFCodec::~FFCodec() {
	close();
}

int FFCodec::open() {
	av_register_all();
	return 0;
}

int FFCodec::close() {
	codec_ = nullptr;
	if (avctx_) {
		avcodec_close(avctx_);
		av_free(avctx_);
		avctx_ = nullptr;
	}
	if (avpkt_) {
		av_packet_unref(avpkt_);
		avpkt_ = nullptr;
	}
	if (frame_) {
		av_frame_free(&frame_);
		frame_ = nullptr;
	}
	if (frame_t_) {
		av_frame_free(&frame_t_);
		frame_t_ = nullptr;
	}
	if (swsctx_) {
		sws_freeContext(swsctx_);
		swsctx_ = nullptr;
	}
	if (swrctx_) {
		swr_free(&swrctx_);
		swrctx_ = nullptr;
	}
	if (infmtCtx_) {
		avformat_close_input(&infmtCtx_);
		avformat_free_context(infmtCtx_);
		infmtCtx_ = nullptr;
	}
	if (outfmtCtx_) {
		avformat_close_input(&outfmtCtx_);
		avformat_free_context(outfmtCtx_);
		outfmtCtx_ = nullptr;
	}

	return 0;
}

AVPixelFormat FFCodec::get_AV_pixel_format(eFFPixelFormat_t pix_fmt) {
	int N = FF_ARRAY_ELEMS(v_pix_fmts);
    for (int i = 0; i < N; i++) {
        const Pix_Fmt_t *entry = &v_pix_fmts[i];
        if (pix_fmt == entry->e_pix_fmt)
            return entry->av_pix_fmt;
    }
    return AV_PIX_FMT_NONE;
}

eFFPixelFormat_t FFCodec::get_FF_pixel_format(AVPixelFormat pix_fmt) {
	int N = FF_ARRAY_ELEMS(v_pix_fmts);
    for (int i = 0; i < N; i++) {
        const Pix_Fmt_t *entry = &v_pix_fmts[i];
        if (pix_fmt == entry->av_pix_fmt)
            return entry->e_pix_fmt;
    }
    return eFF_PIX_FMT_NONE;
}

AVCodecID FFCodec::get_AV_codecID(eFFCodecID_t codec_id) {
	int N = FF_ARRAY_ELEMS(av_codec_ids);
    for (int i = 0; i < N; i++) {
        const Codec_ID_t *entry = &av_codec_ids[i];
        if (codec_id == entry->codec_id)
            return entry->av_codec_id;
    }
    return AV_CODEC_ID_NONE;
}

eFFCodecID_t FFCodec::get_FF_codecID(AVCodecID codec_id) {
	int N = FF_ARRAY_ELEMS(av_codec_ids);
    for (int i = 0; i < N; i++) {
        const Codec_ID_t *entry = &av_codec_ids[i];
        if (codec_id == entry->av_codec_id)
            return entry->codec_id;
    }
    return eFF_CODEC_ID_NONE;
}

AVSampleFormat FFCodec::get_AV_sample_format(eFFSampleFormat_t sample_fmt) {
	int N = FF_ARRAY_ELEMS(a_sample_fmts);
    for (int i = 0; i < N; i++) {
        const Sample_Fmt_t *entry = &a_sample_fmts[i];
        if (sample_fmt == entry->e_sample_fmt)
            return entry->av_sample_fmt;
    }
    return AV_SAMPLE_FMT_NONE;
}

eFFSampleFormat_t FFCodec::get_FF_sample_format(AVSampleFormat sample_fmt) {
	int N = FF_ARRAY_ELEMS(a_sample_fmts);
    for (int i = 0; i < N; i++) {
        const Sample_Fmt_t *entry = &a_sample_fmts[i];
        if (sample_fmt == entry->av_sample_fmt)
            return entry->e_sample_fmt;
    }
    return eFF_SAMPLE_FMT_NONE;
}

int FFCodec::check_sample_fmt(enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec_->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }   
    return 0;
}

AVSampleFormat FFCodec::select_sample_fmt()
{
	const enum AVSampleFormat *p = codec_->sample_fmts;

    return *p;
}

int FFCodec::check_sample_rate(int sample_rate)
{
	const int *p= codec_->supported_samplerates;
    if (!p){
        if (sample_rate == 44100)
            return 1;
        return 0;
    }

    while (*p) {
        if (*p == sample_rate)
            return 1;
        p++;
    }
    return 0;
}

int FFCodec::get_sample_rate()
{
	const int *p = codec_->supported_samplerates;
    if (!p)
        return 44100;

	int best_samp_rate = 0;
    while (*p) {
        best_samp_rate = FFMAX(*p, best_samp_rate);
        p++;
    }
    return best_samp_rate;
}

int FFCodec::check_channel_layout(const uint64_t av_channel_layout) {
	const uint64_t *p = codec_->channel_layouts;
	if (!p)
		return 0;

	while (*p) {
		if (*p == av_channel_layout)
			return 1;
		p++;
	}
	return 0;
}

int FFCodec::get_channel_layout()
{   
	const uint64_t *p = codec_->channel_layouts;
    if (!p)
        return AV_CH_LAYOUT_STEREO;

	uint64_t best_ch_layout = 0;
	int best_nb_chs = 0;
    while (*p) {
        int nb_chs = av_get_channel_layout_nb_channels(*p);
        if (nb_chs > best_nb_chs) {
            best_ch_layout = *p;
            best_nb_chs = nb_chs;
        }
        p++;
    }
    return best_ch_layout;
}

int FFCodec::get_channel_layout(enum eFFChannelLayout_t ch_layout) {
	int N = FF_ARRAY_ELEMS(a_channel_layouts);
	for (int i = 0; i < N; i++) {
		const Channel_Layout_t *entry = &a_channel_layouts[i];
		if (ch_layout == entry->ch_layout)
			return entry->av_ch_layout;
	}
	return AV_CH_LAYOUT_MONO;
}

int FFCodec::get_profile() {
	AVProfile *p = const_cast<AVProfile *>(codec_->profiles);
	if (!p)
		return FF_PROFILE_AAC_HE_V2;

	int best_profile = 0;
	while (p) {
		best_profile = FFMAX(p->profile, best_profile);
		p++;
	}
	return best_profile;
}

int FFCodec::check_pix_fmt(enum AVPixelFormat pix_fmt)
{
	const enum AVPixelFormat *p = codec_->pix_fmts;
	if (!p) {
		if (pix_fmt == AV_PIX_FMT_YUV420P)
			return 1;
		return 0;
	}

    while (*p != AV_PIX_FMT_NONE) {
        if (*p == pix_fmt)
            return 1;
        p++;
    }
    return 0;
}

AVPixelFormat FFCodec::get_pix_fmt()
{
	const enum AVPixelFormat *p = codec_->pix_fmts;

    return *p;
}