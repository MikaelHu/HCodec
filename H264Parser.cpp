#include "stdafx.h"
#include "H264Parser.h"



H264Parser::H264Parser() {
}

H264Parser::~H264Parser() {
}

int H264Parser::Parse(const uint8_t* h264_stream, int size, uint8_t* nalu, int& len) {
	int start = -1;
	int end = -1;
	for (int i = 0; i < size - 4;)
	{
		// parse NALU 00 00 00 01 or 00 00 01
		if (h264_stream[i] == 0 && h264_stream[i + 1] == 0 && h264_stream[i + 2] == 0 && h264_stream[i + 3] == 1)
		{
			if (start == -1)
			{
				start = i;
				i += 4;
				continue;
			}
			end = i;
		}
		else if (h264_stream[i] == 0 && h264_stream[i + 1] == 0 && h264_stream[i + 2] == 1)
		{
			if (start == -1)
			{
				start = i;
				i += 3;
				continue;
			}
			end = i;
		}

		if (end >= 0)
		{
			memcpy(nalu, h264_stream + start, end - start);
			len = end - start;
			return start;
		}

		++i;
	}

	return -1;
}