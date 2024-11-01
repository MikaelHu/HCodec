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

#include <memory>
#include <string>
#include <functional>
#include "api/svc/codec_api.h"


#pragma comment(lib, "common.lib") 
#pragma comment(lib, "encoder.lib") 
#pragma comment(lib, "decoder.lib") 
#pragma comment(lib, "openh264.lib") 


#define OPENH264_VIDEO_CODEC_False -1
#define OPENH264_VIDEO_CODEC_OK 0
#define OPENH264_VIDEO_CODEC_UNINITIALIZED -2
#define OPENH264_VIDEO_CODEC_ERR_PARAMETER -3
#define OPENH264_VIDEO_CODEC_ERROR -4

// #define OPENH264COVERTEST

// Pic Info
typedef struct Pic_Info{
	int		iWidth;
	int		iHeight;
	long	nStamp;
	int		iType;   //frame type. currently only support videoFormatI420
	int		iFrameRate;
	unsigned long	dwFrameNum;
}s_Pic_Info, *ps_Pic_Info;

typedef struct Enc_Param {
	int       iPicWidth;       
	int       iPicHeight;      
	int       iBitrate;   
	int		  iFrameRate;
}s_Enc_Param, *ps_Enc_Param;

class HCODEC_API OpenH264Decoder
{
public:
	OpenH264Decoder();
	~OpenH264Decoder();

	int Open();
	//pYuv: YUV420p, width, height 输出图像的宽和高, delay 延迟解码.
	int Decode(unsigned char* pBuf, int iSize, unsigned char *pYuv, int& width, int& height, int delay, int& err);
	int Close();

protected:
	ISVCDecoder*	decoder_{ nullptr };
};


class HCODEC_API OpenH264Encoder
{
public:
	typedef std::function<int(uint8_t* out_data, int out_size)> ST_Enc_CB;

public:
	OpenH264Encoder(const s_Enc_Param& enc_param);
	~OpenH264Encoder();

	int Open();
	//pBuf: YUV420p, pic_info 输入图像的参数，enc_cb 编码回调函数。
	int Encode(unsigned char* pBuf, int iSize, const s_Pic_Info& pic_info, ST_Enc_CB enc_cb, int& err);
	int Close();

protected:
	ISVCEncoder*	encoder_{ nullptr };
	s_Enc_Param		enc_param_;
	int				iSpspps_;
	__int64			iFrameIdx_;
	int				imaxFrameRate_;
};