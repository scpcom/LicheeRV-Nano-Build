/*
 * Based on:
 * https://github.com/riverlight/H264toFLVConverter
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <cvi_buffer.h>
#include <maix_avc2flv.h>

#define AVC2FLV_SUCCESS	0
#define AVC2FLV_FAILED	1

static unsigned char _FlvHeader[9];
static unsigned char *_pSPS = NULL, *_pPPS = NULL;
static int _nSPSSize, _nPPSSize;
static int _bWriteAVCSeqHeader;
static int _nStreamID;
static int _nVideoTimeStamp;
static int _bHaveAudio, _bHaveVideo;

static unsigned char *pItData = NULL;
static unsigned int nItDataSize = 0;
static unsigned int nItOffset = 0;

static unsigned char *_pOutBuf = NULL;
static unsigned int nOutBufMax = 0;
static unsigned int nOutBufSize = 0;

static void _bufOutWrite(char *data, unsigned int len)
{
	if (nOutBufSize + len > nOutBufMax)
		return;

	memcpy(_pOutBuf + nOutBufSize, data, len);
	nOutBufSize += len;
}

static void WriteChar(char u)
{
	_bufOutWrite((char *)&u, 1);
}

static void WriteU4(int i)
{
	unsigned char _u[4];
	_u[0] = i >> 24; _u[1] = (i >> 16) & 0xff; _u[2] = (i >> 8) & 0xff; _u[3] = i & 0xff;
	_bufOutWrite((char *)_u, 4);
}

static void WriteU3(int i)
{
	unsigned char _u[3];
	_u[0] = i >> 16; _u[1] = (i >> 8) & 0xff; _u[2] = i & 0xff;
	_bufOutWrite((char *)_u, 3);
}

static void WriteU2(int i)
{
	unsigned char _u[2];
	_u[0] = i >> 8; _u[1] = i & 0xff;
	_bufOutWrite((char *)_u, 2);
}


static void MakeFlvHeader(unsigned char *pFlvHeader)
{
	pFlvHeader[0] = 'F';
	pFlvHeader[1] = 'L';
	pFlvHeader[2] = 'V';
	pFlvHeader[3] = 1;
	pFlvHeader[4] = 0x0;
	if (_bHaveVideo!=0)
		pFlvHeader[4] |= 0x01;
	if (_bHaveAudio != 0)
		pFlvHeader[4] |= 0x04;

	unsigned int size = 9;
	unsigned char size_u[4];
	size_u[0] = size >> 24;
	size_u[1] = (size >> 16) & 0xff;
	size_u[2] = (size >> 8) & 0xff;
	size_u[3] = size & 0xff;
	memcpy(pFlvHeader + 5, size_u, sizeof(unsigned int));

	_bufOutWrite((char *)_FlvHeader, 9);

	int _nPrevTagSize = 0;
	WriteU4(_nPrevTagSize);
}

static void WriteH264Header(unsigned int nTimeStamp)
{
	char cTagType = 0x09;
	_bufOutWrite(&cTagType, 1);
	int nDataSize = 1 + 1 + 3 + 6 + 2 + (_nSPSSize - 4) + 1 + 2 + (_nPPSSize - 4);

	WriteU3(nDataSize);

	WriteU3(nTimeStamp);

	unsigned char cTTex = nTimeStamp >> 24;
	_bufOutWrite((char *)&cTTex, 1);

	WriteU3(_nStreamID);

	unsigned char cVideoParam = 0x17;
	_bufOutWrite((char *)&cVideoParam, 1);
	unsigned char cAVCPacketType = 0; /* seq header */
	_bufOutWrite((char *)&cAVCPacketType, 1);

	WriteU3(0);

	WriteChar((unsigned char)(0x01));
	WriteChar(_pSPS[5]);
	WriteChar(_pSPS[6]);
	WriteChar(_pSPS[7]);
	WriteChar((unsigned char)(0xff));
	WriteChar((unsigned char)(0xE1));

	WriteU2(_nSPSSize - 4);
	_bufOutWrite((char *)(_pSPS + 4), _nSPSSize - 4);
	WriteChar((unsigned char)(0x01));

	WriteU2(_nPPSSize - 4);
	_bufOutWrite((char *)(_pPPS + 4), _nPPSSize - 4);

	int _nPrevTagSize = 11 + nDataSize;
	WriteU4(_nPrevTagSize);
}

static void WriteH264Frame(char *pNalu, int nNaluSize, unsigned int nTimeStamp)
{
	int nNaluType = pNalu[4] & 0x1f;
	if (nNaluType == 7 || nNaluType == 8)
		return;

	WriteChar(0x09);
	int nDataSize;
	nDataSize = 1 + 1 + 3 + 4 + (nNaluSize - 4);
	WriteU3(nDataSize);
	WriteU3(nTimeStamp);
	WriteChar((unsigned char)(nTimeStamp >> 24));

	WriteU3(_nStreamID);

	if (nNaluType == 5)
		WriteChar(0x17);
	else
		WriteChar(0x27);
	WriteChar((unsigned char)(0x01));
	WriteU3(0);

	WriteU4(nNaluSize - 4);

	_bufOutWrite((char *)(pNalu + 4), nNaluSize - 4);

	int _nPrevTagSize = 11 + nDataSize;
	WriteU4(_nPrevTagSize);
}

static void WriteH264EndofSeq(void)
{
	WriteChar(0x09);
	int nDataSize;
	nDataSize = 1 + 1 + 3;
	WriteU3(nDataSize);
	WriteU3(_nVideoTimeStamp);
	WriteChar((unsigned char)(_nVideoTimeStamp >> 24));

	WriteU3(_nStreamID);

	WriteChar(0x27);
	WriteChar(0x02);

	WriteU3(0);
}

static int GetOneNalu(unsigned char *pBufIn, int nInSize, unsigned char **ppNalu, int *nNaluSize)
{
	unsigned char *p = pBufIn;
	int nStartPos = 0, nEndPos = 0;

	if (nInSize < 4)
		return 0;

	while (1)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
		{
			nStartPos = p - pBufIn;
			break;
		}
		p++;
		if (p - pBufIn >= nInSize - 4)
			return 0;
	}
	p++;
	while (1)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
		{
			nEndPos = p - pBufIn;
			break;
		}
		p++;
		if (p - pBufIn >= nInSize - 4)
		{
			nEndPos = nInSize;
			break;
		}
	}
	*nNaluSize = nEndPos - nStartPos;
	*ppNalu = pBufIn + nStartPos;

	return 1;
}

static int ConvertH264(char *pNalu, int nNaluSize, uint32_t nTimeStamp)
{
	_nVideoTimeStamp = nTimeStamp;

	if (pNalu == NULL || nNaluSize <= 4)
		return 0;

	int nNaluType = pNalu[4] & 0x1f;
	if (_pSPS == NULL && nNaluType == 0x07)
	{
		_pSPS = (unsigned char*)malloc(nNaluSize);
		_nSPSSize = nNaluSize;
		memcpy(_pSPS, pNalu, nNaluSize);
	}
	if (_pPPS == NULL && nNaluType == 0x08)
	{
		_pPPS = (unsigned char*)malloc(nNaluSize);
		_nPPSSize = nNaluSize;
		memcpy(_pPPS, pNalu, nNaluSize);
	}
	if (_pSPS != NULL && _pPPS != NULL && _bWriteAVCSeqHeader == 0)
	{
		WriteH264Header(nTimeStamp);
		_bWriteAVCSeqHeader = 1;
	}

	if (_bWriteAVCSeqHeader != 0) {
		WriteH264Frame(pNalu, nNaluSize, nTimeStamp);
	}

	return 1;
}


int maix_avc2flv_init(int max_buff_size)
{
	_pOutBuf = malloc(max_buff_size);
	nOutBufMax = max_buff_size;
	nOutBufSize = 0;

	_pSPS = NULL;
	_pPPS = NULL;
	_nSPSSize = 0;
	_nPPSSize = 0;
	_bWriteAVCSeqHeader = 0;
	_nStreamID = 0;

	return AVC2FLV_SUCCESS;
}

int maix_avc2flv_deinit()
{
	if (_pSPS) {
		free(_pSPS);
		_pSPS = NULL;
	}
	if (_pPPS) {
		free(_pPPS);
		_pPPS = NULL;
	}

	_nSPSSize = 0;
	_nPPSSize = 0;
	_bWriteAVCSeqHeader = 0;

	if (_pOutBuf) {
		free(_pOutBuf);
		_pOutBuf = NULL;
	}

	nOutBufMax = 0;
	nOutBufSize = 0;

	return AVC2FLV_SUCCESS;
}

int maix_avc2flv_prepare(uint8_t *data, int data_size)
{
	pItData = data;
	nItDataSize = data_size;
	nItOffset = 0;

	return AVC2FLV_SUCCESS;
}

int maix_avc2flv_iterate(void **nalu, int *size)
{
	unsigned char *pOneNalu = NULL;
	int nOneNaluSize = 0;

	nOutBufSize = 0;

	if (pItData == NULL || nItOffset + 4 >= nItDataSize) {
		return AVC2FLV_FAILED;
	}

	if (GetOneNalu(pItData + nItOffset, nItDataSize - nItOffset, &pOneNalu, &nOneNaluSize) == 0) {
		return AVC2FLV_FAILED;
	}
	if (pOneNalu == NULL || nOneNaluSize < 1) {
		return AVC2FLV_FAILED;
	}

	if (ConvertH264((char *)pOneNalu, nOneNaluSize, _nVideoTimeStamp) == 0) {
		return AVC2FLV_FAILED;
	}

	nItOffset += nOneNaluSize;

	if (nOutBufSize > 4 && _pOutBuf[4] != 0x67 && _pOutBuf[4] != 0x68)
	{
		_nVideoTimeStamp += 33;
	}
	if (nalu) {
		*nalu = _pOutBuf;
	}
	if (size) {
		*size = nOutBufSize;
	}

	return AVC2FLV_SUCCESS;
}

int maix_avc2flv(void *nalu, int nalu_size, uint32_t pts, uint32_t dts, uint8_t **flv, int *flv_size)
{
	int nOffset = 0;

	UNUSED(dts);
	nOutBufSize = 0;

	if (nalu == NULL || nalu_size == 0)
	{
		return AVC2FLV_FAILED;
	}

	while (1)
	{
		unsigned char *pOneNalu = NULL;
		int nOneNaluSize = 0;
		if (GetOneNalu(nalu + nOffset, nalu_size - nOffset, &pOneNalu, &nOneNaluSize) == 0)
			break;

		ConvertH264((char *)pOneNalu, nOneNaluSize, pts);

		nOffset += nOneNaluSize;
		if (nOffset >= nalu_size - 4)
			break;
	}

	if (nOutBufSize < 1) {
		return AVC2FLV_FAILED;
	}
	if (flv) {
		*flv = _pOutBuf;
	}
	if (flv_size) {
		*flv_size = nOutBufSize;
	}

	return AVC2FLV_SUCCESS;
}

int maix_flv_get_tail(uint8_t **data, int *size)
{
	nOutBufSize = 0;

	if (_bHaveVideo != 0)
		WriteH264EndofSeq();

	if (nOutBufSize < 1) {
		return AVC2FLV_FAILED;
	}
	if (data) {
		*data = _pOutBuf;
	}
	if (size) {
		*size = nOutBufSize;
	}

	return AVC2FLV_SUCCESS;
}

int maix_flv_get_header(int audio, int video, uint8_t **data, int *size)
{
	nOutBufSize = 0;

	_bHaveAudio = audio;
	_bHaveVideo = video;

	MakeFlvHeader(_FlvHeader);

	if (nOutBufSize < 1) {
		return AVC2FLV_FAILED;
	}
	if (data) {
		*data = _pOutBuf;
	}
	if (size) {
		*size = nOutBufSize;
	}

	return AVC2FLV_SUCCESS;
}
