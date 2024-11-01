#include "stdafx.h"
#include "common/inc/measure_time.h"
#include "common/inc/typedefs.h"
#include "common/inc/macros.h"
#include "Common.h"
#include "OpenH264.h"



//###############################################################################
//################ OpenH264Decoder ################
//###############################################################################
OpenH264Decoder::OpenH264Decoder() {
}

OpenH264Decoder::~OpenH264Decoder() {
}

int OpenH264Decoder::Open() {
	if (!decoder_) {
		int nret = WelsCreateDecoder(&decoder_);
		RETURN_V_IF(nret == 0, OPENH264_VIDEO_CODEC_ERROR);

		SDecodingParam sDecParam = { 0 };
		//param.eOutputColorFormat = EVideoFormatType.videoFormatI420;	// only I420
		//sDecParam.uiTargetDqLayer = UCHAR_MAX;
		//sDecParam.eEcActiveIdc = ERROR_CON_FRAME_COPY_CROSS_IDR;
		//sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
		sDecParam.uiTargetDqLayer = (uint8_t)-1;
		sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
		sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
		nret = decoder_->Initialize(&sDecParam);
		if (nret != 0) {
			WelsDestroyDecoder(decoder_);
			SAFE_DEL(decoder_);
			RETURN_V_IF(nret == 0, OPENH264_VIDEO_CODEC_ERR_PARAMETER);
		}
	}

	return 0;
}

int OpenH264Decoder::Close() {
	if (decoder_) {
		decoder_->Uninitialize();
		WelsDestroyDecoder(decoder_);
		decoder_ = nullptr;
	}

	return 0;
}

int OpenH264Decoder::Decode(unsigned char* pBuf, int iSize, unsigned char *pYuv, int& width, int& height, int delay, int& err) {
	int n_ret = OPENH264_VIDEO_CODEC_False;

	if (decoder_)
	{
		unsigned long long uiTimeStamp = 0;
		int64_t iStart = 0, iEnd = 0, iTotal = 0;
		uint8_t* pData[3] = { NULL };
		SBufferInfo sDstBufInfo;

		if (iSize < 4) { //too small size, no effective data, ignore
			return n_ret;
		}

#ifdef OPENH264COVERTEST
		int32_t iEndOfStreamFlag;
		decoder_->GetOption(DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);
		int32_t iCurIdrPicId;
		decoder_->GetOption(DECODER_OPTION_IDR_PIC_ID, &iCurIdrPicId);
		int32_t iFrameNum;
		decoder_->GetOption(DECODER_OPTION_FRAME_NUM, &iFrameNum);
		int32_t bCurAuContainLtrMarkSeFlag;
		decoder_->GetOption(DECODER_OPTION_LTR_MARKING_FLAG, &bCurAuContainLtrMarkSeFlag);
		int32_t iFrameNumOfAuMarkedLtr;
		decoder_->GetOption(DECODER_OPTION_LTR_MARKED_FRAME_NUM, &iFrameNumOfAuMarkedLtr);
		int32_t iFeedbackVclNalInAu;
		decoder_->GetOption(DECODER_OPTION_VCL_NAL, &iFeedbackVclNalInAu);
		int32_t iFeedbackTidInAu;
		decoder_->GetOption(DECODER_OPTION_TEMPORAL_ID, &iFeedbackTidInAu);
#endif // OPENH264COVERTEST

		iStart = WelsTime();
		pData[0] = NULL;
		pData[1] = NULL;
		pData[2] = NULL;
		uiTimeStamp++;
		memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
		sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;

		if(!delay)
			decoder_->DecodeFrameNoDelay(pBuf, iSize, pData, &sDstBufInfo);
		else
			decoder_->DecodeFrame2(pBuf, iSize, pData, &sDstBufInfo);
	

		if (sDstBufInfo.iBufferStatus == 1) {
			width = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
			height = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
			int y_src_width = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];  // y_dst_width + padding
			int uv_src_width = sDstBufInfo.UsrData.sSystemBuffer.iStride[1]; // uv_dst_width + padding
			int y_dst_width = width;
			int uv_dst_width = width / 2;
			int y_dst_height = height;
			int uv_dst_height = height / 2;
			// y1 ... y1280, padding, y1281 ... y2560, padding, y2561 ... y921600, padding
			unsigned char *y_plane = pData[0];
			// u1 ... u640, padding, u641 ... u1280, padding, u1281 ... u230400, padding
			unsigned char *u_plane = pData[1];
			// v1 ... v640, padding, v641 ... v1280, padding, v1281 ... v230400, padding
			unsigned char *v_plane = pData[2];

			int pos = 0;
			for (int row = 0; row < y_dst_height; row++)
			{
				memcpy(pYuv + pos, y_plane + y_src_width * row, y_dst_width);
				pos += y_dst_width;
			}
			for (int row = 0; row < uv_dst_height; row++)
			{
				memcpy(pYuv + pos, u_plane + uv_src_width * row, uv_dst_width);
				pos += uv_dst_width;
			}
			for (int row = 0; row < uv_dst_height; row++)
			{
				memcpy(pYuv + pos, v_plane + uv_src_width * row, uv_dst_width);
				pos += uv_dst_width;
			}
		}
		iEnd = WelsTime();
		iTotal += iEnd - iStart;

		if (!delay) {
			iStart = WelsTime();
			pData[0] = NULL;
			pData[1] = NULL;
			pData[2] = NULL;
			memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
			sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
			decoder_->DecodeFrame2(NULL, 0, pData, &sDstBufInfo);
			if (sDstBufInfo.iBufferStatus == 1) {
				width = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
				height = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
				int y_src_width = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];  // y_dst_width + padding
				int uv_src_width = sDstBufInfo.UsrData.sSystemBuffer.iStride[1]; // uv_dst_width + padding
				int y_dst_width = width;
				int uv_dst_width = width / 2;
				int y_dst_height = height;
				int uv_dst_height = height / 2;
				// y1 ... y1280, padding, y1281 ... y2560, padding, y2561 ... y921600, padding
				unsigned char *y_plane = pData[0];
				// u1 ... u640, padding, u641 ... u1280, padding, u1281 ... u230400, padding
				unsigned char *u_plane = pData[1];
				// v1 ... v640, padding, v641 ... v1280, padding, v1281 ... v230400, padding
				unsigned char *v_plane = pData[2];

				int pos = 0;
				for (int row = 0; row < y_dst_height; row++)
				{
					memcpy(pYuv + pos, y_plane + y_src_width * row, y_dst_width);
					pos += y_dst_width;
				}
				for (int row = 0; row < uv_dst_height; row++)
				{
					memcpy(pYuv + pos, u_plane + uv_src_width * row, uv_dst_width);
					pos += uv_dst_width;
				}
				for (int row = 0; row < uv_dst_height; row++)
				{
					memcpy(pYuv + pos, v_plane + uv_src_width * row, uv_dst_width);
					pos += uv_dst_width;
				}
			}
			iEnd = WelsTime();
			iTotal += iEnd - iStart;
		}

		if (sDstBufInfo.iBufferStatus == 1) {
			return OPENH264_VIDEO_CODEC_OK;
		}
		else {
			err = OPENH264_VIDEO_CODEC_ERROR;
			return n_ret;
		}
	}

	err = OPENH264_VIDEO_CODEC_ERR_PARAMETER;
	return n_ret;
}


//###############################################################################
//################ OpenH264Encoder ################
//###############################################################################

OpenH264Encoder::OpenH264Encoder(const s_Enc_Param& enc_param): enc_param_(enc_param),
iSpspps_(0), iFrameIdx_(0), imaxFrameRate_(0) {
}

OpenH264Encoder::~OpenH264Encoder() {
}

int OpenH264Encoder::Open() {
	if (!encoder_) {
		printf("[OpenH264Encoder] WelsCreateSVCEncoder!\n");
		int nret = WelsCreateSVCEncoder(&encoder_);
		RETURN_V_IF(nret == 0, OPENH264_VIDEO_CODEC_ERROR);

		//initilize with basic parameter
		SEncParamExt param;
		memset(&param, 0, sizeof(SEncParamBase));

		encoder_->GetDefaultParams(&param);
		param.iUsageType = CAMERA_VIDEO_REAL_TIME;
		param.fMaxFrameRate = enc_param_.iFrameRate;
		param.iPicWidth = enc_param_.iPicWidth;
		param.iPicHeight = enc_param_.iPicHeight;
		param.iTargetBitrate = enc_param_.iBitrate;
		param.iMaxBitrate = UNSPECIFIED_BIT_RATE;
		param.iRCMode = RC_QUALITY_MODE;
		param.iTemporalLayerNum = 3;
		param.iSpatialLayerNum = 1;
		param.bEnableDenoise = 0;
		param.bEnableBackgroundDetection = 1;
		param.bEnableAdaptiveQuant = 1;
		param.bEnableFrameSkip = 1;
		param.bEnableLongTermReference = 0;
		param.iLtrMarkPeriod = 30;
		param.uiIntraPeriod = (unsigned int)enc_param_.iFrameRate * 2;
		param.eSpsPpsIdStrategy = INCREASING_ID;
		param.bPrefixNalAddingCtrl = 0;
		param.iComplexityMode = MEDIUM_COMPLEXITY;
		param.bSimulcastAVC = false;
		param.sSpatialLayers[0].uiProfileIdc = PRO_BASELINE;
		param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
		param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
		param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
		param.sSpatialLayers[0].iSpatialBitrate = param.iTargetBitrate;
		param.sSpatialLayers[0].iMaxSpatialBitrate = param.iMaxBitrate;
		param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;

		float fMaxFr = param.sSpatialLayers[param.iSpatialLayerNum - 1].fFrameRate;
		for (int32_t i = param.iSpatialLayerNum - 2; i >= 0; --i) {
			if (param.sSpatialLayers[i].fFrameRate > fMaxFr + EPSN)
				fMaxFr = param.sSpatialLayers[i].fFrameRate;
		}
		param.fMaxFrameRate = fMaxFr;

		nret = encoder_->InitializeExt(&param);
		if (nret != 0) {
			printf("[OpenH264Encoder] WelsDestroySVCEncoder err!\n");
			WelsDestroySVCEncoder(encoder_);
			encoder_ = nullptr;

			RETURN_V_IF(nret == 0, OPENH264_VIDEO_CODEC_ERROR);
		}

		int videoFormat = videoFormatI420;
		encoder_->SetOption(ENCODER_OPTION_DATAFORMAT, &videoFormat);

		imaxFrameRate_ = param.fMaxFrameRate;
	}
	return 0;
}


int OpenH264Encoder::Close() {
	if (encoder_) {
		encoder_->Uninitialize();
		WelsDestroySVCEncoder(encoder_);
		encoder_ = nullptr;
	}
	return 0;
}

int OpenH264Encoder::Encode(unsigned char* pBuf, int iSize, const s_Pic_Info& pic_info, ST_Enc_CB enc_cb, int& err) {
	SSourcePicture pic;
	memset(&pic, 0, sizeof(SSourcePicture));
	int width = pic_info.iWidth;
	int height = pic_info.iHeight;
	pic.iPicWidth = width;
	pic.iPicHeight = height;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = pic.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
	pic.pData[0] = pBuf;
	pic.pData[1] = pic.pData[0] + width * height;
	pic.pData[2] = pic.pData[1] + (width * height >> 2);
	pic.uiTimeStamp = WELS_ROUND(iFrameIdx_ * (1000 / imaxFrameRate_));

	SFrameBSInfo info;
	memset(&info, 0, sizeof(SFrameBSInfo));

	int iEncFrames = encoder_->EncodeFrame(&pic, &info);

	++iFrameIdx_;

	int  nret = -1;
	if (info.eFrameType != videoFrameTypeSkip) {
		switch (info.eFrameType) {
		case videoFrameTypeIDR:
			printf("[OpenH264Encoder] IDR frame!\n");
			break;
		case videoFrameTypeI:
			printf("[OpenH264Encoder] I frame!\n");
			break;
		case videoFrameTypeP:
			printf("[OpenH264Encoder] P frame!\n");
			break;
		case videoFrameTypeIPMixed:
			printf("[OpenH264Encoder] I and P mixed frame!\n");
			break;
		case videoFrameTypeInvalid:
			printf("[OpenH264Encoder] Invalid frame!\n");
			break;
		default:
			break;
		}

		//for what usage?
		int first_layer = 0;
		if (iSpspps_) {
			first_layer = info.iLayerNum - 1;
		}
		else {
			iSpspps_ = 1;
		}

		if (iEncFrames == cmResultSuccess) {
			printf("[OpenH264Encoder] info.iLayerNum:%d\n", info.iLayerNum);
			for (int i = first_layer; i<info.iLayerNum; i++) {
				printf("[OpenH264Encoder] Layer:%d NalCount:%d\n", i, info.sLayerInfo[i].iNalCount);
				int pos = 0;
				for (int j = 0; j < info.sLayerInfo[i].iNalCount; j++) {
					printf("[OpenH264Encoder] LayNum:%d  NalCount:%d iFrameSizeInBytes:%d NalLengthInByte:%d\n", i, j, info.iFrameSizeInBytes, info.sLayerInfo[i].pNalLengthInByte[j]);

					nret = enc_cb(info.sLayerInfo[i].pBsBuf + pos, info.sLayerInfo[i].pNalLengthInByte[j]);
					if (!nret) {
						err = nret;
						printf("[OpenH264Encoder] FrameEncode SendH264Nalu failure: %d", err);
					}
					pos += info.sLayerInfo[i].pNalLengthInByte[j];
				}
			}
		}

		iSpspps_ = 0;  //hnc??? added!!!
	}
	else {
		printf("[OpenH264Encoder] Skip frame!\n");
	}

	return !nret ? OPENH264_VIDEO_CODEC_OK : OPENH264_VIDEO_CODEC_False;
}