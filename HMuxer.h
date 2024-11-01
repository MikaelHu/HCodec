#pragma once

#ifdef DLL_EXPORT
#ifdef BUILD_DLL
#define HCODEC_API __declspec(dllexport)
#else
#define HCODEC_API __declspec(dllimport)
#endif
#else
#define HCODEC_API
#endif

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include "FFMpeg/ffencoder.h"
#include "Common.h"


using std::string;
using std::queue;
using std::mutex;
using std::thread;


#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */


template<typename T, typename SizeType = uint16_t>
class MemoryPool;

// the numbers of wanted pieces of memory.
#define MEM_NUM 16



class HCODEC_API HMuxerBase {
protected:
	typedef struct RawData {
		uint8_t *data{ nullptr };
		int		size{ 0 };
	}RawData_t, *pRawData_t;

	typedef struct OutputStream {
		AVStream *st{ nullptr };
		AVCodecContext *enc{ nullptr };

		/* pts of the next frame that will be generated */
		int64_t next_pts{ 0 };
		int samples_count{ 0 };

		AVFrame *frame{ nullptr };
		AVFrame *tmp_frame{ nullptr };

		struct SwsContext *sws_ctx{ nullptr };
		struct SwrContext *swr_ctx{ nullptr };
	}OutputStream_t, *pOutputStream_t;

public:
	// vfmt: input raw video stream format; afmt: input raw audio stream format.
	HMuxerBase(const FFVideoFormat &vfmt, const FFAudioFormat &afmt);
	virtual ~HMuxerBase();

	// out_file: out put file
	virtual int Open(const string& out_file) { return 0; };
	// data: video or audio stream; size: data size; type: 0 video, 1 audio
	// return 0 success, -1 err
	virtual int Mux(const uint8_t *data, int size, int type);
	virtual void Close() { return; };

protected:
	int Init();
	static void AvLog_cb(void *avcl, int level, const char *fmt, va_list vl);
	// return 0 video, 1 audio., -1 nothing
	virtual int GetMuxDataType() { return -1; };

protected:
	FFVideoFormat		ffVFmt_;	// input video format
	FFAudioFormat		ffAFmt_;	// input audio format
	AVFormatContext*	outfmtCtx_{ nullptr };
	OutputStream_t		vOStream_;
	OutputStream_t		aOStream_;

	MemoryPool<uint8_t, uint32_t>*	vMemPool_{ nullptr };
	MemoryPool<uint8_t>*	aMemPool_{ nullptr };
	queue<RawData_t>		vQueue_;
	queue<RawData_t>		aQueue_;
	mutex					vMtx_;
	mutex					aMtx_;

	thread*		workThread_{ nullptr };
	int			bExit_{ 0 };

	static FILE*	plogf_;
	std::string		strlogfile_;
};

// Raw video stream(yuv, bgr .etc) and raw audio stream(pcm) muxer.
class HCODEC_API HMuxer : public HMuxerBase {
public:
	// vfmt: input raw video stream format; afmt: input raw audio stream format.
	HMuxer(const FFVideoFormat &vfmt, const FFAudioFormat &afmt);
	virtual ~HMuxer();

	// out_file: out put file
	virtual int Open(const string& out_file);
	// data: raw video or audio stream; size: data size; type: 0 video, 1 audio
	virtual void Close();

protected:
	int Init();
	int DoMux();
	// return 0 video, 1 audio., -1 nothing
	virtual int GetMuxDataType();

protected:
	FFVideoEncoder*		pFFVEncoder_{ nullptr };
	FFAudioEncoder*		pFFAEncoder_{ nullptr };
}; 


// Encoded video stream(h264, h265 .etc) and encoded audio stream(aac, .etc) muxer.
class HCODEC_API HReMuxer : public HMuxerBase {
public:
	// vfmt: input video stream format; afmt: input audio stream format.
	HReMuxer(const FFVideoFormat &vfmt, const FFAudioFormat &afmt);
	virtual ~HReMuxer();

	// out_file: out put file
	virtual int Open(const string& out_file);
	// data: video or audio stream; size: data size; type: 0 video, 1 audio
	virtual void Close();

protected:
	int Init();
	// type: 0 video, 1 audio.
	// return 0 success, -1 err
	int AddStream(int type);
	void LogPacket(AVPacket *packet);

	// return 0 video, 1 audio., -1 nothing
	virtual int GetMuxDataType();

	int DoMux();
	// type: 0 video, 1 audio.
	// return 0 success, -1 err
	int WriteFrame(AVPacket *packet, int type);

protected:
	AVPacket	*avpkt_{ nullptr };
};