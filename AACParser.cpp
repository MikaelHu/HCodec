#include "stdafx.h"
#include "AACParser.h"




int sample_frequency[] = { 96000, 88200, 64000, 48000, \
44100, 32000, 24000, 22050, \
16000, 12000, 11025, 8000, \
7350, 0, 0, -1 };


AACParser::AACParser() {
}

AACParser::~AACParser() {
}

int AACParser::Parse(const uint8_t* aac_stream, int size, AAC_t& aac) {
	uint8_t* p = const_cast<uint8_t*>(aac_stream);
	uint8_t* p_begin = nullptr;
	uint8_t* p_end = nullptr;

	while (p < aac_stream + size) {
		if (!p_begin && p[0] == 0xff && (p[1] & 0xf0) == 0xf0) {
			p_begin = p;
			aac.fheader.ID = (p[1] & 0x08) >> 3;
			aac.fheader.layer = (p[1] & 0x06) >> 1;
			aac.fheader.protection_absent = (p[1] & 0x01);
			aac.fheader.profile = (p[2] & 0xc0) >> 6;
			aac.fheader.sample_freq_index = (p[2] & 0x3c) >> 2;
			if (aac.fheader.sample_freq_index != 4) {
				printf("[AACParser] parse with error frequency = %d \n", sample_frequency[aac.fheader.sample_freq_index]);
			}
			aac.fheader.private_bit = (p[2] & 0x02) >> 1;
			aac.fheader.channel_config = (((p[2] & 0x01)) << 2) | ((p[3] & 0xc0) >> 6);
			aac.fheader.original_copy = (p[3] & 0x20) >> 5;
			aac.fheader.home = (p[3] & 0x10) >> 4;

			/*aac.vheader.copyright_identification_bit = (p[3] & 0x08) >> 3;
			aac.vheader.copyright_identification_start = (p[3] & 0x04) >> 2;
			aac.vheader.aac_frame_length = ((p[3] & 0x03) << 11) | (p[4] << 3) | (p[5] & 0xe0 >> 5);
			aac.vheader.adts_buffer_fullness = (p[5] & 0x1f << 6) | (p[6] & 0xfc >> 2);
			aac.vheader.number_of_raw_data_blocks_in_frame = p[6] & 0x03;*/

			p = p + sizeof(aac.fheader) + sizeof(aac.vheader);
			aac.data = p;
		}
		if (p_begin && p[0] == 0xff && (p[1] & 0xf0) == 0xf0) {
			p_end = p - 1;
			aac.size = p - aac.data;
			break;
		}

		p++;
	}

	return p_begin - aac_stream;
}

void AACParser::GetAdtsHeader(uint8_t* adts_header, int aac_length)
{
	int profile = 2;  //AAC LC
					  //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
	int freqIdx = 4;  //44.1KHz
	int chanCfg = 2;  //CPE

	// fill in ADTS header
	adts_header[0] = (uint8_t)0xFF;
	adts_header[1] = (uint8_t)0xF9;
	adts_header[2] = (uint8_t)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
	adts_header[3] = (uint8_t)(((chanCfg & 3) << 6) + (aac_length >> 11));
	adts_header[4] = (uint8_t)((aac_length & 0x7FF) >> 3);
	adts_header[5] = (uint8_t)(((aac_length & 7) << 5) + 0x1F);
	adts_header[6] = (uint8_t)0xFC;
}

void AACParser::GetAdtsHeader1(uint8_t *adts_header, int aac_length) {
	int profile = 2;  //AAC LC
					  //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
	int freqIdx = 4;  //44.1KHz
	int chanCfg = 2;  //CPE

	//uint8_t freq_idx = 0;    //0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
	//switch (ffcodec_->avctx_->sample_rate) {
	//case 96000:
	//	freq_idx = 0;
	//	break;
	//case 88200:
	//	freq_idx = 1;
	//	break;
	//case 64000:
	//	freq_idx = 2;
	//	break;
	//case 48000:
	//	freq_idx = 3;
	//	break;
	//case 44100:
	//	freq_idx = 4;
	//	break;
	//case 32000:
	//	freq_idx = 5;
	//	break;
	//case 24000:
	//	freq_idx = 6;
	//	break;
	//case 22050:
	//	freq_idx = 7;
	//	break;
	//case 16000:
	//	freq_idx = 8;
	//	break;
	//case 12000:
	//	freq_idx = 9;
	//	break;
	//case 11025:
	//	freq_idx = 10;
	//	break;
	//case 8000:
	//	freq_idx = 11;
	//	break;
	//case 7350:
	//	freq_idx = 12;
	//	break;
	//default:
	//	freq_idx = 4;
	//	break;
	//}

	//uint8_t chanCfg = ffcodec_->avctx_->channels;
	uint32_t frame_length = aac_length + 7;
	adts_header[0] = (uint8_t)0xFF;
	adts_header[1] = (uint8_t)0xF1;
	adts_header[2] = (uint8_t)(((/*ffcodec_->avctx_->*/profile) << 6) + (freqIdx << 2) + (chanCfg >> 2));
	adts_header[3] = (uint8_t)(((chanCfg & 3) << 6) + (frame_length >> 11));
	adts_header[4] = (uint8_t)((frame_length & 0x7FF) >> 3);
	adts_header[5] = (uint8_t)(((frame_length & 7) << 5) + 0x1F);
	adts_header[6] = (uint8_t)0xFC;
}