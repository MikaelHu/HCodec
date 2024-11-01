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


#include <stdio.h>
#include <stdint.h>



typedef struct {
	unsigned syncword : 12; // FFF
	unsigned ID : 1; //MPEG��ʶ����0��ʶMPEG-4��1��ʶMPEG-2
	unsigned layer : 2; // always: '00'
	unsigned protection_absent : 1; //��ʾ�Ƿ�����У�顣Warning, set to 1 if there is no CRC and 0 if there is CRC
	unsigned profile : 2; //��ʾʹ���ĸ������AAC����01 Low Complexity(LC)--- AAC LC����ЩоƬֻ֧��AAC LC ��
	unsigned sample_freq_index : 4;//��ʾʹ�õĲ������±꣬ͨ������±��� Sampling Frequencies[ ]�����в��ҵ�֪�����ʵ�ֵ��
	unsigned private_bit : 1;
	unsigned channel_config : 3; /*
								0: Defined in AOT Specifc Config
								1: 1 channel: front-center
								2: 2 channels: front-left, front-right
								3: 3 channels: front-center, front-left, front-right
								4: 4 channels: front-center, front-left, front-right, back-center
								5: 5 channels: front-center, front-left, front-right, back-left, back-right
								6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
								7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
								8-15: Reserved
								*/
	unsigned    original_copy : 1;
	unsigned    home : 1;
} ADTS_Fixed_Hearder_t, *pADTS_Fixed_Hearder_t; // 28bits


typedef struct {
	unsigned copyright_identification_bit : 1;
	unsigned copyright_identification_start : 1;
	/*
	aac_frame_length = (protection_absent == 1 ? 7 : 9) + size(AACFrame)
	protection_absent=0ʱ, header length=9bytes
	protection_absent=1ʱ, header length=7bytes
	*/
	unsigned aac_frame_length : 13;
	unsigned adts_buffer_fullness : 11; //0x7FF ˵�������ʿɱ��������

	/*��ʾADTS֡���� number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡��
	����˵number_of_raw_data_blocks_in_frame == 0 ��ʾ˵ADTS֡����һ��AAC���ݿ顣*/
	unsigned number_of_raw_data_blocks_in_frame : 2;
} ADTS_Var_Header_t, *pADTS_Var_Header_t;

typedef struct {
	ADTS_Fixed_Hearder_t	fheader;
	ADTS_Var_Header_t		vheader;
	uint8_t*				data;	// payload
	size_t					size;	// data size
}AAC_t, *pAAC_t;


class HCODEC_API AACParser {
public:
	AACParser();
	~AACParser();

	//return pos: >= 0 aac start, -1 err.
	int Parse(const uint8_t* aac_stream, int size, AAC_t& aac);

	void GetAdtsHeader(uint8_t *adts_header, int aac_length);
	void GetAdtsHeader1(uint8_t *adts_header, int aac_length);
};