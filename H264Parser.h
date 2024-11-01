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


#include <stdint.h>



class HCODEC_API H264Parser {
public:
	H264Parser();
	~H264Parser();

	//return pos: >= 0 nalu start, -1 err.
	int Parse(const uint8_t* h264_stream, int size, uint8_t* nalu, int& len);

};