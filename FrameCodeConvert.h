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


class HCODEC_API FrameCodeConvert
{
	friend class CGarbo;
public:
	~FrameCodeConvert();
	static FrameCodeConvert* GetInstance();

	bool YV12ToRGB24_Pinknoise(unsigned char* pYUV, unsigned char* pRGB24, int width, int height);
	bool YUV420ToRGB24_Pinknoise(unsigned char* pYUV, unsigned char* pRGB24, int width, int height);
	bool YUV420ToRGB24_Pinknoise(unsigned char* pY, unsigned char* pU, unsigned char* pV, unsigned char* pRGB24, int width, int height);
	bool YUV420ToRGB32_Pinknoise(unsigned char* pY, unsigned char* pU, unsigned char* pV, unsigned char* pRGB32, int width, int height);

private:
	FrameCodeConvert() {};
	FrameCodeConvert(const FrameCodeConvert& ref) {};
	FrameCodeConvert& operator =(const FrameCodeConvert& ref) { return *this; };

	static FrameCodeConvert* _instance;
	class CGarbo
	{
	public:
		~CGarbo()
		{
			if (FrameCodeConvert::_instance)
			{
				delete FrameCodeConvert::_instance;
			}
			FrameCodeConvert::_instance = nullptr;
		}
	};
};

