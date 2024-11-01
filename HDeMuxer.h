#pragma once

#include <string>
#include <functional>
#include "Common.h"
#include "FFMpeg/ffdecoder.h"


#ifdef DLL_EXPORT
#ifdef BUILD_DLL
#define HCODEC_API __declspec(dllexport)
#else
#define HCODEC_API __declspec(dllimport)
#endif
#else
#define HCODEC_API
#endif

using std::string;


class HCODEC_API HDeMuxer {
public:
	typedef std::function<int(uint8_t *data, int size, int type)> ST_Dmx_CB;

public:
	HDeMuxer();
	~HDeMuxer();

	int Open(const string& in_file);
	// dmx_cb:: data: video or audio stream; size: data size; type: 0 video, 1 audio
	// return 0 success, -1 err
	int DeMux(ST_Dmx_CB dmx_cb);
	void Close();

protected:
	int Init();
	void LogPacket();
	static void AvLog_cb(void *avcl, int level, const char *fmt, va_list vl);
	int ProcessH264Pkt(AVPacket* pkt);

protected:
	AVFormatContext*	infmtCtx_{ nullptr };
	AVCodec				*codec_{ nullptr };
	AVCodecContext		*avctx_{ nullptr };
	int					aStreamIndex_{ -1 };
	int					vStreamIndex_{ -1 };
	AVPacket			*avpkt_;

	static FILE*	plogf_;
	std::string		strlogfile_;
};