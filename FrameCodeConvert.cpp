#include "stdafx.h"
#include "FrameCodeConvert.h"

extern "C" {
#include "./yuv2rgb003/yuv2rgb.h"
}


FrameCodeConvert* FrameCodeConvert::_instance = nullptr;

FrameCodeConvert::~FrameCodeConvert()
{
}

FrameCodeConvert* FrameCodeConvert::GetInstance()
{
	if (!_instance)
	{
		_instance = new FrameCodeConvert();
		static CGarbo _garbo;
	}
	return _instance;
}

bool FrameCodeConvert::YV12ToRGB24_Pinknoise(unsigned char* pYUV, unsigned char* pBGR24, int width, int height)
{
	if (width < 1 || height < 1 || pYUV == nullptr || pBGR24 == nullptr)
		return false;

	unsigned char *yData = pYUV;
	unsigned char *vData = &pYUV[width * height];
	unsigned char *uData = &vData[width * height >> 2];
	yuv420_2_rgb888(pBGR24, yData, uData, vData, width, height, width, width >> 1, width * 3, yuv2rgb565_table, 0);

	return true;
}

bool FrameCodeConvert::YUV420ToRGB24_Pinknoise(unsigned char* pYUV, unsigned char* pBGR24, int width, int height)
{
	if (width < 1 || height < 1 || pYUV == nullptr || pBGR24 == nullptr)
		return false;

	unsigned char *yData = pYUV;
	unsigned char *uData = &pYUV[width * height];
	unsigned char *vData = &uData[width * height >> 2];
	yuv420_2_rgb888(pBGR24, yData, uData, vData, width, height, width, width >> 1, width * 3, yuv2rgb565_table, 0);

	return true;
}

bool FrameCodeConvert::YUV420ToRGB24_Pinknoise(unsigned char* pY, unsigned char* pU, unsigned char* pV, 
	unsigned char* pBGR24, int width, int height)
{
	if (width < 1 || height < 1 || pY == nullptr || pU == nullptr || pV == nullptr || pBGR24 == nullptr)
		return false;

	yuv420_2_rgb888(pBGR24, pY, pU, pU, width, height, width, width >> 1, width * 3, yuv2rgb565_table, 0);

	return true;
}

bool FrameCodeConvert::YUV420ToRGB32_Pinknoise(unsigned char* pY, unsigned char* pU, unsigned char* pV, 
	unsigned char* pRGB32, int width, int height)
{
	if (width < 1 || height < 1 || pY == nullptr || pU == nullptr || pV == nullptr || pRGB32 == nullptr)
		return false;

	yuv420_2_rgb8888(pRGB32, pY, pU, pU, width, height, width, width >> 1, width * 4, yuv2rgb565_table, 0);

	return true;
}
