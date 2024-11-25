#ifdef __cplusplus
#include <cstdarg>
#else
#include <stdarg.h>
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include "math.h"
#include <inttypes.h>

#include <fcntl.h>		/* low-level i/o */
#ifdef __cplusplus
extern "C" {
#endif
#include <linux/types.h>
#include <linux/cvi_common.h>
#include <linux/cvi_comm_sys.h>
#include <linux/sys_uapi.h>
extern int ionFree(struct sys_ion_data *para);
#ifdef __cplusplus
}
#endif
#include "cvi_buffer.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"
#include "cvi_sns_ctrl.h"
#include "cvi_ive.h"
#include "sample_comm.h"
#ifdef __cplusplus
#include "sophgo_middleware.hpp"
#else
#include "maix_mmf.h"
#endif



#ifndef MMF_FUNC_GET_PARAM_METHOD
#define MMF_FUNC_GET_PARAM_METHOD(x)    ((x >> 24) & 0xffffff)
#endif
#ifndef MMF_FUNC_GET_PARAM_NUM
#define MMF_FUNC_GET_PARAM_NUM(x)       ((x & 0xff))
#endif

#define MMF_DEC_MAX_CHN			2
#define MMF_ENC_MAX_CHN			2
#define MMF_VI_MAX_CHN 			2		// manually limit the max channel number of vi
#define MMF_VO_VIDEO_MAX_CHN 	1		// manually limit the max channel number of vo
#define MMF_VO_OSD_MAX_CHN 		1
#define MMF_VO_VIDEO_LAYER    	0
#define MMF_VO_OSD_LAYER    	1
#define MMF_VO_USE_NV21_ONLY	0
#define MMF_RGN_MAX_NUM			16

#define MMF_VB_VI_ID			0
#define MMF_VB_VO_ID			1
#define MMF_VB_USER_ID			2
#define MMF_VB_ENC_H26X_ID		3
#define MMF_VB_ENC_JPEG_ID		4
#define MMF_VB_DEC_H26X_ID		5
#define MMF_VB_DEC_JPEG_ID		6

#if VPSS_MAX_PHY_CHN_NUM < MMF_VI_MAX_CHN
#error "VPSS_MAX_PHY_CHN_NUM < MMF_VI_MAX_CHN"
#endif
typedef struct {
	int mmf_used_cnt;

	PIXEL_FORMAT_E vi_format;
	PIXEL_FORMAT_E vi_vpss_format;
	int vi_pop_timeout;
	int vi_vpss;
	bool vi_is_inited;
	bool vi_chn_is_inited[MMF_VI_MAX_CHN];
	SIZE_S vi_max_size;
	SIZE_S vi_size;
	VIDEO_FRAME_INFO_S vi_frame[MMF_VI_MAX_CHN];

	int vo_rotate;	// 90, 180, 270
	int vo_vpss;
	bool vo_video_chn_is_inited[MMF_VO_VIDEO_MAX_CHN];
	bool vo_osd_chn_is_inited[MMF_VO_OSD_MAX_CHN];
	SIZE_S vo_vpss_in_size[MMF_VO_VIDEO_MAX_CHN];
	SIZE_S vo_vpss_out_size[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_out_fps[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_out_depth[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_in_format[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_fit[MMF_VO_VIDEO_MAX_CHN];
	VIDEO_FRAME_INFO_S *vo_video_pre_frame[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_width[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_height[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_format[MMF_VO_VIDEO_MAX_CHN];
	SAMPLE_VO_CONFIG_S vo_video_cfg[MMF_VO_VIDEO_MAX_CHN];

	int ive_is_init;
	IVE_HANDLE ive_handle;
	IVE_IMAGE_S ive_rgb2yuv_rgb_img;
	IVE_IMAGE_S ive_rgb2yuv_yuv_img;
	int ive_rgb2yuv_w;
	int ive_rgb2yuv_h;

	bool rgn_is_init[MMF_RGN_MAX_NUM];
	bool rgn_is_bind[MMF_RGN_MAX_NUM];
	RGN_TYPE_E rgn_type[MMF_RGN_MAX_NUM];
	int rgn_id[MMF_RGN_MAX_NUM];
	MOD_ID_E rgn_mod_id[MMF_RGN_MAX_NUM];
	CVI_S32 rgn_dev_id[MMF_RGN_MAX_NUM];
	CVI_S32 rgn_chn_id[MMF_RGN_MAX_NUM];
	uint8_t* rgn_canvas_data[MMF_RGN_MAX_NUM];
	int rgn_canvas_w[MMF_RGN_MAX_NUM];
	int rgn_canvas_h[MMF_RGN_MAX_NUM];
	int rgn_canvas_format[MMF_RGN_MAX_NUM];

	PAYLOAD_TYPE_E enc_chn_type[MMF_ENC_MAX_CHN];
	int enc_chn_vb_id[MMF_ENC_MAX_CHN];
	int enc_chn_vpss[MMF_ENC_MAX_CHN];
	int enc_chn_is_init[MMF_ENC_MAX_CHN];
	int enc_chn_running[MMF_ENC_MAX_CHN];
	VIDEO_FRAME_INFO_S *enc_chn_frame[MMF_ENC_MAX_CHN];
	VENC_STREAM_S enc_chn_stream[MMF_ENC_MAX_CHN];
	mmf_venc_cfg_t enc_chn_cfg[MMF_ENC_MAX_CHN];

	int dec_pop_timeout;
	PAYLOAD_TYPE_E dec_chn_type[MMF_DEC_MAX_CHN];
	int dec_chn_vb_id[MMF_DEC_MAX_CHN];
	int dec_chn_vpss[MMF_DEC_MAX_CHN];
	int dec_chn_is_init[MMF_DEC_MAX_CHN];
	int dec_chn_running[MMF_DEC_MAX_CHN];
	VIDEO_FRAME_INFO_S dec_chn_frame[MMF_DEC_MAX_CHN];
	VIDEO_FRAME_INFO_S *dec_pst_frame[MMF_DEC_MAX_CHN];
	VDEC_STREAM_S dec_chn_stream[MMF_DEC_MAX_CHN];
	SIZE_S dec_chn_size_in[MMF_DEC_MAX_CHN];
	mmf_vdec_cfg_t dec_chn_cfg[MMF_DEC_MAX_CHN];

	VB_CONFIG_S vb_conf;
	int vb_of_vi_is_config : 1;
	int vb_of_vo_is_config : 1;
	int vb_of_private_is_config : 1;
	int vb_size_of_vi;
	int vb_count_of_vi;
	int vb_size_of_vo;
	int vb_count_of_vo;
	int vb_size_of_private;
	int vb_count_of_private;

	SAMPLE_SNS_TYPE_E sensor_type;
} priv_t;

typedef struct {
	int enc_h26x_enable : 1;
	int enc_jpg_enable : 1;
	int dec_h26x_enable : 1;
	int dec_jpg_enable : 1;
	bool vi_hmirror[MMF_VI_MAX_CHN];
	bool vi_vflip[MMF_VI_MAX_CHN];
	bool vo_video_hmirror[MMF_VO_VIDEO_MAX_CHN];
	bool vo_video_vflip[MMF_VO_VIDEO_MAX_CHN];
	bool vo_osd_hmirror[MMF_VO_OSD_MAX_CHN];
	bool vo_osd_vflip[MMF_VO_OSD_MAX_CHN];
} g_priv_t;

static priv_t priv;
static g_priv_t g_priv;

static int mmf_region_frame_push2(int ch, void *frame_info);

static int mmf_venc_deinit(int ch);
static int _mmf_venc_push(int ch, uint8_t *data, int w, int h, int format, int quality);
static int mmf_rst_venc_channel(int ch, int w, int h, int format, int quality);

static int mmf_vdec_deinit(int ch);
static int _mmf_vdec_push(int ch, VDEC_STREAM_S *stStream);
static int mmf_rst_vdec_channel(int ch, mmf_vdec_cfg_t *cfg, SIZE_S size_in);

#define DISP_W	640
#define DISP_H	480
static void priv_param_init(void)
{
	priv.vi_pop_timeout = 100;
	priv.vi_vpss = VPSS_INVALID_GRP;
	priv.vo_rotate = 90;
	priv.vo_vpss = VPSS_INVALID_GRP;
	priv.dec_pop_timeout = 1000;

	priv.vb_conf.u32MaxPoolCnt = 1;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkSize = ALIGN(DISP_W, DEFAULT_ALIGN) * ALIGN(DISP_H, DEFAULT_ALIGN) * 3;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkCnt = 8;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	priv.vb_conf.u32MaxPoolCnt ++;

	priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
	priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkCnt = 2;
	priv.vb_conf.astCommPool[MMF_VB_USER_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	priv.vb_conf.u32MaxPoolCnt ++;

	g_priv.enc_h26x_enable = 1;
	if (g_priv.enc_h26x_enable) {
		priv.vb_conf.astCommPool[MMF_VB_ENC_H26X_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
		priv.vb_conf.astCommPool[MMF_VB_ENC_H26X_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_ENC_H26X_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	g_priv.enc_jpg_enable = 1;
	if (g_priv.enc_jpg_enable) {
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	g_priv.dec_h26x_enable = 1;
	if (g_priv.dec_h26x_enable) {
		priv.vb_conf.astCommPool[MMF_VB_DEC_H26X_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3;
		priv.vb_conf.astCommPool[MMF_VB_DEC_H26X_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_DEC_H26X_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	g_priv.dec_jpg_enable = 1;
	if (g_priv.dec_jpg_enable) {
		priv.vb_conf.astCommPool[MMF_VB_DEC_JPEG_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3;
		priv.vb_conf.astCommPool[MMF_VB_DEC_JPEG_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_DEC_JPEG_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	if (priv.vb_of_vi_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].u32BlkSize = priv.vb_size_of_vi;
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].u32BlkCnt = priv.vb_count_of_vi;
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}

	if (priv.vb_of_vo_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkSize = priv.vb_size_of_vo;
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkCnt = priv.vb_count_of_vo;
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}

	if (priv.vb_of_private_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkSize = priv.vb_size_of_private;
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkCnt = priv.vb_count_of_private;
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}
}

static SAMPLE_VI_CONFIG_S g_stViConfig;
static SAMPLE_INI_CFG_S g_stIniCfg;

void mmf_dump_grpattr(VPSS_GRP_ATTR_S *GrpAttr) {
	printf("GrpAttr->u32MaxW: %d, GrpAttr->u32MaxH: %d\r\n", GrpAttr->u32MaxW, GrpAttr->u32MaxH);
	printf("GrpAttr->enPixelFormat:%d \r\n", GrpAttr->enPixelFormat);
	printf("GrpAttr->stFrameRate.s32SrcFrameRate:%d \r\n", GrpAttr->stFrameRate.s32SrcFrameRate);
	printf("GrpAttr->stFrameRate.s32DstFrameRate:%d \r\n", GrpAttr->stFrameRate.s32DstFrameRate);
	printf("GrpAttr->u8VpssDev:%d\n", GrpAttr->u8VpssDev);
}

void mmf_dump_venc_chn_status(VENC_CHN_STATUS_S *status) {
	if (status == NULL) {
		printf("status is null\n");
		return;
	}

	printf("u32LeftPics:\t\t%d\n", status->u32LeftPics);
	printf("u32LeftStreamBytes:\t\t%d\n", status->u32LeftStreamBytes);
	printf("u32LeftStreamFrames:\t\t%d\n", status->u32LeftStreamFrames);
	printf("u32CurPacks:\t\t%d\n", status->u32CurPacks);
	printf("u32LeftRecvPics:\t\t%d\n", status->u32LeftRecvPics);
	printf("u32LeftEncPics:\t\t%d\n", status->u32LeftEncPics);
	printf("bJpegSnapEnd:\t\t%d\n", status->bJpegSnapEnd);
	printf("stVencStrmInfo:\t\tnone\n"); // status->stVencStrmInfo
}

void mmf_dump_chnattr(VPSS_CHN_ATTR_S *ChnAttr) {
	printf("ChnAttr->u32Width:%d \r\n", ChnAttr->u32Width);
	printf("ChnAttr->u32Height:%d \r\n", ChnAttr->u32Height);
	printf("ChnAttr->enVideoFormat:%d \r\n", ChnAttr->enVideoFormat);
	printf("ChnAttr->enPixelFormat:%d \r\n", ChnAttr->enPixelFormat);
	printf("ChnAttr->stFrameRate.s32SrcFrameRate:%d \r\n", ChnAttr->stFrameRate.s32SrcFrameRate);
	printf("ChnAttr->stFrameRate.s32DstFrameRate:%d \r\n", ChnAttr->stFrameRate.s32DstFrameRate);
	printf("ChnAttr->bMirror:%d \r\n", ChnAttr->bMirror);
	printf("ChnAttr->bFlip:%d \r\n", ChnAttr->bFlip);
	printf("ChnAttr->u32Depth:%d \r\n", ChnAttr->u32Depth);
	printf("ChnAttr->stAspectRatio.bEnable:%d \r\n", ChnAttr->stAspectRatio.enMode);
	printf("ChnAttr->stAspectRatio.u32BgColor:%d \r\n", ChnAttr->stAspectRatio.u32BgColor);
	printf("ChnAttr->stAspectRatio.bEnableBgColor:%d \r\n", ChnAttr->stAspectRatio.bEnableBgColor);
	printf("ChnAttr->stAspectRatio.stVideoRect.s32X:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.s32X);
	printf("ChnAttr->stAspectRatio.stVideoRect.s32Y:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.s32Y);
	printf("ChnAttr->stAspectRatio.stVideoRect.u32Width:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.u32Width);
	printf("ChnAttr->stAspectRatio.stVideoRect.u32Height:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.u32Height);
	printf("ChnAttr->stNormalize.bEnable:%d \r\n", ChnAttr->stNormalize.bEnable);
	for (int i = 0; i < 3; i ++)
		printf("ChnAttr->stLumaScale.factor[%d]:%f \r\n", i, ChnAttr->stNormalize.factor[i]);
	for (int i = 0; i < 3; i ++)
		printf("ChnAttr->stLumaScale.mean[%d]:%f \r\n", i, ChnAttr->stNormalize.mean[i]);
	printf("ChnAttr->stNormalize.rounding:%d \r\n", ChnAttr->stNormalize.rounding);
}

void mmf_dump_frame(VIDEO_FRAME_INFO_S *frame) {
	if (frame == NULL) {
		printf("frame is null\n");
		return;
	}

	VIDEO_FRAME_S *vframe = &frame->stVFrame;
	printf("u32Width:\t\t%d\n", vframe->u32Width);
	printf("u32Height:\t\t%d\n", vframe->u32Height);
	printf("u32Stride[0]:\t\t%d\n", vframe->u32Stride[0]);
	printf("u32Stride[1]:\t\t%d\n", vframe->u32Stride[1]);
	printf("u32Stride[2]:\t\t%d\n", vframe->u32Stride[2]);
	printf("u32Length[0]:\t\t%d\n", vframe->u32Length[0]);
	printf("u32Length[1]:\t\t%d\n", vframe->u32Length[1]);
	printf("u32Length[2]:\t\t%d\n", vframe->u32Length[2]);
	printf("u64PhyAddr[0]:\t\t%#llx\n", vframe->u64PhyAddr[0]);
	printf("u64PhyAddr[1]:\t\t%#llx\n", vframe->u64PhyAddr[1]);
	printf("u64PhyAddr[2]:\t\t%#llx\n", vframe->u64PhyAddr[2]);
	printf("pu8VirAddr[0]:\t\t%p\n", vframe->pu8VirAddr[0]);
	printf("pu8VirAddr[1]:\t\t%p\n", vframe->pu8VirAddr[1]);
	printf("pu8VirAddr[2]:\t\t%p\n", vframe->pu8VirAddr[2]);

	printf("enPixelFormat:\t\t%d\n", vframe->enPixelFormat);
	printf("enBayerFormat:\t\t%d\n", vframe->enBayerFormat);
	printf("enVideoFormat:\t\t%d\n", vframe->enVideoFormat);
	printf("enCompressMode:\t\t%d\n", vframe->enCompressMode);
	printf("enDynamicRange:\t\t%d\n", vframe->enDynamicRange);
	printf("enColorGamut:\t\t%d\n", vframe->enColorGamut);

	printf("s16OffsetTop:\t\t%d\n", vframe->s16OffsetTop);
	printf("s16OffsetBottom:\t\t%d\n", vframe->s16OffsetBottom);
	printf("s16OffsetLeft:\t\t%d\n", vframe->s16OffsetLeft);
	printf("s16OffsetRight:\t\t%d\n", vframe->s16OffsetRight);
}

static int _free_leak_memory_of_ion(void)
{
	#define MAX_LINE_LENGTH 256
	FILE *fp;
	char line[MAX_LINE_LENGTH];
	char alloc_buf_size_str[20], phy_addr_str[20], buffer_name[20];
	int alloc_buf_size;
	uint64_t phy_addr;

	fp = fopen("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary", "r");
	if (fp == NULL) {
		fprintf(stderr, "Error opening file\n");
		return 1;
	}

	while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
		if (sscanf(line, "%*d %s %s %*d %s", alloc_buf_size_str, phy_addr_str, buffer_name) == 3) {
			if (strcmp(buffer_name, "VI_DMA_BUF")
				&& strcmp(buffer_name, "ISP_SHARED_BUFFER_0"))
				continue;
			struct sys_ion_data2 ion_data;

			alloc_buf_size = atoi(alloc_buf_size_str);
			phy_addr = (unsigned int)strtol(phy_addr_str, NULL, 16);

			memset(&ion_data, 0, sizeof(ion_data));
			ion_data.cached = 1;
			ion_data.dmabuf_fd = (uint32_t)-1;
			ion_data.size = alloc_buf_size;
			ion_data.addr_p = phy_addr;
			memset(ion_data.name, 0, sizeof(ion_data.name));
			strcpy((char *)ion_data.name, buffer_name);

			printf("alloc_buf_size(%s): %d, phy_addr(%s): %#lx, buffer_name: %s\n",
						alloc_buf_size_str, alloc_buf_size, phy_addr_str, phy_addr, buffer_name);

			printf("ion_data.size:%d, ion_data.addr_p:%#x, ion_data.name:%s\r\n", ion_data.size, (int)ion_data.addr_p, ion_data.name);

			int res = ionFree((struct sys_ion_data *)&ion_data);
			if (res) {
				printf("ionFree2 failed! res:%#x\r\n", res);
				mmf_deinit();
				return -1;
			}
		}
	}

	fclose(fp);

	return 0;
}

static CVI_S32 _mmf_init_frame(int id, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat,  VIDEO_FRAME_INFO_S *pstVideoFrame, VB_CAL_CONFIG_S *pstVbCfg)
{
	VIDEO_FRAME_S *pstVFrame;
	VB_BLK blk;

	if ((CVI_U32)id >= priv.vb_conf.u32MaxPoolCnt) {
		printf("Invalid vb pool. id: %d\n", id);
		return CVI_FAILURE;
	}
	if (!pstVideoFrame || !pstVbCfg) {
		return CVI_FAILURE;
	}

	pstVFrame = &pstVideoFrame->stVFrame;

	pstVFrame->enCompressMode = COMPRESS_MODE_NONE;
	pstVFrame->enPixelFormat = enPixelFormat;
	pstVFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
	pstVFrame->enColorGamut = COLOR_GAMUT_BT709;
	pstVFrame->u32Width = stSize.u32Width;
	pstVFrame->u32Height = stSize.u32Height;
	pstVFrame->u32TimeRef = 0;
	pstVFrame->u64PTS = 0;
	pstVFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

	int retry_cnt = 0;
_retry:
	blk = CVI_VB_GetBlock(id, pstVbCfg->u32VBSize);
	if (blk == VB_INVALID_HANDLE) {
		if (retry_cnt ++ < 5) {
			usleep(1000);
			goto _retry;
		}
		printf("Can't acquire vb block. id: %d size:%d\n", id, pstVbCfg->u32VBSize);
		return CVI_FAILURE;
	}

	pstVideoFrame->u32PoolId = CVI_VB_Handle2PoolId(blk);
	pstVFrame->u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
	pstVFrame->u32Stride[0] = pstVbCfg->u32MainStride;
	pstVFrame->u32Length[0] = pstVbCfg->u32MainYSize;
	pstVFrame->pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[0], pstVbCfg->u32VBSize);

	if (pstVbCfg->plane_num == 2) {
		pstVFrame->u64PhyAddr[1] = pstVFrame->u64PhyAddr[0] + ALIGN(pstVbCfg->u32MainYSize, pstVbCfg->u16AddrAlign);
		pstVFrame->u32Stride[1] = pstVbCfg->u32CStride;
		pstVFrame->u32Length[1] = pstVbCfg->u32MainCSize;
		pstVFrame->pu8VirAddr[1] = (CVI_U8 *)pstVFrame->pu8VirAddr[0] + pstVFrame->u32Length[0];
	}
	if (pstVbCfg->plane_num == 3) {
		pstVFrame->u64PhyAddr[2] = pstVFrame->u64PhyAddr[1] + ALIGN(pstVbCfg->u32MainCSize, pstVbCfg->u16AddrAlign);
		pstVFrame->u32Stride[2] = pstVbCfg->u32CStride;
		pstVFrame->u32Length[2] = pstVbCfg->u32MainCSize;
		pstVFrame->pu8VirAddr[2] = (CVI_U8 *)pstVFrame->pu8VirAddr[1] + pstVFrame->u32Length[1];
	}

	CVI_U32 total_size = pstVFrame->u32Length[0] + pstVFrame->u32Length[1] + pstVFrame->u32Length[2];
	if (total_size > pstVbCfg->u32VBSize)
		printf("vb block. id: %d size:%d < %d\n", pstVideoFrame->u32PoolId, pstVbCfg->u32VBSize, total_size);

	// CVI_VENC_TRACE("pool id: %u\n", pstVideoFrame->u32PoolId);
	// CVI_VENC_TRACE("phy addr(%#llx, %#llx, %#llx), Size %x\n", (long long)pstVFrame->u64PhyAddr[0]
	// 	, (long long)pstVFrame->u64PhyAddr[1], (long long)pstVFrame->u64PhyAddr[2], pstVbCfg->u32VBSize);
	// CVI_VENC_TRACE("vir addr(%p, %p, %p), Size %x\n", pstVFrame->pu8VirAddr[0]
	// 	, pstVFrame->pu8VirAddr[1], pstVFrame->pu8VirAddr[2], pstVbCfg->u32MainSize);

	return CVI_SUCCESS;
}

static VIDEO_FRAME_INFO_S *_mmf_alloc_frame(int id, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat)
{
	VIDEO_FRAME_INFO_S *pstVideoFrame;
	VB_CAL_CONFIG_S stVbCfg;

	pstVideoFrame = (VIDEO_FRAME_INFO_S *)calloc(sizeof(*pstVideoFrame), 1);
	if (pstVideoFrame == NULL) {
		printf("Failed to allocate VIDEO_FRAME_INFO_S\n");
		return NULL;
	}

	memset(&stVbCfg, 0, sizeof(stVbCfg));
	VENC_GetPicBufferConfig(stSize.u32Width,
				stSize.u32Height,
				enPixelFormat,
				DATA_BITWIDTH_8,
				COMPRESS_MODE_NONE,
				&stVbCfg);

	if (_mmf_init_frame(id, stSize, enPixelFormat, pstVideoFrame, &stVbCfg) != CVI_SUCCESS)
	{
		free(pstVideoFrame);
		return NULL;
	}

	return pstVideoFrame;
}

static CVI_S32 _mmf_deinit_frame(VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VIDEO_FRAME_S *pstVFrame;
	VB_BLK blk;

	if (!pstVideoFrame)
		return CVI_FAILURE;

	pstVFrame = &pstVideoFrame->stVFrame;

	if (pstVFrame->pu8VirAddr[2] == (CVI_U8 *)pstVFrame->pu8VirAddr[1] + pstVFrame->u32Length[1])
	{
		pstVFrame->u32Length[1] += pstVFrame->u32Length[2];
		pstVFrame->u32Length[2] = 0;
		pstVFrame->pu8VirAddr[2] = NULL;
		pstVFrame->u64PhyAddr[2] = 0;
	}
	if (pstVFrame->pu8VirAddr[1] == (CVI_U8 *)pstVFrame->pu8VirAddr[0] + pstVFrame->u32Length[0])
	{
		pstVFrame->u32Length[0] += pstVFrame->u32Length[1];
		pstVFrame->u32Length[1] = 0;
		pstVFrame->pu8VirAddr[1] = NULL;
		pstVFrame->u64PhyAddr[1] = 0;
	}

	for (int b = 0; b < 3; b++) {
		if (pstVFrame->pu8VirAddr[b]) {
			CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[b], pstVFrame->u32Length[b]);
			pstVFrame->u32Length[b] = 0;
			pstVFrame->pu8VirAddr[b] = NULL;
		}

		if (pstVFrame->u64PhyAddr[b]) {
			blk = CVI_VB_PhysAddr2Handle(pstVFrame->u64PhyAddr[b]);
			if (blk != VB_INVALID_HANDLE) {
				CVI_VB_ReleaseBlock(blk);
			}
			pstVFrame->u64PhyAddr[b] = 0;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 _mmf_free_frame(VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	if (_mmf_deinit_frame(pstVideoFrame) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	free(pstVideoFrame);

	return CVI_SUCCESS;
}

static int cvi_ive_init(void)
{
	CVI_S32 s32Ret;
	if (priv.ive_is_init)
		return 0;

	priv.ive_rgb2yuv_w = 640;
	priv.ive_rgb2yuv_h = 480;
	priv.ive_handle = CVI_IVE_CreateHandle();
	if (priv.ive_handle == NULL) {
		printf("CVI_IVE_CreateHandle failed!\n");
		return -1;
	}

	s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, IVE_IMAGE_TYPE_U8C3_PACKAGE, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
	if (s32Ret != CVI_SUCCESS) {
		printf("Create src image failed!\n");
		CVI_IVE_DestroyHandle(priv.ive_handle);
		return -1;
	}

	s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img, IVE_IMAGE_TYPE_YUV420SP, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
	if (s32Ret != CVI_SUCCESS) {
		printf("Create src image failed!\n");
		CVI_IVE_DestroyHandle(priv.ive_handle);
		return -1;
	}

	priv.ive_is_init = 1;
	return 0;
}

static int cvi_ive_deinit(void)
{
	if (priv.ive_is_init == 0)
		return 0;

	CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img);
	CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img);
	CVI_IVE_DestroyHandle(priv.ive_handle);

	priv.ive_is_init = 0;
	return 0;
}

static int cvi_rgb2nv21(uint8_t *src, int input_w, int input_h)
{
	CVI_S32 s32Ret;

	int width = ALIGN(input_w, DEFAULT_ALIGN);
	int height = input_h;

	if (width != priv.ive_rgb2yuv_w || height != priv.ive_rgb2yuv_h) {
		CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img);
		CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img);
		priv.ive_rgb2yuv_w = width;
		priv.ive_rgb2yuv_h = height;
		printf("reinit rgb2nv21 buffer, buff w:%d h:%d\n", priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, IVE_IMAGE_TYPE_U8C3_PACKAGE, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		if (s32Ret != CVI_SUCCESS) {
			printf("Create src image failed!\n");
			return -1;
		}

		s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img, IVE_IMAGE_TYPE_YUV420SP, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		if (s32Ret != CVI_SUCCESS) {
			printf("Create src image failed!\n");
			return -1;
		}
	}

	if (width != input_w) {
		for (int h = 0; h < height; h++) {
			memcpy((uint8_t *)priv.ive_rgb2yuv_rgb_img.u64VirAddr[0] + width * h * 3, (uint8_t *)src + input_w * h * 3, input_w * 3);
		}
	} else {
		memcpy((uint8_t *)priv.ive_rgb2yuv_rgb_img.u64VirAddr[0], (uint8_t *)src, width * height * 3);
	}

	IVE_CSC_CTRL_S stCtrl;
	stCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_RGB2YUV;
	s32Ret = CVI_IVE_CSC(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, &priv.ive_rgb2yuv_yuv_img, &stCtrl, 1);
	if (s32Ret != CVI_SUCCESS) {
		printf("Run HW IVE CSC YUV2RGB failed!\n");
		return -1;
	}
	return 0;
}

static int _try_release_sys(void)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	SAMPLE_INI_CFG_S	   	stIniCfg;
	SAMPLE_VI_CONFIG_S 		stViConfig;
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		printf("Parse complete\n");
		return s32Ret;
	}

	priv.sensor_type = stIniCfg.enSnsType[0];

	s32Ret = CVI_VI_SetDevNum(stIniCfg.devNum);
	if (s32Ret != CVI_SUCCESS) {
		printf("VI_SetDevNum failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		printf("SAMPLE_COMM_VI_IniToViCfg failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_DestroyIsp(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		printf("SAMPLE_COMM_VI_DestroyIsp failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_DestroyVi(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		printf("SAMPLE_COMM_VI_DestroyVi failed with %#x\n", s32Ret);
		return s32Ret;
	}

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

int _try_release_vio_all(void)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	s32Ret = mmf_del_vi_channel_all();
	if (s32Ret != CVI_SUCCESS) {
		printf("mmf_del_vi_channel_all failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = mmf_del_vo_channel_all(0);
	if (s32Ret != CVI_SUCCESS) {
		printf("mmf_del_vo_channel_all failed with %#x\n", s32Ret);
		return s32Ret;
	}
	return s32Ret;
}

void mmf_pre_config_sys(mmf_sys_cfg_t *cfg)
{
	UNUSED(cfg);
	// TODO support custom buffer pools
}

static void _mmf_sys_exit(void)
{
	if (g_stViConfig.s32WorkingViNum != 0) {
		SAMPLE_COMM_VI_DestroyIsp(&g_stViConfig);
		SAMPLE_COMM_VI_DestroyVi(&g_stViConfig);
	}
	SAMPLE_COMM_SYS_Exit();
}

static CVI_S32 _mmf_sys_init(SIZE_S stSize)
{
	VB_CONFIG_S stVbConf;
	CVI_U32 u32BlkSize, u32BlkRotSize;
	CVI_U32 u32TotalSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	COMPRESS_MODE_E enCompressMode   = COMPRESS_MODE_NONE;

	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	memcpy(&stVbConf, &priv.vb_conf, sizeof(VB_CONFIG_S));
	// stVbConf.u32MaxPoolCnt		= 3;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, priv.vi_format,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, priv.vi_format,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkSize = MAX(u32BlkSize, u32BlkRotSize);

	stVbConf.astCommPool[MMF_VB_VI_ID].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[MMF_VB_VI_ID].u32BlkCnt	= 3;
	stVbConf.astCommPool[MMF_VB_VI_ID].enRemapMode	= VB_REMAP_MODE_CACHED;
#if 0
{
	VB_CONFIG_S vb_config;
	s32Ret = CVI_VB_GetConfig(&vb_config);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VB_GetConfig NG\n");
		return CVI_FAILURE;
	}
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	printf("vb_config.u32MaxPoolCnt :%d\r\n", stVbConf.u32MaxPoolCnt);
	for (int i = 0; i < stVbConf.u32MaxPoolCnt; ++i) {
		printf("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						stVbConf.astCommPool[i].u32BlkSize, \
						stVbConf.astCommPool[i].u32BlkCnt, \
						stVbConf.astCommPool[i].enRemapMode);
	}
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	printf("vb_config.u32MaxPoolCnt :%d\r\n", vb_config.u32MaxPoolCnt);
	for (int i = 0; i < vb_config.u32MaxPoolCnt; ++i) {
		printf("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						vb_config.astCommPool[i].u32BlkSize, \
						vb_config.astCommPool[i].u32BlkCnt, \
						vb_config.astCommPool[i].enRemapMode);
	}
}
#endif
	u32TotalSize = 0;
	for (CVI_U32 i = 0; i < stVbConf.u32MaxPoolCnt; ++i) {
		u32TotalSize += stVbConf.astCommPool[i].u32BlkSize * stVbConf.astCommPool[i].u32BlkCnt;
	}
	printf("common pools total size: %u KiB\n", u32TotalSize / 1024);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		printf("system init failed with %#x\n", s32Ret);
		goto error;
	}
#if 0
{
	VB_CONFIG_S vb_config;
	s32Ret = CVI_VB_GetConfig(&vb_config);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VB_GetConfig NG\n");
		return CVI_FAILURE;
	}
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	for (int i = 0; i < vb_config.u32MaxPoolCnt; ++i) {
		printf("vb_config.u32MaxPoolCnt :%d\r\n", vb_config.u32MaxPoolCnt);
		printf("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						vb_config.astCommPool[i].u32BlkSize, \
						vb_config.astCommPool[i].u32BlkCnt, \
						vb_config.astCommPool[i].enRemapMode);
	}
}
#endif
	return s32Ret;
error:
	_mmf_sys_exit();
	return s32Ret;
}

static CVI_S32 _mmf_vpss_deinit(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	CVI_S32 s32Ret = CVI_SUCCESS;

	/*stop vpss*/
	abChnEnable[VpssChn] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
	if (s32Ret != CVI_SUCCESS) {
		printf("stop vpss group failed. s32Ret: 0x%x !\n", s32Ret);
	}

	return s32Ret;
}

static CVI_S32 _mmf_vpss_deinit_new(VPSS_GRP VpssGrp)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_VPSS_StopGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		printf("Vpss Stop Grp %d failed! Please check param\n", VpssGrp);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_DestroyGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		printf("Vpss Destroy Grp %d failed! Please check\n", VpssGrp);
		return CVI_FAILURE;
	}

	return s32Ret;
}

// fit = 0, width to new width, height to new height, may be stretch
// fit = 1, keep aspect ratio, fill blank area with black color
// fit = other, keep aspect ratio, crop image to fit new size
static CVI_S32 _mmf_vpss_init(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SIZE_S stSizeIn, SIZE_S stSizeOut, PIXEL_FORMAT_E formatIn, PIXEL_FORMAT_E formatOut,
int fps, int depth, bool mirror, bool flip, int fit)
{
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	VPSS_CROP_INFO_S stGrpCropInfo;
	CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
	CVI_S32 s32Ret = CVI_SUCCESS;

	memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = formatIn;
	stVpssGrpAttr.u32MaxW                        = stSizeIn.u32Width;
	stVpssGrpAttr.u32MaxH                        = stSizeIn.u32Height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	CVI_FLOAT corp_scale_w = (CVI_FLOAT)stSizeIn.u32Width / stSizeOut.u32Width;
	CVI_FLOAT corp_scale_h = (CVI_FLOAT)stSizeIn.u32Height / stSizeOut.u32Height;
	CVI_U32 crop_w = -1, crop_h = -1;
	if (fit == 0) {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.s32X       = 0;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.s32Y       = 0;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.u32Width   = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.u32Height  = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		stGrpCropInfo.bEnable = false;
	} else if (fit == 1) {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		stGrpCropInfo.bEnable = false;
	} else {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		crop_w = corp_scale_w < corp_scale_h ? stSizeOut.u32Width * corp_scale_w: stSizeOut.u32Width * corp_scale_h;
		crop_h = corp_scale_w < corp_scale_h ? stSizeOut.u32Height * corp_scale_w: stSizeOut.u32Height * corp_scale_h;
		if (corp_scale_h < 0 || corp_scale_w < 0) {
			printf("crop scale error. corp_scale_w: %f, corp_scale_h: %f\n", corp_scale_w, corp_scale_h);
			goto error;
		}

		stGrpCropInfo.bEnable = true;
		stGrpCropInfo.stCropRect.s32X = (stSizeIn.u32Width - crop_w) / 2;
		stGrpCropInfo.stCropRect.s32Y = (stSizeIn.u32Height - crop_h) / 2;
		stGrpCropInfo.stCropRect.u32Width = crop_w;
		stGrpCropInfo.stCropRect.u32Height = crop_h;
	}

	/*start vpss*/
	abChnEnable[0] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("init vpss group failed. s32Ret: 0x%x ! retry!!!\n", s32Ret);
		s32Ret = SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
		if (s32Ret != CVI_SUCCESS) {
			printf("stop vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		}
		s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			printf("retry to init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
			return s32Ret;
		} else {
			printf("retry to init vpss group ok!\n");
		}
	}

	if (crop_w != 0 && crop_h != 0) {
		s32Ret = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stGrpCropInfo);
		if (s32Ret != CVI_SUCCESS) {
			printf("set vpss group crop failed. s32Ret: 0x%x !\n", s32Ret);
			goto error;
		}
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		goto error;
	}

	return s32Ret;
error:
	_mmf_vpss_deinit(VpssGrp, VpssChn);
	return s32Ret;
}

static CVI_S32 _mmf_init(void)
{
	MMF_VERSION_S stVersion;
	SAMPLE_INI_CFG_S	   stIniCfg;
	SAMPLE_VI_CONFIG_S stViConfig;

	PIC_SIZE_E enPicSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	LOG_LEVEL_CONF_S log_conf;

	CVI_SYS_GetVersion(&stVersion);
	printf("maix multi-media version:%s\n", stVersion.version);

	log_conf.enModId = CVI_ID_LOG;
	log_conf.s32Level = CVI_DBG_DEBUG;
	CVI_LOG_SetLevelConf(&log_conf);

	// Get config from ini if found.
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		printf("Parse complete\n");
	}

	//Set sensor number
	CVI_VI_SetDevNum(stIniCfg.devNum);

	/************************************************
	 * step1:  Config VI
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	memcpy(&g_stViConfig, &stViConfig, sizeof(SAMPLE_VI_CONFIG_S));
	memcpy(&g_stIniCfg, &stIniCfg, sizeof(SAMPLE_INI_CFG_S));

	/************************************************
	 * step2:  Get input size
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType[0], &enPicSize);
	if (s32Ret != CVI_SUCCESS) {
		printf("SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
	if (s32Ret != CVI_SUCCESS) {
		printf("SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
		return s32Ret;
	}

	/************************************************
	 * step3:  Init modules
	 ************************************************/
	if (0 != _free_leak_memory_of_ion()) {
		printf("free leak memory error\n");
	}

	s32Ret = _mmf_sys_init(stSize);
	if (s32Ret != CVI_SUCCESS) {
		printf("sys init failed. s32Ret: 0x%x !\n", s32Ret);
		goto _need_exit_sys_and_deinit_vi;
	}

	s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		printf("vi init failed. s32Ret: 0x%x !\n", s32Ret);
		printf("Please try to check if the camera is working.\n");
		goto _need_exit_sys_and_deinit_vi;
	}

	priv.vi_max_size.u32Width = stSize.u32Width;
	priv.vi_max_size.u32Height = stSize.u32Height;
	priv.vi_size.u32Width = stSize.u32Width;
	priv.vi_size.u32Height = stSize.u32Height;

	return s32Ret;

_need_exit_sys_and_deinit_vi:
	_mmf_sys_exit();

	return s32Ret;
}

static void _mmf_deinit(void)
{
	_mmf_sys_exit();
}

static int _vi_get_unused_ch() {
	for (int i = 0; i < MMF_VI_MAX_CHN; i++) {
		if (priv.vi_chn_is_inited[i] == false) {
			return i;
		}
	}
	return -1;
}

char* mmf_get_sensor_name(void)
{
	static char name[30];

	switch (priv.sensor_type) {
	    case GCORE_GC4653_MIPI_4M_30FPS_10BIT:
	    case GCORE_GC4653_MIPI_720P_60FPS_10BIT:
		snprintf(name, sizeof(name), "gcore_gc4653");
		return name;
	    case SMS_SC035GS_MIPI_480P_120FPS_12BIT:
		snprintf(name, sizeof(name), "sms_sc035gs");
		return name;
	    case LONTIUM_LT6911_2M_60FPS_8BIT:
		snprintf(name, sizeof(name), "lt6911");
		return name;
	    case OV_OS04A10_MIPI_4M_1440P_30FPS_12BIT:
		snprintf(name, sizeof(name), "ov_os04a10");
		return name;
	    case GCORE_OV2685_MIPI_1600x1200_30FPS_10BIT:
		snprintf(name, sizeof(name), "ov_ov2685");
		return name;
	    default:
		break;
	}

	snprintf(name, sizeof(name), "gcore_gc4653");
	return name;
}

int mmf_vi_format_init(void)
{
	PIXEL_FORMAT_E vi_format = MMF_VI_PIXEL_FORMAT;
	PIXEL_FORMAT_E vi_vpss_format = MMF_VI_PIXEL_FORMAT;

	// config vi param
	char *sensor_name = mmf_get_sensor_name();

	if (!strcmp(sensor_name, "sms_sc035gs")) {
		vi_format = PIXEL_FORMAT_NV21;
		vi_vpss_format = PIXEL_FORMAT_YUV_400;
	} else if (!strcmp(sensor_name, "ov_ov2685")) {
		vi_format = PIXEL_FORMAT_NV21;
		vi_vpss_format = PIXEL_FORMAT_NV21;
	} else if (!strcmp(sensor_name, "lt6911")) {
		vi_format = PIXEL_FORMAT_UYVY;
		vi_vpss_format = PIXEL_FORMAT_UYVY;
	} else if (!strcmp(sensor_name, "ov_os04a10")) {
		vi_format = PIXEL_FORMAT_NV21;
		vi_vpss_format = PIXEL_FORMAT_NV21;
	} else { // default is gcore_gc4653
		vi_format = PIXEL_FORMAT_NV21;
		vi_vpss_format = PIXEL_FORMAT_NV21;
	}

	priv.vi_format = vi_format;
	priv.vi_vpss_format = vi_vpss_format;

	return 0;
}

int mmf_init(void)
{
	if (priv.mmf_used_cnt) {
		priv.mmf_used_cnt ++;
		printf("maix multi-media already inited(cnt:%d)\n", priv.mmf_used_cnt);
		return 0;
	}

	priv_param_init();

	if (_try_release_sys() != CVI_SUCCESS) {
		printf("try release sys failed\n");
		return -1;
	} else {
		printf("try release sys ok\n");
	}

	mmf_vi_format_init();

	if (_mmf_init() != CVI_SUCCESS) {
		printf("maix multi-media init failed\n");
		return -1;
	} else {
		printf("maix multi-media init ok\n");
	}

#if MMF_VO_USE_NV21_ONLY
	if (cvi_ive_init() != CVI_SUCCESS) {
		printf("cvi_ive_init failed\n");
		return -1;
	} else {
		printf("cvi_ive_init ok\n");
	}
#else
	UNUSED(cvi_ive_init);
#endif
	priv.mmf_used_cnt = 1;

	if (_try_release_vio_all() != CVI_SUCCESS) {
		printf("try release vio failed\n");
		return -1;
	} else {
		printf("try release vio ok\n");
	}

	return 0;
}

bool mmf_is_init(void)
{
	return priv.mmf_used_cnt > 0 ? true : false;
}

int mmf_deinit(void) {
	if (!priv.mmf_used_cnt) {
		return 0;
	}

	priv.mmf_used_cnt --;

	if (priv.mmf_used_cnt) {
		return 0;
	} else {
		printf("maix multi-media driver destroyed.\n");
#if MMF_VO_USE_NV21_ONLY
		cvi_ive_deinit();
#else
		UNUSED(cvi_ive_deinit);
#endif

		mmf_del_vi_channel_all();
		mmf_del_vo_channel_all(0);
		mmf_del_venc_channel_all();
		mmf_vi_deinit();
		mmf_del_region_channel_all();
		_mmf_deinit();
	}

	return 0;
}

int mmf_try_deinit(bool force)
{
	UNUSED(force);
	return mmf_deinit();
}

int mmf_get_vi_unused_channel(void) {
	return _vi_get_unused_ch();
}

static CVI_S32 _mmf_vpss_chn_init(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, int width, int height, PIXEL_FORMAT_E format, int fps, int depth, bool mirror, bool flip, int fit)
{
#if 1
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CROP_INFO_S stChnCropInfo;
	VPSS_CHN_ATTR_S chn_attr;
	CVI_S32 s32Ret = CVI_SUCCESS;

	memset(&chn_attr, 0, sizeof(chn_attr));
	s32Ret = CVI_VPSS_GetGrpAttr(VpssGrp, &stGrpAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_GetGrpAttr failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}
	CVI_FLOAT corp_scale_w = (CVI_FLOAT)stGrpAttr.u32MaxW / width;
	CVI_FLOAT corp_scale_h = (CVI_FLOAT)stGrpAttr.u32MaxH / height;
	CVI_U32 crop_w = -1, crop_h = -1;
	if (fit == 0) {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
		chn_attr.stAspectRatio.stVideoRect.s32X       = 0;
		chn_attr.stAspectRatio.stVideoRect.s32Y       = 0;
		chn_attr.stAspectRatio.stVideoRect.u32Width   = width;
		chn_attr.stAspectRatio.stVideoRect.u32Height  = height;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		stChnCropInfo.bEnable = false;
	} else if (fit == 1) {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		stChnCropInfo.bEnable = false;
	} else {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		crop_w = corp_scale_w < corp_scale_h ? width * corp_scale_w: width * corp_scale_h;
		crop_h = corp_scale_w < corp_scale_h ? height * corp_scale_w: height * corp_scale_h;
		if (corp_scale_h < 0 || corp_scale_w < 0) {
			printf("crop scale error. corp_scale_w: %f, corp_scale_h: %f\n", corp_scale_w, corp_scale_h);
			return -1;
		}

		stChnCropInfo.bEnable = true;
		stChnCropInfo.stCropRect.s32X = (stGrpAttr.u32MaxW - crop_w) / 2;
		stChnCropInfo.stCropRect.s32Y = (stGrpAttr.u32MaxH - crop_h) / 2;
		stChnCropInfo.stCropRect.u32Width = crop_w;
		stChnCropInfo.stCropRect.u32Height = crop_h;
	}

	if (crop_w != 0 && crop_h != 0) {
		s32Ret = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stChnCropInfo);
		if (s32Ret != CVI_SUCCESS) {
			printf("set vpss group crop failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}
	}

	s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	return s32Ret;
#else
	CVI_S32 s32Ret;
	VPSS_CHN_ATTR_S chn_attr = {0};
	chn_attr.u32Width                    = width;
	chn_attr.u32Height                   = height;
	chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
	chn_attr.enPixelFormat               = format;
	chn_attr.stFrameRate.s32SrcFrameRate = fps;
	chn_attr.stFrameRate.s32DstFrameRate = fps;
	chn_attr.u32Depth                    = depth;
	chn_attr.bMirror                     = mirror;
	chn_attr.bFlip                       = flip;
	chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
	chn_attr.stAspectRatio.stVideoRect.s32X       = 0;
	chn_attr.stAspectRatio.stVideoRect.s32Y       = 0;
	chn_attr.stAspectRatio.stVideoRect.u32Width   = width;
	chn_attr.stAspectRatio.stVideoRect.u32Height  = height;
	chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
	chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
	chn_attr.stNormalize.bEnable         = CVI_FALSE;

	s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
#endif
}

static CVI_S32 _mmf_vpss_chn_deinit(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	return CVI_VPSS_DisableChn(VpssGrp, VpssChn);
}

static CVI_S32 _mmf_vpss_init_new_with_fps(VPSS_GRP VpssGrp, CVI_U32 width, CVI_U32 height, PIXEL_FORMAT_E format, int fps)
{
	VPSS_GRP_ATTR_S    stVpssGrpAttr;
	CVI_S32 s32Ret = CVI_SUCCESS;
	VPSS_CHN_ATTR_S astVpssChnAttr;

	memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = format;
	stVpssGrpAttr.u32MaxW                        = width;
	stVpssGrpAttr.u32MaxH                        = height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	astVpssChnAttr.stFrameRate.s32SrcFrameRate = fps;
	astVpssChnAttr.stFrameRate.s32DstFrameRate = fps;


	s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_CreateGrp(grp:%d) failed with %#x, retry!\n", VpssGrp, s32Ret);
		CVI_VPSS_DestroyGrp(VpssGrp);

		s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
			return CVI_FAILURE;
		}
	}

	CVI_VPSS_SetChnAttr(VpssGrp, VPSS_CHN0, &astVpssChnAttr);

	s32Ret = CVI_VPSS_ResetGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_ResetGrp(grp:%d) failed with %#x!%d\n", VpssGrp, s32Ret, CVI_ERR_VPSS_ILLEGAL_PARAM);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_StartGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

static CVI_S32 _mmf_vi_init(CVI_U32 width, CVI_U32 height, PIXEL_FORMAT_E format, int fps)
{
	VPSS_GRP out_grp;
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.vi_is_inited) {
		return s32Ret;
	}

	out_grp = CVI_VPSS_GetAvailableGrp();

	s32Ret = _mmf_vpss_init_new_with_fps(out_grp, width, height, format, fps);
	if (s32Ret != CVI_SUCCESS) {
		printf("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	priv.vi_vpss = out_grp;
	priv.vi_is_inited = true;

	return s32Ret;
}

int mmf_vi_init2(mmf_vi_cfg_t *vi_info)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.vi_is_inited) {
		return s32Ret;
	}

	priv.vi_format = (PIXEL_FORMAT_E)vi_info->fmt;
	priv.vi_size.u32Width = vi_info->w;
	priv.vi_size.u32Height = vi_info->h;

	s32Ret = _mmf_vi_init(priv.vi_size.u32Width, priv.vi_size.u32Height, priv.vi_vpss_format, vi_info->fps);

	return s32Ret;
}

int mmf_vi_init(void)
{
	return _mmf_vi_init(priv.vi_size.u32Width, priv.vi_size.u32Height, priv.vi_vpss_format, 60);
}

int mmf_vi_deinit(void)
{
	if (!priv.vi_is_inited) {
		return 0;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.vi_vpss != VPSS_INVALID_GRP) {
		s32Ret = _mmf_vpss_deinit_new(priv.vi_vpss);
	}
	if (s32Ret != CVI_SUCCESS) {
		printf("_mmf_vpss_deinit_new failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.vi_vpss = VPSS_INVALID_GRP;
	priv.vi_is_inited = false;

	return s32Ret;
}

static int _mmf_add_vi_channel(int ch, int width, int height, int format, int fps, int depth, bool mirror, bool flip, int fit) {
	if (!priv.mmf_used_cnt || !priv.vi_is_inited) {
		printf("%s: maix multi-media or vi not inited\n", __func__);
		return -1;
	}

	if (width <= 0 || height <= 0) {
		printf("invalid width or height\n");
		return -1;
	}

	if (format != PIXEL_FORMAT_NV21
		&& format != PIXEL_FORMAT_RGB_888) {
		printf("invalid format\n");
		return -1;
	}

	if ((format == PIXEL_FORMAT_RGB_888 && width * height * 3 > 640 * 480 * 3)
		|| (format == PIXEL_FORMAT_NV21 && width * height * 3 / 2 > 2560 * 1440 * 3 / 2)) {
		printf("image size is too large, for NV21, maximum resolution 2560x1440, for RGB888, maximum resolution 640x480!\n");
		return -1;
	}

	if (mmf_vi_chn_is_open(ch)) {
		printf("vi ch %d already open\n", ch);
		return -1;
	}

#if 0
	CVI_S32 s32Ret = CVI_SUCCESS;
	SIZE_S stSizeIn, stSizeOut;
	int fps = 30;
	int depth = 2;
	PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
	PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)format;
	stSizeIn.u32Width   = priv.vi_size.u32Width;
	stSizeIn.u32Height  = priv.vi_size.u32Height;
	stSizeOut.u32Width  = ALIGN(width, DEFAULT_ALIGN);
	stSizeOut.u32Height = height;
	bool mirror = !g_priv.vi_hmirror[ch];
	bool flip = !g_priv.vi_vflip[ch];
	s32Ret = _mmf_vpss_init(priv.vi_vpss, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, 2);
	if (s32Ret != CVI_SUCCESS) {
		printf("vpss init failed. s32Ret: 0x%x. try again..\r\n", s32Ret);
		s32Ret = _mmf_vpss_deinit(priv.vi_vpss, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}

		s32Ret = _mmf_vpss_init(priv.vi_vpss, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, 2);
		if (s32Ret != CVI_SUCCESS) {
			printf("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}
	}

	priv.vi_chn_is_inited[ch] = true;
	return 0;
_need_deinit_vpss:
	_mmf_vpss_deinit(priv.vi_vpss, ch);
	return -1;
#else
	CVI_S32 s32Ret = CVI_SUCCESS;
	int width_out = ALIGN(width, DEFAULT_ALIGN);
	int height_out = height;
	PIXEL_FORMAT_E format_out = (PIXEL_FORMAT_E)format;
	s32Ret = _mmf_vpss_chn_deinit(priv.vi_vpss, ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = _mmf_vpss_chn_init(priv.vi_vpss, ch, width_out, height_out, format_out, fps, depth, mirror, flip, fit);
	if (s32Ret != CVI_SUCCESS) {
		printf("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_VI_Bind_VPSS(priv.vi_vpss, ch, 0);
	if (s32Ret != CVI_SUCCESS) {
		printf("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		goto _need_deinit_vpss_chn;
	}

	priv.vi_chn_is_inited[ch] = true;

	return 0;
_need_deinit_vpss_chn:
	_mmf_vpss_chn_deinit(priv.vi_vpss, ch);
	return -1;
#endif
}

int mmf_add_vi_channel(int ch, int width, int height, int format) {
	return  _mmf_add_vi_channel(ch, width, height, format, 30, 2, !g_priv.vi_hmirror[ch], !g_priv.vi_vflip[ch], 2);
}

int mmf_del_vi_channel(int ch) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}

	if (priv.vi_chn_is_inited[ch] == false) {
		printf("vi ch %d not open\n", ch);
		return -1;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	s32Ret = SAMPLE_COMM_VI_UnBind_VPSS(priv.vi_vpss, ch, 0);
	if (s32Ret != CVI_SUCCESS) {
		printf("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		// return -1; // continue to deinit vpss
	}

	if (0 != _mmf_vpss_chn_deinit(priv.vi_vpss, ch)) {
		printf("_mmf_vpss_chn_deinit failed. s32Ret: 0x%x !\n", s32Ret);
	}

	priv.vi_chn_is_inited[ch] = false;
	return s32Ret;
}

int mmf_del_vi_channel_all() {
	for (int i = 0; i < MMF_VI_MAX_CHN; i++) {
		if (priv.vi_chn_is_inited[i] == true) {
			mmf_del_vi_channel(i);
		}
	}
	return 0;
}

bool mmf_vi_chn_is_open(int ch) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return false;
	}

	return priv.vi_chn_is_inited[ch];
}

int mmf_reset_vi_channel(int ch, int width, int height, int format)
{
	mmf_del_vi_channel(ch);
	return mmf_add_vi_channel(ch, width, height, format);
}

int mmf_vi_aligned_width(int ch) {
	UNUSED(ch);
	return DEFAULT_ALIGN;
}

int mmf_vi_frame_pop2(int ch, void **frame_info,  mmf_frame_info_t *frame_info_mmap) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}
	if (!priv.vi_chn_is_inited[ch]) {
		printf("vi ch %d not open\n", ch);
		return -1;
	}
	if (frame_info == NULL) {
		printf("invalid param\n");
		return -1;
	}

	int ret = -1;
	VIDEO_FRAME_INFO_S *frame = &priv.vi_frame[ch];
	if (CVI_VPSS_GetChnFrame(priv.vi_vpss, ch, frame, priv.vi_pop_timeout) == 0) {
		int image_size = frame->stVFrame.u32Length[0]
			       + frame->stVFrame.u32Length[1]
			       + frame->stVFrame.u32Length[2];
		CVI_VOID *vir_addr;
		vir_addr = CVI_SYS_MmapCache(frame->stVFrame.u64PhyAddr[0], image_size);
		CVI_SYS_IonInvalidateCache(frame->stVFrame.u64PhyAddr[0], vir_addr, image_size);

		frame->stVFrame.pu8VirAddr[0] = (CVI_U8 *)vir_addr;	// save virtual address for munmap
		// printf("width: %d, height: %d, total_buf_length: %d, phy:%#lx  vir:%p\n",
		// 	   frame->stVFrame.u32Width,
		// 	   frame->stVFrame.u32Height, image_size,
		// 	   frame->stVFrame.u64PhyAddr[0], vir_addr);

		*frame_info = frame;

		if (!frame_info_mmap)
			return 0;
		frame_info_mmap->data = vir_addr;
		frame_info_mmap->len = image_size;
		frame_info_mmap->w = frame->stVFrame.u32Width;
		frame_info_mmap->h = frame->stVFrame.u32Height;
		frame_info_mmap->fmt = frame->stVFrame.enPixelFormat;

		return 0;
	}

	return ret;
}

int mmf_vi_frame_pop(int ch, void **data, int *len, int *width, int *height, int *format) {
	int ret;
	void *frame;
	mmf_frame_info_t frame_mmap;

	if (data == NULL || len == NULL || width == NULL || height == NULL || format == NULL) {
		printf("invalid param\n");
		return -1;
	}

	ret = mmf_vi_frame_pop2(ch, &frame, &frame_mmap);
	if (ret != 0)
		return ret;

	*data = frame_mmap.data;
	*len = frame_mmap.len;
	*width = frame_mmap.w;
	*height = frame_mmap.h;
	*format = frame_mmap.fmt;

	return 0;
}

void mmf_vi_frame_free2(int ch, void **frame_info)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return;
	}
	if (!frame_info)
		return;
	VIDEO_FRAME_INFO_S *frame = &priv.vi_frame[ch];
	if (*frame_info != frame)
		return;

	mmf_vi_frame_free(ch);
}

void mmf_vi_frame_free(int ch) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return;
	}

	VIDEO_FRAME_INFO_S *frame = &priv.vi_frame[ch];
	int image_size = frame->stVFrame.u32Length[0]
		       + frame->stVFrame.u32Length[1]
		       + frame->stVFrame.u32Length[2];
	if (frame->stVFrame.pu8VirAddr[0]) {
		CVI_SYS_Munmap(frame->stVFrame.pu8VirAddr[0], image_size);
		frame->stVFrame.pu8VirAddr[0] = NULL;
	}
	if (CVI_VPSS_ReleaseChnFrame(priv.vi_vpss, ch, frame) != 0)
			printf("CVI_VI_ReleaseChnFrame failed\n");
}

// manage vo channels
int mmf_get_vo_unused_channel(int layer) {
	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		for (int i = 0; i < MMF_VO_VIDEO_MAX_CHN; i++) {
			if (priv.vo_video_chn_is_inited[i] == false) {
				return i;
			}
		}
		break;
	case MMF_VO_OSD_LAYER:
		return mmf_get_region_unused_channel();
	default:
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return -1;
}

static CVI_S32 _mmf_vo_vpss_init(CVI_U32 width, CVI_U32 height, PIXEL_FORMAT_E format)
{
	VPSS_GRP out_grp = CVI_VPSS_GetAvailableGrp();
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = _mmf_vpss_init_new_with_fps(out_grp, width, height, format, 60);
	if (s32Ret != CVI_SUCCESS) {
		return s32Ret;
	}

	priv.vo_vpss = out_grp;

	return s32Ret;
}

static CVI_S32 _mmf_vo_vpss_deinit(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (priv.vo_vpss != VPSS_INVALID_GRP) {
		s32Ret = _mmf_vpss_deinit_new(priv.vo_vpss);
	}
	if (s32Ret != CVI_SUCCESS) {
		return s32Ret;
	}

	priv.vo_vpss = VPSS_INVALID_GRP;

	return s32Ret;
}

// fit = 0, width to new width, height to new height, may be stretch
// fit = 1, keep aspect ratio, fill blank area with black color
// fit = 2, keep aspect ratio, crop image to fit new size
static int _mmf_add_vo_channel(int layer, int ch, int width, int height, int format_in, int format_out, int fps, int depth, bool mirror, bool flip, int fit) {
	if (layer == MMF_VO_VIDEO_LAYER) {
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}

		SAMPLE_VO_CONFIG_S stVoConfig;
		RECT_S stDefDispRect  = {0, 0, (CVI_U32)width, (CVI_U32)height};
		SIZE_S stDefImageSize = {(CVI_U32)width, (CVI_U32)height};
		CVI_S32 s32Ret = CVI_SUCCESS;
		CVI_U32 panel_init = false;
		VO_PUB_ATTR_S stVoPubAttr;

	#if !MMF_VO_USE_NV21_ONLY
		if (priv.vo_rotate == 90 || priv.vo_rotate == 270) {
			stDefDispRect.u32Width = (CVI_U32)height;
			stDefDispRect.u32Height = (CVI_U32)width;
			stDefImageSize.u32Width = (CVI_U32)height;
			stDefImageSize.u32Height = (CVI_U32)width;
		}
	#else
		if (format_in == PIXEL_FORMAT_NV21 && (priv.vo_rotate == 90 || priv.vo_rotate == 270)) {
			stDefDispRect.u32Width = (CVI_U32)height;
			stDefDispRect.u32Height = (CVI_U32)width;
			stDefImageSize.u32Width = (CVI_U32)height;
			stDefImageSize.u32Height = (CVI_U32)width;
		}
	#endif
		SIZE_S stSizeIn, stSizeOut;
		PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)format_in;
		PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)format_out;

		CVI_VO_Get_Panel_Status(0, ch, &panel_init);
		if (panel_init) {
			CVI_VO_GetPubAttr(0, &stVoPubAttr);
			printf("Panel w=%d, h=%d.\n",\
					stVoPubAttr.stSyncInfo.u16Hact, stVoPubAttr.stSyncInfo.u16Vact);
			stDefDispRect.u32Width = stVoPubAttr.stSyncInfo.u16Hact;
			stDefDispRect.u32Height = stVoPubAttr.stSyncInfo.u16Vact;
			stDefImageSize.u32Width = stVoPubAttr.stSyncInfo.u16Hact;
			stDefImageSize.u32Height = stVoPubAttr.stSyncInfo.u16Vact;
		}
		s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			printf("SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
			return -1;
		}

		stVoConfig.VoDev	 = 0;
		stVoConfig.stVoPubAttr.enIntfType  = VO_INTF_MIPI;
		stVoConfig.stVoPubAttr.enIntfSync  = VO_OUTPUT_720x1280_60;
		stVoConfig.stDispRect	 = stDefDispRect;
		stVoConfig.stImageSize	 = stDefImageSize;
		stVoConfig.enPixFormat	 = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
		stVoConfig.enVoMode	 = VO_MODE_1MUX;
		s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			printf("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
			return -1;
		}

		memcpy(&priv.vo_video_cfg[ch], &stVoConfig, sizeof(SAMPLE_VO_CONFIG_S));

		switch (priv.vo_rotate) {
			case 0:break;
			case 90:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_90);
				break;
			case 180:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_180);
				break;
			case 270:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_270);
				break;
			default:
				break;
		}

		stSizeIn.u32Width   = width;
		stSizeIn.u32Height  = height;
		stSizeOut.u32Width  = width;
		stSizeOut.u32Height = height;
		priv.vo_vpss_in_format[ch] = format_in;
		priv.vo_vpss_in_size[ch].u32Width = stSizeIn.u32Width;
		priv.vo_vpss_in_size[ch].u32Height = stSizeIn.u32Height;
		priv.vo_vpss_out_size[ch].u32Width = stSizeOut.u32Width;
		priv.vo_vpss_out_size[ch].u32Height = stSizeOut.u32Height;
		priv.vo_vpss_fit[ch] = fit;
#if 1
		s32Ret = _mmf_vo_vpss_deinit();
		if (s32Ret != CVI_SUCCESS) {
			printf("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_stop_vo;
		}

		s32Ret = _mmf_vo_vpss_init(stSizeIn.u32Width, stSizeIn.u32Height, formatIn);
		if (s32Ret != CVI_SUCCESS) {
			printf("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_stop_vo;
		}

		s32Ret = _mmf_vpss_chn_deinit(priv.vo_vpss, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		s32Ret = _mmf_vpss_chn_init(priv.vo_vpss, ch, stSizeOut.u32Width, stSizeOut.u32Height, formatOut, fps, depth, mirror, flip, fit);
		if (s32Ret != CVI_SUCCESS) {
			printf("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VO(priv.vo_vpss, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_deinit_vpss_chn;
		}

		// if (priv.vo_video_pre_frame[ch]) {
		// 	_mmf_free_frame(priv.vo_video_pre_frame[ch]);
		// 	priv.vo_video_pre_frame[ch] = NULL;
		// 	priv.vo_video_pre_frame_width[ch] = -1;
		// 	priv.vo_video_pre_frame_height[ch] = -1;
		// 	priv.vo_video_pre_frame_format[ch] = -1;
		// }

		priv.vo_video_pre_frame_width[ch] = width;
		priv.vo_video_pre_frame_height[ch] = height;
		priv.vo_video_pre_frame_format[ch] = format_in;
		// priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format_in);
		// if (!priv.vo_video_pre_frame[ch]) {
		// 	printf("Alloc frame failed!\r\n");
		// 	goto error_and_unbind;
		// }

		priv.vo_video_chn_is_inited[ch] = true;

		return s32Ret;
// error_and_unbind:
// 		s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch);
// 		if (s32Ret != CVI_SUCCESS) {
// 			printf("vo unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
// 		}
error_and_deinit_vpss:
		s32Ret = _mmf_vo_vpss_deinit();
		if (s32Ret != CVI_SUCCESS) {
			printf("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_deinit_vpss_chn:
		s32Ret = _mmf_vpss_chn_deinit(priv.vo_vpss, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_stop_vo:
		s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			printf("vo stop failed. s32Ret: 0x%x !\n", s32Ret);
		}
		return -1;
#else
		priv.vo_vpss = 1;
		s32Ret = _mmf_vpss_init(priv.vo_vpss, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
		if (s32Ret != CVI_SUCCESS) {
			printf("vpss init failed. s32Ret: 0x%x. try again..\r\n", s32Ret);
			s32Ret = _mmf_vpss_deinit(priv.vo_vpss, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
				goto error_and_stop_vo;
			}

			s32Ret = _mmf_vpss_init(priv.vo_vpss, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
			if (s32Ret != CVI_SUCCESS) {
				printf("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
				goto error_and_stop_vo;
			}
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VO(priv.vo_vpss, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		if (priv.vo_video_pre_frame[ch]) {
			_mmf_free_frame(priv.vo_video_pre_frame[ch]);
			priv.vo_video_pre_frame[ch] = NULL;
			priv.vo_video_pre_frame_width[ch] = -1;
			priv.vo_video_pre_frame_height[ch] = -1;
			priv.vo_video_pre_frame_format[ch] = -1;
		}

		priv.vo_video_pre_frame_width[ch] = width;
		priv.vo_video_pre_frame_height[ch] = height;
		priv.vo_video_pre_frame_format[ch] = format_in;
		priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format_in);
		if (!priv.vo_video_pre_frame[ch]) {
			printf("Alloc frame failed!\r\n");
			goto error_and_unbind;
		}

		priv.vo_video_chn_is_inited[ch] = true;

		return s32Ret;
error_and_stop_vo:
		s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			printf("vo stop failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_unbind:
		s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vo unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_deinit_vpss:
		s32Ret = _mmf_vpss_deinit(priv.vo_vpss, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
		}
		return -1;
#endif
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (fit != 0) {
			printf("region support fit = 0 only!\r\n");
			return false;
		}

		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}

		if (priv.vo_osd_chn_is_inited[ch]) {
			printf("vo osd ch %d already open\n", ch);
			return -1;
		}

		if (format_in != PIXEL_FORMAT_ARGB_8888) {
			printf("only support ARGB format.\n");
			return -1;
		}

		if (0 != mmf_add_region_channel(ch, OVERLAY_RGN, CVI_ID_VPSS, priv.vo_vpss, ch, 0, 0, width, height, format_in)) {
			printf("mmf_add_region_channel failed!\r\n");
			return -1;
		}

		priv.vo_osd_chn_is_inited[ch] = true;

		return 0;
	} else {
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return -1;
}

int mmf_add_vo_channel_with_fit(int layer, int ch, int width, int height, int format, int fit)
{
	return _mmf_add_vo_channel(layer, ch, width, height, format, PIXEL_FORMAT_NV21, 30, 0, !g_priv.vo_video_hmirror[ch], !g_priv.vo_video_vflip[ch], fit);
}

int mmf_add_vo_channel(int layer, int ch, int width, int height, int format)
{
	return mmf_add_vo_channel_with_fit(layer, ch, width, height, format, priv.vo_vpss_fit[ch]);
}

int mmf_del_vo_channel(int layer, int ch) {
	if (layer == MMF_VO_VIDEO_LAYER) {
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return CVI_FALSE;
		}

		if (priv.vo_video_chn_is_inited[ch] == false) {
			return CVI_SUCCESS;
		}

		if (CVI_SUCCESS != SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch)) {
			printf("vo unbind vpss failed.!\n");
		}
#if 0
		if (0 != _mmf_vpss_deinit(priv.vo_vpss, ch)) {
			printf("vpss deinit failed.!\n");
		}
#else
		if (0 != _mmf_vpss_chn_deinit(priv.vo_vpss, ch)) {
			printf("_mmf_vpss_chn_deinit failed!\n");
		}

		if (0 != _mmf_vo_vpss_deinit()) {
			printf("_mmf_vpss_deinit_new failed!\n");
		}
#endif
		if (CVI_SUCCESS != SAMPLE_COMM_VO_StopVO(&priv.vo_video_cfg[ch])) {
			printf("SAMPLE_COMM_VO_StopVO failed with %#x\n", CVI_FAILURE);
			return CVI_FAILURE;
		}

		// if (priv.vo_video_pre_frame[ch]) {
		// 	_mmf_free_frame(priv.vo_video_pre_frame[ch]);
		// 	priv.vo_video_pre_frame_width[ch] = -1;
		// 	priv.vo_video_pre_frame_height[ch] = -1;
		// 	priv.vo_video_pre_frame_format[ch] = -1;
		// 	priv.vo_video_pre_frame[ch] = NULL;
		// }

		priv.vo_video_chn_is_inited[ch] = false;
		return CVI_SUCCESS;
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return CVI_FALSE;
		}

		if (priv.vo_osd_chn_is_inited[ch] == false) {
			return CVI_SUCCESS;
		}

		if (0 != mmf_del_region_channel(ch)) {
			printf("mmf_del_region_channel failed!\r\n");
		}

		priv.vo_osd_chn_is_inited[ch] = false;
		return CVI_SUCCESS;
	} else {
		printf("invalid layer %d\n", layer);
		return CVI_FAILURE;
	}

	return CVI_FAILURE;
}

int mmf_del_vo_channel_all(int layer) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		for (int i = 0; i < MMF_VO_VIDEO_MAX_CHN; i++) {
			if (priv.vo_video_chn_is_inited[i] == true) {
				s32Ret = mmf_del_vo_channel(layer, i);
				if (s32Ret != CVI_SUCCESS) {
					printf("mmf_del_vo_channel failed with %#x\n", s32Ret);
					// return s32Ret; // continue to del other chn
				}
			}
		}
		break;
	case MMF_VO_OSD_LAYER:
		for (int i = 0; i < MMF_VO_OSD_MAX_CHN; i++) {
			if (priv.vo_osd_chn_is_inited[i] == true) {
				s32Ret = mmf_del_vo_channel(layer, i);
				if (s32Ret != CVI_SUCCESS) {
					printf("mmf_del_vo_channel failed with %#x\n", s32Ret);
					// return s32Ret; // continue to del other chn
				}
			}
		}
		break;
	default:
		printf("invalid layer %d\n", layer);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

bool mmf_vo_channel_is_open(int layer, int ch) {

	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}
		return priv.vo_video_chn_is_inited[ch];
	case MMF_VO_OSD_LAYER:
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}
		return priv.vo_osd_chn_is_inited[ch];
	default:
		printf("invalid layer %d\n", layer);
		return false;
	}

	return false;
}

int mmf_vo_frame_push2(int layer, int ch, int fit, void *frame_info) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *)frame_info;

	if (!frame) {
		return -1;
	}

	int width = frame->stVFrame.u32Width;
	int height = frame->stVFrame.u32Height;
	int format = frame->stVFrame.enPixelFormat;

	if (layer == MMF_VO_VIDEO_LAYER) {
		if (fit != priv.vo_vpss_fit[ch]
		|| width != (int)priv.vo_vpss_in_size[ch].u32Width
		|| height != (int)priv.vo_vpss_in_size[ch].u32Height
		|| format != (int)priv.vo_vpss_in_format[ch]) {
			priv.vo_vpss_in_format[ch] = format;
			priv.vo_vpss_in_size[ch].u32Width = width;
			priv.vo_vpss_in_size[ch].u32Height = height;
			priv.vo_vpss_fit[ch] = fit;
			int width_out = priv.vo_vpss_out_size[ch].u32Width;
			int height_out = priv.vo_vpss_out_size[ch].u32Height;
			int fps_out = priv.vo_vpss_out_fps[ch];
			int depth_out = priv.vo_vpss_out_depth[ch];
			int mirror_out = !g_priv.vo_video_hmirror[ch];
			int flip_out = !g_priv.vo_video_vflip[ch];
			s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vo_vpss_deinit();
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vo_vpss_init(width, height, (PIXEL_FORMAT_E)format);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_deinit(priv.vo_vpss, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_init(priv.vo_vpss, ch, width_out, height_out, PIXEL_FORMAT_NV21, fps_out, depth_out, mirror_out, flip_out, fit);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = SAMPLE_COMM_VPSS_Bind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}
		}

		// if (priv.vo_video_cfg[ch].enPixFormat != (PIXEL_FORMAT_E)format) {
		// 	printf("vo ch %d format not match. input:%d need:%d\n", ch, format, priv.vo_video_cfg[ch].enPixFormat);
		// 	return CVI_FAILURE;
		// }

		// mmf_vo_frame_push
		s32Ret = CVI_VPSS_SendFrame(priv.vo_vpss, frame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VO_SendFrame failed >< with %#x\n", s32Ret);
			return s32Ret;
		}
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return -1;
		}

		if (priv.vo_osd_chn_is_inited[ch] == false) {
			printf("vo osd ch %d not open\n", ch);
			return -1;
		}

		if (format != PIXEL_FORMAT_ARGB_8888) {
			printf("only support ARGB format.\n");
			return -1;
		}

		if (0 != mmf_region_frame_push2(ch, frame)) {
			printf("mmf_region_flush failed!\r\n");
			return -1;
		}
	} else {
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return CVI_SUCCESS;
}

// flush vo
int mmf_vo_frame_push_with_fit(int layer, int ch, void *data, int len, int width, int height, int format, int fit) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	UNUSED(len);
	UNUSED(layer);
	UNUSED(cvi_rgb2nv21);

	if (layer == MMF_VO_VIDEO_LAYER) {
		if (fit != priv.vo_vpss_fit[ch]
		|| width != (int)priv.vo_vpss_in_size[ch].u32Width
		|| height != (int)priv.vo_vpss_in_size[ch].u32Height
		|| format != (int)priv.vo_vpss_in_format[ch]) {
#if 1
			priv.vo_vpss_in_format[ch] = format;
			priv.vo_vpss_in_size[ch].u32Width = width;
			priv.vo_vpss_in_size[ch].u32Height = height;
			priv.vo_vpss_fit[ch] = fit;
			int width_out = priv.vo_vpss_out_size[ch].u32Width;
			int height_out = priv.vo_vpss_out_size[ch].u32Height;
			int fps_out = priv.vo_vpss_out_fps[ch];
			int depth_out = priv.vo_vpss_out_depth[ch];
			int mirror_out = !g_priv.vo_video_hmirror[ch];
			int flip_out = !g_priv.vo_video_vflip[ch];
			s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vo_vpss_deinit();
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vo_vpss_init(width, height, (PIXEL_FORMAT_E)format);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_deinit(priv.vo_vpss, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_init(priv.vo_vpss, ch, width_out, height_out, PIXEL_FORMAT_NV21, fps_out, depth_out, mirror_out, flip_out, fit);
			if (s32Ret != CVI_SUCCESS) {
				printf("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = SAMPLE_COMM_VPSS_Bind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}
#else
			s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			SIZE_S stSizeIn, stSizeOut;
			int fps = 30;
			int depth = 0;
			PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)format;
			PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
			stSizeIn.u32Width   = width;
			stSizeIn.u32Height  = height;
			stSizeOut.u32Width  = priv.vo_vpss_out_size[ch].u32Width;
			stSizeOut.u32Height = priv.vo_vpss_out_size[ch].u32Height;
			priv.vo_vpss_in_size[ch].u32Width = stSizeIn.u32Width;
			priv.vo_vpss_in_size[ch].u32Height = stSizeIn.u32Height;
			priv.vo_vpss_fit[ch] = fit;
			_mmf_vpss_deinit(priv.vo_vpss, ch);
			bool mirror = !g_priv.vo_video_hmirror[ch];
			bool flip = !g_priv.vo_video_vflip[ch];
			int fit = priv.vo_vpss_fit[ch];
			s32Ret = _mmf_vpss_init(priv.vo_vpss, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
			if (s32Ret != CVI_SUCCESS) {
				printf("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			// printf("vpss vo reinit.\r\n");
			// printf("stSizeIn.u32Width: %d, stSizeIn.u32Height: %d, stSizeOut.u32Width: %d, stSizeOut.u32Height: %d formatOut:%d\n",
			// 						stSizeIn.u32Width, stSizeIn.u32Height, stSizeOut.u32Width, stSizeOut.u32Height, formatOut);
			s32Ret = SAMPLE_COMM_VPSS_Bind_VO(priv.vo_vpss, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				_mmf_vpss_deinit(priv.vo_vpss, ch);
				return -1;
			}
#endif
		}

#if 0
		if (!priv.vo_video_pre_frame[ch]
			|| priv.vo_video_pre_frame_width[ch] != width
			|| priv.vo_video_pre_frame_height[ch] != height
			|| priv.vo_video_pre_frame_format[ch] != format) {
			if (priv.vo_video_pre_frame[ch]) {
				_mmf_free_frame(priv.vo_video_pre_frame[ch]);
				priv.vo_video_pre_frame[ch] = NULL;
			}
			priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format);
			if (!priv.vo_video_pre_frame[ch]) {
				printf("Alloc frame failed!\r\n");
				return -1;
			}
			priv.vo_video_pre_frame_width[ch] = width;
			priv.vo_video_pre_frame_height[ch] = height;
			priv.vo_video_pre_frame_format[ch] = format;
		}

		VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *)priv.vo_video_pre_frame[ch];
		switch (format) {
		case PIXEL_FORMAT_RGB_888:
			// if (fit == 2) {	// crop image and keep aspect ratio
			// 	CVI_FLOAT corp_scale_w = (CVI_FLOAT)priv.vo_vpss_in_size[ch].u32Width / priv.vo_vpss_out_size[ch].u32Width;
			// 	CVI_FLOAT corp_scale_h = (CVI_FLOAT)priv.vo_vpss_in_size[ch].u32Height / priv.vo_vpss_out_size[ch].u32Height;
			// 	CVI_U32 crop_w = corp_scale_w < corp_scale_h ? priv.vo_vpss_out_size[ch].u32Width * corp_scale_w: priv.vo_vpss_out_size[ch].u32Width * corp_scale_h;
			// 	CVI_U32 crop_h = corp_scale_w < corp_scale_h ? priv.vo_vpss_out_size[ch].u32Height * corp_scale_w: priv.vo_vpss_out_size[ch].u32Height * corp_scale_h;
			// 	if (corp_scale_h < 0 || corp_scale_w < 0) {
			// 		printf("crop scale error. corp_scale_w: %d, corp_scale_h: %d\n", corp_scale_w, corp_scale_h);
			// 		return -1;
			// 	}
			// } else
			{
				if (frame->stVFrame.u32Stride[0] != (CVI_U32)width * 3) {
					for (int h = 0; h < height; h++) {
						memcpy((uint8_t *)frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Stride[0] * h, ((uint8_t *)data) + width * h * 3, width * 3);
					}
				} else {
					memcpy(frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3);
				}
				CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0],
									frame->stVFrame.pu8VirAddr[0],
									width * height * 3);
			}
		break;
		case PIXEL_FORMAT_NV21:
			if (frame->stVFrame.u32Stride[0] != (CVI_U32)width) {
				for (int h = 0; h < height * 3 / 2; h ++) {
					memcpy((uint8_t *)frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + width * h, width);
				}
			} else {
				memcpy(frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3 / 2);
			}

			CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0],
								frame->stVFrame.pu8VirAddr[0],
								width * height * 3 / 2);
		break;
		default:
			printf("format not support\n");
			return CVI_FAILURE;
		}

		s32Ret = CVI_VPSS_SendFrame(priv.vo_vpss, frame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VO_SendFrame failed, ret:%#x\n", s32Ret);
			return s32Ret;
		}
#else
		VIDEO_FRAME_INFO_S stVideoFrame;
		VB_CAL_CONFIG_S stVbCalConfig;
		UNUSED(len);
		COMMON_GetPicBufferConfig(width, height, (PIXEL_FORMAT_E)format, DATA_BITWIDTH_8
			, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

		// if (priv.vo_video_cfg[ch].enPixFormat != (PIXEL_FORMAT_E)format) {
		// 	printf("vo ch %d format not match. input:%d need:%d\n", ch, format, priv.vo_video_cfg[ch].enPixFormat);
		// 	return CVI_FAILURE;
		// }

		memset(&stVideoFrame, 0, sizeof(stVideoFrame));

		if (_mmf_init_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format, &stVideoFrame, &stVbCalConfig) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}

		switch (format) {
		case PIXEL_FORMAT_RGB_888:
			if (stVideoFrame.stVFrame.u32Stride[0] != (CVI_U32)width * 3) {
				for (int h = 0; h < height; h++) {
					memcpy((uint8_t *)stVideoFrame.stVFrame.pu8VirAddr[0] + stVideoFrame.stVFrame.u32Stride[0] * h, ((uint8_t *)data) + width * h * 3, width * 3);
				}
			} else {
				memcpy(stVideoFrame.stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3);
			}
			CVI_SYS_IonFlushCache(stVideoFrame.stVFrame.u64PhyAddr[0],
								stVideoFrame.stVFrame.pu8VirAddr[0],
								width * height * 3);
		break;
		case PIXEL_FORMAT_NV21:
			if (stVideoFrame.stVFrame.u32Stride[0] != (CVI_U32)width) {
				for (int h = 0; h < height * 3 / 2; h ++) {
					memcpy((uint8_t *)stVideoFrame.stVFrame.pu8VirAddr[0] + stVideoFrame.stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + width * h, width);
				}
			} else {
				memcpy(stVideoFrame.stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3 / 2);
			}

			CVI_SYS_IonFlushCache(stVideoFrame.stVFrame.u64PhyAddr[0],
								stVideoFrame.stVFrame.pu8VirAddr[0],
								width * height * 3 / 2);
		break;
		default:
			printf("format not support\n");
			_mmf_deinit_frame(&stVideoFrame);
			return CVI_FAILURE;
		}

	#if 0
		VPSS_GRP_ATTR_S GrpAttr;
		CVI_VPSS_GetGrpAttr(priv.vo_vpss, &GrpAttr);
		mmf_dump_grpattr(&GrpAttr);

		VPSS_CHN_ATTR_S ChnAttr;
		CVI_VPSS_GetChnAttr(priv.vo_vpss, ch, &ChnAttr);
		mmf_dump_chnattr(&ChnAttr);

		mmf_dump_frame(&stVideoFrame);
	#endif
		// UNUSED(layer);
		// mmf_vo_frame_push
		s32Ret = CVI_VPSS_SendFrame(priv.vo_vpss, &stVideoFrame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VO_SendFrame failed >< with %#x\n", s32Ret);
			_mmf_deinit_frame(&stVideoFrame);
			return s32Ret;
		}

		_mmf_deinit_frame(&stVideoFrame);
#endif
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return -1;
		}

		if (priv.vo_osd_chn_is_inited[ch] == false) {
			printf("vo osd ch %d not open\n", ch);
			return -1;
		}

		if (format != PIXEL_FORMAT_ARGB_8888) {
			printf("only support ARGB format.\n");
			return -1;
		}

		if (0 != mmf_region_frame_push(ch, data, len)) {
			printf("mmf_region_flush failed!\r\n");
			return -1;
		}
	} else {
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return CVI_SUCCESS;
}

int mmf_vo_frame_push(int layer, int ch, void *data, int len, int width, int height, int format) {
	return mmf_vo_frame_push_with_fit(layer, ch, data, len, width, height, format, priv.vo_vpss_fit[ch]);
}

static CVI_S32 _mmf_region_attach_to_channel(CVI_S32 ch, int x, int y, RGN_TYPE_E enType, MMF_CHN_S *pstChn)
{
#define OverlayMinHandle 0
#define OverlayExMinHandle 20
#define CoverMinHandle 40
#define CoverExMinHandle 60
#define MosaicMinHandle 80
#define OdecHandle 100
	CVI_S32 s32Ret = CVI_SUCCESS;
	RGN_CHN_ATTR_S stChnAttr;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("HandleId is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (enType != OVERLAY_RGN) {
		printf("enType is illegal %d!\n", enType);
		return CVI_FAILURE;
	}

	if (pstChn == CVI_NULL) {
		printf("pstChn is NULL !\n");
		return CVI_FAILURE;
	}
	memset(&stChnAttr, 0, sizeof(stChnAttr));

	/*set the chn config*/
	stChnAttr.bShow = CVI_TRUE;
	switch (enType) {
	case OVERLAY_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = OVERLAY_RGN;
		stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = CVI_FALSE;
		break;
	case OVERLAYEX_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = OVERLAYEX_RGN;
		stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.bInvColEn = CVI_FALSE;
		break;
	case COVER_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = COVER_RGN;
		stChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;

		stChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 100;
		stChnAttr.unChnAttr.stCoverChn.stRect.u32Width = 100;

		stChnAttr.unChnAttr.stCoverChn.u32Color = 0x0000ffff;

		stChnAttr.unChnAttr.stCoverChn.enCoordinate = RGN_ABS_COOR;
		break;
	case COVEREX_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = COVEREX_RGN;
		stChnAttr.unChnAttr.stCoverExChn.enCoverType = AREA_RECT;

		stChnAttr.unChnAttr.stCoverExChn.stRect.u32Height = 100;
		stChnAttr.unChnAttr.stCoverExChn.stRect.u32Width = 100;

		stChnAttr.unChnAttr.stCoverExChn.u32Color = 0x0000ffff;
		break;
	case MOSAIC_RGN:
		stChnAttr.enType = MOSAIC_RGN;
		stChnAttr.unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_8;
		stChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 96; // 8 pixel align
		stChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 96;
		break;
	default:
		return CVI_FAILURE;
	}

	if (enType == OVERLAY_RGN) {
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = x;
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = y;
		stChnAttr.unChnAttr.stOverlayChn.u32Layer = ch;
	}
	s32Ret = CVI_RGN_AttachToChn(ch, pstChn, &stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_AttachToChn failed!\n");
		return s32Ret;
	}

	return s32Ret;
}

static int _mmf_region_init(int ch, int type, int width, int height, int format)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	RGN_ATTR_S stRegion;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}
	if (type != OVERLAY_RGN) {
		printf("enType is illegal %d!\n", type);
		return CVI_FAILURE;
	}

	if (priv.rgn_is_init[ch]) {
		return 0;
	}

	stRegion.enType = (RGN_TYPE_E)type;
	stRegion.unAttr.stOverlay.enPixelFormat = (PIXEL_FORMAT_E)format;
	stRegion.unAttr.stOverlay.stSize.u32Height = height;
	stRegion.unAttr.stOverlay.stSize.u32Width = width;
	stRegion.unAttr.stOverlay.u32BgColor = 0x00000000; // ARGB1555 transparent
	stRegion.unAttr.stOverlay.u32CanvasNum = 1;
	stRegion.unAttr.stOverlay.stCompressInfo.enOSDCompressMode = OSD_COMPRESS_MODE_NONE;
	s32Ret = CVI_RGN_Create(ch, &stRegion);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_Create failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.rgn_type[ch] = (RGN_TYPE_E)type;
	priv.rgn_id[ch] = ch;
	priv.rgn_is_init[ch] = true;

	return s32Ret;
}


static int _mmf_region_bind(int ch, int mod_id, int dev_id, int chn_id, int x, int y)
{
	CVI_S32 s32Ret;

	MMF_CHN_S stChn = {
		.enModId = (MOD_ID_E)mod_id,
		.s32DevId = (CVI_S32)dev_id,
		.s32ChnId = (CVI_S32)chn_id
	};

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (priv.rgn_is_bind[ch]) {
		return 0;
	}

	RGN_TYPE_E type = (RGN_TYPE_E)priv.rgn_type[ch];
	s32Ret = _mmf_region_attach_to_channel(ch, x, y, type, &stChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("_mmf_region_attach_to_channel failed!\n");
		return s32Ret;
	}

	priv.rgn_mod_id[ch] = (MOD_ID_E)mod_id;
	priv.rgn_dev_id[ch] = (CVI_S32)dev_id;
	priv.rgn_chn_id[ch] = (CVI_S32)chn_id;
	priv.rgn_is_bind[ch] = true;

	return s32Ret;
}

static int _mmf_region_unbind(int ch)
{
	CVI_S32 s32Ret;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	MMF_CHN_S stChn = {
		.enModId = (MOD_ID_E)priv.rgn_mod_id[ch],
		.s32DevId = (CVI_S32)priv.rgn_dev_id[ch],
		.s32ChnId = (CVI_S32)priv.rgn_chn_id[ch]
	};

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_DetachFromChn(ch, &stChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_DetachFromChn %d failed!\n", ch);
	}

	priv.rgn_is_bind[ch] = false;

	return s32Ret;
}

static int _mmf_region_deinit(int ch)
{
	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (priv.rgn_is_bind[ch]) {
		_mmf_region_unbind(ch);
	}

	CVI_S32 s32Ret = CVI_RGN_Destroy(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_Destroy failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.rgn_is_init[ch] = false;

	return s32Ret;
}

int mmf_get_region_unused_channel(void)
{
	for (int i = 0; i < MMF_RGN_MAX_NUM; i++) {
		if (priv.rgn_is_init[i] == false) {
			return i;
		}
	}
	return -1;
}

int mmf_add_region_channel(int ch, int type, int mod_id, int dev_id, int chn_id, int x, int y, int width, int height, int format)
{
	if (0 != _mmf_region_init(ch, type, width, height, format)) {
		return -1;
	}

	if (0 != _mmf_region_bind(ch, mod_id, dev_id, chn_id, x, y)) {
		_mmf_region_deinit(ch);
		return -1;
	}

	return 0;
}

int mmf_del_region_channel(int ch)
{
	if (0 != _mmf_region_deinit(ch)) {
		return -1;
	}

	return 0;
}

int mmf_del_region_channel_all(void)
{
	for (int ch = 0; ch < MMF_RGN_MAX_NUM; ch ++) {
		_mmf_region_deinit(ch);
	}

	return 0;
}

int mmf_region_get_canvas(int ch, void **data, int *width, int *height, int *format)
{
	CVI_S32 s32Ret;
	RGN_CANVAS_INFO_S stCanvasInfo;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_GetCanvasInfo(ch, &stCanvasInfo);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	if (data) *data = stCanvasInfo.pu8VirtAddr;
	if (width) *width = (int)stCanvasInfo.stSize.u32Width;
	if (height) *height = (int)stCanvasInfo.stSize.u32Height;
	if (format) *format = (int)stCanvasInfo.enPixelFormat;

	return s32Ret;
}

int mmf_region_update_canvas(int ch)
{
	CVI_S32 s32Ret;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_UpdateCanvas(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

static int mmf_region_frame_push2(int ch, void *frame_info)
{
	void *data;
	int len = 0;
	VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *)frame_info;

	if (!frame) {
		return CVI_FAILURE;
	}

	data = frame->stVFrame.pu8VirAddr[0];

	if (frame->stVFrame.pu8VirAddr[2] == (CVI_U8 *)frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1]) {
		len += frame->stVFrame.u32Length[2];
	}
	if (frame->stVFrame.pu8VirAddr[1] == (CVI_U8 *)frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0]) {
		len += frame->stVFrame.u32Length[1];
	}
	if (frame->stVFrame.pu8VirAddr[0]) {
		len += frame->stVFrame.u32Length[0];
	}

	if (!data || !len) {
		return CVI_FAILURE;
	}

	return mmf_region_frame_push(ch, data, len);
}

int mmf_region_frame_push(int ch, void *data, int len)
{
	CVI_S32 s32Ret;
	RGN_CANVAS_INFO_S stCanvasInfo;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		printf("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_GetCanvasInfo(ch, &stCanvasInfo);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	if (stCanvasInfo.enPixelFormat == PIXEL_FORMAT_ARGB_8888) {
		if (!data || (CVI_U32)len != stCanvasInfo.stSize.u32Width * stCanvasInfo.stSize.u32Height * 4) {
			printf("Param is error!\r\n");
			return CVI_FAILURE;
		}
		memcpy(stCanvasInfo.pu8VirtAddr, data, len);
	} else {
		printf("Not support format!\r\n");
		return CVI_FAILURE;
	}

	s32Ret = CVI_RGN_UpdateCanvas(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

static int mmf_invert_codec_to_maix(PAYLOAD_TYPE_E mmf_codec) {
	switch (mmf_codec) {
		case PT_JPEG:
			return 0;
		case PT_H265:
			return 1;
		case PT_H264:
			return 2;
		case PT_MJPEG:
			return 4;
		default:
			return 0xFF;
	}
}

static PAYLOAD_TYPE_E mmf_invert_codec_to_mmf(int maix_codec) {
	switch (maix_codec) {
		case 0:
			return PT_JPEG;
		case 1:
			return PT_H265;
		case 2:
			return PT_H264;
		case 3:
			return PT_JPEG;
		case 4:
			return PT_MJPEG;
		default:
			return PT_BUTT;
	}
}

static CVI_S32 _mmf_venc_vpss_init(int ch, mmf_venc_cfg_t *cfg)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	priv.enc_chn_vpss[ch] = VPSS_INVALID_GRP;
	switch (cfg->fmt) {
	case PIXEL_FORMAT_RGB_888:
	{
		VPSS_GRP out_grp = CVI_VPSS_GetAvailableGrp();

		s32Ret = _mmf_vpss_init(out_grp, ch, (SIZE_S){(CVI_U32)cfg->w, (CVI_U32)cfg->h}, (SIZE_S){(CVI_U32)cfg->w, (CVI_U32)cfg->h}, (PIXEL_FORMAT_E)cfg->fmt, PIXEL_FORMAT_YUV_PLANAR_420, cfg->output_fps, 0, CVI_FALSE, CVI_FALSE, 0);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS init failed with %d\n", s32Ret);
			return s32Ret;
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(out_grp, ch, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS bind VENC failed with %#x\n", s32Ret);
			_mmf_vpss_deinit(out_grp, ch);
			return s32Ret;
		}

		priv.enc_chn_vpss[ch] = out_grp;
		break;
	}
	case PIXEL_FORMAT_NV21:
		break;
	default:
		printf("unknown format!\n");
		s32Ret = CVI_FAILURE;
		return s32Ret;
	}

	return s32Ret;
}

static CVI_S32 _mmf_venc_vpss_deinit(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (priv.enc_chn_vpss[ch] != VPSS_INVALID_GRP) {
		VPSS_GRP out_grp = priv.enc_chn_vpss[ch];

		s32Ret = SAMPLE_COMM_VPSS_UnBind_VENC(out_grp, ch, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS unbind VENC failed with %d\n", s32Ret);
		}

		s32Ret = _mmf_vpss_deinit(out_grp, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS deinit failed with %d\n", s32Ret);
		}

		priv.enc_chn_vpss[ch] = VPSS_INVALID_GRP;
	}

	return s32Ret;
}

static int _mmf_enc_jpg_init(int ch, mmf_venc_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (priv.enc_chn_is_init[ch]) {
		return 0;
	}

	if ((int8_t)cfg->jpg_quality < 50) {
		printf("quality range is (50, 100]\n");
		return -1;
	}

	if ((cfg->fmt == PIXEL_FORMAT_RGB_888 && cfg->w * cfg->h * 3 > 640 * 480 * 3)
		|| (cfg->fmt == PIXEL_FORMAT_NV21 && cfg->w * cfg->h * 3 / 2 > 2560 * 1440 * 3 / 2)) {
		printf("image size is too large, for NV21, maximum resolution 2560x1440, for RGB888, maximum resolution 640x480!\n");
		return -1;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;

	VENC_CHN_ATTR_S stVencChnAttr;
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	stVencChnAttr.stVencAttr.enType = PT_JPEG;
	stVencChnAttr.stVencAttr.u32MaxPicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32MaxPicHeight = cfg->h;
	stVencChnAttr.stVencAttr.u32PicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32PicHeight = cfg->h;
	stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_FALSE;
	stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_FALSE;
	stVencChnAttr.stVencAttr.bByFrame = 1;
	stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VENC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ResetChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VENC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_DestoryChn [%d] failed with %#x\n", ch, s32Ret);
	}

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	VENC_JPEG_PARAM_S stJpegParam;
	memset(&stJpegParam, 0, sizeof(VENC_JPEG_PARAM_S));
	s32Ret = CVI_VENC_GetJpegParam(ch, &stJpegParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_GetJpegParam failed with %#x\n", s32Ret);
		goto out_chn;
	}
	stJpegParam.u32Qfactor = (cfg->jpg_quality <= 50) ? 51 : (cfg->jpg_quality >= 100) ? 99 : cfg->jpg_quality;
	s32Ret = CVI_VENC_SetJpegParam(ch, &stJpegParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_SetJpegParam failed with %#x\n", s32Ret);
		goto out_chn;
	}

	s32Ret = _mmf_venc_vpss_init(ch, cfg);
	if (s32Ret != CVI_SUCCESS) {
		goto out_chn;
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StartRecvFrame failed with %#x\n", s32Ret);
		s32Ret = CVI_FAILURE;
		goto out_vpss;
	}

	if (priv.enc_chn_frame[ch]) {
		_mmf_free_frame(priv.enc_chn_frame[ch]);
		priv.enc_chn_frame[ch] = NULL;
	}

	priv.enc_chn_type[ch] = PT_JPEG;
	priv.enc_chn_vb_id[ch] = MMF_VB_ENC_JPEG_ID;

	memcpy(&priv.enc_chn_cfg[ch], cfg, sizeof(priv.enc_chn_cfg[ch]));
	priv.enc_chn_cfg[ch].w = cfg->w;
	priv.enc_chn_cfg[ch].h = cfg->h;
	priv.enc_chn_cfg[ch].fmt = cfg->fmt;
	priv.enc_chn_cfg[ch].jpg_quality = cfg->jpg_quality;
	priv.enc_chn_is_init[ch] = 1;
	priv.enc_chn_running[ch] = 0;

	return s32Ret;

out_vpss:
	_mmf_venc_vpss_deinit(ch);
out_chn:
	CVI_VENC_DestroyChn(ch);
	return s32Ret;
}

int mmf_enc_jpg_init(int ch, int w, int h, int format, int quality)
{
	mmf_venc_cfg_t cfg = {
		.type = 4,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = format,
		.jpg_quality = (uint8_t)quality,
		.gop = 0,  // unused
		.intput_fps = 30,
		.output_fps = 30,
		.bitrate = 0,  // unused
	};

	return _mmf_enc_jpg_init(ch, &cfg);
}

int mmf_enc_jpg_deinit(int ch)
{
	return  mmf_venc_deinit(ch);
}

int mmf_enc_jpg_push_with_quality(int ch, uint8_t *data, int w, int h, int format, int quality)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_is_init[ch]) {
		s32Ret = mmf_enc_jpg_init(ch, w, h, format, quality);
		if (s32Ret != CVI_SUCCESS) {
			printf("mmf_enc_jpg_init failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	return _mmf_venc_push(ch, data, w, h, format, quality);
}

int mmf_enc_jpg_push(int ch, uint8_t *data, int w, int h, int format)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_cfg[ch].jpg_quality) {
		priv.enc_chn_cfg[ch].jpg_quality = 80;
	}

	return mmf_enc_jpg_push_with_quality(ch, data, w,h, format, priv.enc_chn_cfg[ch].jpg_quality);
}

int mmf_enc_jpg_pop(int ch, uint8_t **data, int *size)
{
	CVI_S32 s32Ret = mmf_venc_pop(ch, NULL);
	if (s32Ret != CVI_SUCCESS) {
		return s32Ret;
	}

	if (!priv.enc_chn_stream[ch].pstPack)
		return -1;
	if (data)
		*data = priv.enc_chn_stream[ch].pstPack[0].pu8Addr;
	if (size)
		*size = priv.enc_chn_stream[ch].pstPack[0].u32Len;

	return s32Ret;
}

int mmf_enc_jpg_free(int ch)
{
	return mmf_venc_free(ch);
}

static int _mmf_enc_h265_init(int ch, mmf_venc_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (priv.enc_chn_is_init[ch]) {
		return 0;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;

	VENC_CHN_ATTR_S stVencChnAttr;
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	stVencChnAttr.stVencAttr.enType = PT_H265;
	stVencChnAttr.stVencAttr.u32MaxPicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32MaxPicHeight = cfg->h;
	stVencChnAttr.stVencAttr.u32BufSize = 1024 * 1024;	// 1024Kb
	stVencChnAttr.stVencAttr.bByFrame = 1;
	stVencChnAttr.stVencAttr.u32PicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32PicHeight = cfg->h;
	stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_TRUE;
	stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_TRUE;
	stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;

	stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
	stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = cfg->gop;
	stVencChnAttr.stRcAttr.stH265Cbr.u32StatTime = 2;
	stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRate = cfg->intput_fps;
	stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate = cfg->output_fps;
	stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = cfg->bitrate;
	stVencChnAttr.stRcAttr.stH265Cbr.bVariFpsEn = 0;

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %d\n", ch, s32Ret);
		return s32Ret;
	}

	s32Ret = _mmf_venc_vpss_init(ch, cfg);
	if (s32Ret != CVI_SUCCESS) {
		goto out_chn;
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StartRecvFrame failed with %d\n", s32Ret);
		goto out_vpss;
	}

	{
		VENC_H265_TRANS_S h265Trans;
		memset(&h265Trans, 0, sizeof(h265Trans));
		s32Ret = CVI_VENC_GetH265Trans(ch, &h265Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH265Trans failed with %d\n", s32Ret);
			goto out_vpss;
		}
		h265Trans.cb_qp_offset = 0;
		h265Trans.cr_qp_offset = 0;
		s32Ret = CVI_VENC_SetH265Trans(ch, &h265Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH265Trans failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	{
		VENC_H265_VUI_S h265Vui;
		memset(&h265Vui, 0, sizeof(h265Vui));
		s32Ret = CVI_VENC_GetH265Vui(ch, &h265Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH265Vui failed with %d\n", s32Ret);
			goto out_vpss;
		}

		h265Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
		h265Vui.stVuiAspectRatio.aspect_ratio_idc = 1;
		h265Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
		h265Vui.stVuiAspectRatio.overscan_appropriate_flag = 0;
		h265Vui.stVuiAspectRatio.sar_width = 1;
		h265Vui.stVuiAspectRatio.sar_height = 1;
		h265Vui.stVuiTimeInfo.timing_info_present_flag = 1;
		h265Vui.stVuiTimeInfo.num_units_in_tick = 1;
		h265Vui.stVuiTimeInfo.time_scale = 30;
		h265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = 1;
		h265Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
		h265Vui.stVuiVideoSignal.video_format = 5;
		h265Vui.stVuiVideoSignal.video_full_range_flag = 0;
		h265Vui.stVuiVideoSignal.colour_description_present_flag = 0;
		h265Vui.stVuiVideoSignal.colour_primaries = 2;
		h265Vui.stVuiVideoSignal.transfer_characteristics = 2;
		h265Vui.stVuiVideoSignal.matrix_coefficients = 2;
		h265Vui.stVuiBitstreamRestric.bitstream_restriction_flag = 0;

		// _mmf_dump_venc_h265_vui(&h265Vui);

		s32Ret = CVI_VENC_SetH265Vui(ch, &h265Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH265Vui failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	// rate control
	{
		VENC_RC_PARAM_S stRcParam;
		s32Ret = CVI_VENC_GetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetRcParam failed with %d\n", s32Ret);
			goto out_vpss;
		}
		stRcParam.s32FirstFrameStartQp = 35;
		stRcParam.stParamH265Cbr.u32MinIprop = 1;
		stRcParam.stParamH265Cbr.u32MaxIprop = 10;
		stRcParam.stParamH265Cbr.u32MaxQp = 51;
		stRcParam.stParamH265Cbr.u32MinQp = 20;
		stRcParam.stParamH265Cbr.u32MaxIQp = 51;
		stRcParam.stParamH265Cbr.u32MinIQp = 20;

		// _mmf_dump_venc_rc_param(&stRcParam);

		s32Ret = CVI_VENC_SetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetRcParam failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	// frame lost set
	{
		VENC_FRAMELOST_S stFL;
		s32Ret = CVI_VENC_GetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetFrameLostStrategy failed with %d\n", s32Ret);
			goto out_vpss;
		}
		stFL.enFrmLostMode = FRMLOST_PSKIP;

		// _mmf_dump_venc_framelost(&stFL);

		s32Ret = CVI_VENC_SetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetFrameLostStrategy failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	if (priv.enc_chn_frame[ch]) {
		_mmf_free_frame(priv.enc_chn_frame[ch]);
		priv.enc_chn_frame[ch] = NULL;
	}

	priv.enc_chn_type[ch] = PT_H265;
	priv.enc_chn_vb_id[ch] = MMF_VB_ENC_H26X_ID;

	memcpy(&priv.enc_chn_cfg[ch], cfg, sizeof(priv.enc_chn_cfg[ch]));
	priv.enc_chn_cfg[ch].w = cfg->w;
	priv.enc_chn_cfg[ch].h = cfg->h;
	priv.enc_chn_cfg[ch].fmt = cfg->fmt;
	priv.enc_chn_cfg[ch].jpg_quality = cfg->jpg_quality;
	priv.enc_chn_running[ch] = 0;
	priv.enc_chn_is_init[ch] = 1;

	return s32Ret;

out_vpss:
	_mmf_venc_vpss_deinit(ch);
out_chn:
	CVI_VENC_DestroyChn(ch);
	return s32Ret;
}

int mmf_enc_h265_init(int ch, int w, int h)
{
	mmf_venc_cfg_t cfg = {
		.type = 1,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = PIXEL_FORMAT_NV21,
		.jpg_quality = 0,  // unused
		.gop = 50,
		.intput_fps = 30,
		.output_fps = 30,
		.bitrate = 3000,
	};

	return _mmf_enc_h265_init(ch, &cfg);
}

static int _mmf_enc_h264_init(int ch, mmf_venc_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (priv.enc_chn_is_init[ch]) {
		return 0;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;

	VENC_CHN_ATTR_S stVencChnAttr;
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	stVencChnAttr.stVencAttr.enType = PT_H264;
	stVencChnAttr.stVencAttr.u32MaxPicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32MaxPicHeight = cfg->h;
	stVencChnAttr.stVencAttr.u32BufSize = 1024 * 1024;	// 1024Kb
	stVencChnAttr.stVencAttr.bByFrame = 1;
	stVencChnAttr.stVencAttr.u32PicWidth = cfg->w;
	stVencChnAttr.stVencAttr.u32PicHeight = cfg->h;
	stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_TRUE;
	stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_TRUE; // ???
	stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;

	VENC_H264_CBR_S *pstH264Cbr = &stVencChnAttr.stRcAttr.stH264Cbr;

	stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
	pstH264Cbr->u32Gop = cfg->gop;
	pstH264Cbr->u32StatTime = 2;
	pstH264Cbr->u32SrcFrameRate = cfg->intput_fps;
	pstH264Cbr->fr32DstFrameRate = cfg->output_fps;
	pstH264Cbr->u32BitRate = cfg->bitrate;
	pstH264Cbr->bVariFpsEn = 0;

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %d\n", ch, s32Ret);
		return s32Ret;
	}

	VENC_PARAM_MOD_S stModParam;
	CVI_VENC_GetModParam(&stModParam);
	stModParam.stH264eModParam.enH264eVBSource = VB_SOURCE_COMMON;
	stModParam.stH264eModParam.bSingleEsBuf = true;
	stModParam.stH264eModParam.u32SingleEsBufSize = 1024 * 1024;
	stModParam.stH264eModParam.u32UserDataMaxLen = 3072;
	CVI_VENC_SetModParam(&stModParam);

	s32Ret = _mmf_venc_vpss_init(ch, cfg);
	if (s32Ret != CVI_SUCCESS) {
		goto out_chn;
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StartRecvFrame failed with %d\n", s32Ret);
		goto out_vpss;
	}

	{
		VENC_H264_TRANS_S h264Trans;
		memset(&h264Trans, 0, sizeof(h264Trans));
		s32Ret = CVI_VENC_GetH264Trans(ch, &h264Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH264Trans failed with %d\n", s32Ret);
			goto out_vpss;
		}

		//h264Trans.cb_qp_offset = 0;
		//h264Trans.cr_qp_offset = 0;
		h264Trans.chroma_qp_index_offset = 0;

		s32Ret = CVI_VENC_SetH264Trans(ch, &h264Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH264Trans failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	{
		VENC_H264_VUI_S h264Vui;
		memset(&h264Vui, 0, sizeof(h264Vui));
		s32Ret = CVI_VENC_GetH264Vui(ch, &h264Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH264Vui failed with %d\n", s32Ret);
			goto out_vpss;
		}

		h264Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
		h264Vui.stVuiAspectRatio.aspect_ratio_idc = 1;
		h264Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
		h264Vui.stVuiAspectRatio.overscan_appropriate_flag = 0;
		h264Vui.stVuiAspectRatio.sar_width = 1;
		h264Vui.stVuiAspectRatio.sar_height = 1;
		h264Vui.stVuiTimeInfo.timing_info_present_flag = 1;
		h264Vui.stVuiTimeInfo.num_units_in_tick = 1;
		h264Vui.stVuiTimeInfo.time_scale = 30;
		//h264Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = 1;
		h264Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
		h264Vui.stVuiVideoSignal.video_format = 5;
		h264Vui.stVuiVideoSignal.video_full_range_flag = 0;
		h264Vui.stVuiVideoSignal.colour_description_present_flag = 0;
		h264Vui.stVuiVideoSignal.colour_primaries = 2;
		h264Vui.stVuiVideoSignal.transfer_characteristics = 2;
		h264Vui.stVuiVideoSignal.matrix_coefficients = 2;
		h264Vui.stVuiBitstreamRestric.bitstream_restriction_flag = 0;

		// _mmf_dump_venc_h264_vui(&h264Vui);

		s32Ret = CVI_VENC_SetH264Vui(ch, &h264Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH264Vui failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	// rate control
	{
		VENC_RC_PARAM_S stRcParam;
		s32Ret = CVI_VENC_GetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetRcParam failed with %d\n", s32Ret);
			goto out_vpss;
		}
		stRcParam.s32FirstFrameStartQp = 35;
		stRcParam.stParamH264Cbr.u32MinIprop = 1;
		stRcParam.stParamH264Cbr.u32MaxIprop = 10;
		stRcParam.stParamH264Cbr.u32MaxQp = 51;
		stRcParam.stParamH264Cbr.u32MinQp = 20;
		stRcParam.stParamH264Cbr.u32MaxIQp = 51;
		stRcParam.stParamH264Cbr.u32MinIQp = 20;

		// _mmf_dump_venc_rc_param(&stRcParam);

		s32Ret = CVI_VENC_SetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetRcParam failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	// frame lost set
	{
		VENC_FRAMELOST_S stFL;
		s32Ret = CVI_VENC_GetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetFrameLostStrategy failed with %d\n", s32Ret);
			goto out_vpss;
		}
		stFL.enFrmLostMode = FRMLOST_PSKIP;

		// _mmf_dump_venc_framelost(&stFL);

		s32Ret = CVI_VENC_SetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetFrameLostStrategy failed with %d\n", s32Ret);
			goto out_vpss;
		}
	}

	if (priv.enc_chn_frame[ch]) {
		_mmf_free_frame(priv.enc_chn_frame[ch]);
		priv.enc_chn_frame[ch] = NULL;
	}

	priv.enc_chn_type[ch] = PT_H264;
	priv.enc_chn_vb_id[ch] = MMF_VB_ENC_H26X_ID;

	memcpy(&priv.enc_chn_cfg[ch], cfg, sizeof(priv.enc_chn_cfg[ch]));
	priv.enc_chn_cfg[ch].w = cfg->w;
	priv.enc_chn_cfg[ch].h = cfg->h;
	priv.enc_chn_cfg[ch].fmt = cfg->fmt;
	priv.enc_chn_cfg[ch].jpg_quality = cfg->jpg_quality;
	priv.enc_chn_running[ch] = 0;
	priv.enc_chn_is_init[ch] = 1;

	return s32Ret;

out_vpss:
	_mmf_venc_vpss_deinit(ch);
out_chn:
	CVI_VENC_DestroyChn(ch);
	return s32Ret;
}

#if 0
int mmf_enc_h264_init(int ch, int w, int h)
{
	mmf_venc_cfg_t cfg = {
		.type = 2,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = PIXEL_FORMAT_NV21,
		.jpg_quality = 0,  // unused
		.gop = 50,
		.intput_fps = 30,
		.output_fps = 30,
		.bitrate = 3000,
	};

	return _mmf_enc_h264_init(ch, &cfg);
}
#endif

static int mmf_venc_deinit(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_is_init[ch]) {
		return 0;
	}

	mmf_stream_t stream;
	if (!mmf_venc_pop(ch, &stream)) {
		mmf_venc_free(ch);
	}

	_mmf_venc_vpss_deinit(ch);

	s32Ret = CVI_VENC_StopRecvFrame(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StopRecvFrame failed with %d\n", s32Ret);
	}

	s32Ret = CVI_VENC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ResetChn vechn[%d] failed with %#x!\n", ch, s32Ret);
	}

	s32Ret = CVI_VENC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_DestroyChn [%d] failed with %d\n", ch, s32Ret);
	}

	if (priv.enc_chn_frame[ch]) {
		_mmf_free_frame(priv.enc_chn_frame[ch]);
		priv.enc_chn_frame[ch] = NULL;
	}

	priv.enc_chn_cfg[ch].w = 0;
	priv.enc_chn_cfg[ch].h = 0;
	priv.enc_chn_cfg[ch].fmt = 0;
	priv.enc_chn_running[ch] = 0;
	priv.enc_chn_is_init[ch] = 0;

	return s32Ret;
}

int mmf_venc_push2(int ch, void *frame_info)
{
	int out_ch = ch;
	CVI_S32 s32Ret = CVI_SUCCESS;
	VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *)frame_info;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (priv.enc_chn_running[ch]) {
		return s32Ret;
	}
	if (!frame) {
		return -1;
	}

	int w = frame->stVFrame.u32Width;
	int h = frame->stVFrame.u32Height;
	int format = frame->stVFrame.enPixelFormat;
	int quality = priv.enc_chn_cfg[ch].jpg_quality;

	if (priv.enc_chn_cfg[ch].w != w || priv.enc_chn_cfg[ch].h != h || priv.enc_chn_cfg[ch].fmt != format || priv.enc_chn_cfg[ch].jpg_quality != quality) {
		s32Ret = mmf_rst_venc_channel(ch, w, h, format, quality);
		if (s32Ret != CVI_SUCCESS) {
			printf("mmf_rst_venc_channel failed with %d\n", s32Ret);
			return s32Ret;
		}

		priv.enc_chn_cfg[ch].w = w;
		priv.enc_chn_cfg[ch].h = h;
		priv.enc_chn_cfg[ch].fmt = format;
	}

	if (priv.enc_chn_vpss[ch] != VPSS_INVALID_GRP) {
		out_ch = priv.enc_chn_vpss[ch];
	}

	s32Ret = CVI_VENC_SendFrame(out_ch, frame, 1000);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_SendFrame failed with %d\n", s32Ret);
		return s32Ret;
	}

	priv.enc_chn_running[ch] = 1;

	return s32Ret;
}

static int _mmf_venc_push(int ch, uint8_t *data, int w, int h, int format, int quality)
{
	int out_ch = ch;
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (priv.enc_chn_running[ch]) {
		return s32Ret;
	}

	SIZE_S stSize = {(CVI_U32)w, (CVI_U32)h};
	if (priv.enc_chn_frame[ch] == NULL || priv.enc_chn_cfg[ch].w != w || priv.enc_chn_cfg[ch].h != h || priv.enc_chn_cfg[ch].fmt != format || priv.enc_chn_cfg[ch].jpg_quality != quality) {
		s32Ret = mmf_rst_venc_channel(ch, w, h, format, quality);
		if (s32Ret != CVI_SUCCESS) {
			printf("mmf_rst_venc_channel failed with %d\n", s32Ret);
			return s32Ret;
		}

		priv.enc_chn_cfg[ch].w = w;
		priv.enc_chn_cfg[ch].h = h;
		priv.enc_chn_cfg[ch].fmt = format;
		priv.enc_chn_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(priv.enc_chn_vb_id[ch], stSize, (PIXEL_FORMAT_E)format);
		if (!priv.enc_chn_frame[ch]) {
			printf("Alloc frame failed!\r\n");
			return -1;
		}
	}

	switch (format) {
		case PIXEL_FORMAT_RGB_888:
		{
			if (priv.enc_chn_frame[ch]->stVFrame.u32Stride[0] != (CVI_U32)w * 3) {
				for (int h0 = 0; h0 < h; h0 ++) {
					memcpy((uint8_t *)priv.enc_chn_frame[ch]->stVFrame.pu8VirAddr[0] + priv.enc_chn_frame[ch]->stVFrame.u32Stride[0] * h0,
							((uint8_t *)data) + w * h0 * 3, w * 3);
				}
			} else {
				memcpy(priv.enc_chn_frame[ch]->stVFrame.pu8VirAddr[0], data, w * h * 3);
			}
		}
		break;
		case PIXEL_FORMAT_NV21:
		{
			if (priv.enc_chn_frame[ch]->stVFrame.u32Stride[0] != (CVI_U32)w) {
				for (int h0 = 0; h0 < h * 3 / 2; h0 ++) {
					memcpy((uint8_t *)priv.enc_chn_frame[ch]->stVFrame.pu8VirAddr[0] + priv.enc_chn_frame[ch]->stVFrame.u32Stride[0] * h0,
							((uint8_t *)data) + w * h0, w);
				}
			} else {
				uint32_t p0_size = w * h;
				uint32_t p1_size = w * h / 2;
				memcpy(priv.enc_chn_frame[ch]->stVFrame.pu8VirAddr[0], ((uint8_t *)data), p0_size);
				memcpy(priv.enc_chn_frame[ch]->stVFrame.pu8VirAddr[1], ((uint8_t *)data) + p0_size, p1_size);
			}
		}
		break;
		default: return -1;
	}

	if (priv.enc_chn_vpss[ch] != VPSS_INVALID_GRP) {
		out_ch = priv.enc_chn_vpss[ch];
	}

	s32Ret = CVI_VENC_SendFrame(out_ch, priv.enc_chn_frame[ch], 1000);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_SendFrame failed with %d\n", s32Ret);
		return s32Ret;
	}

	priv.enc_chn_running[ch] = 1;

	return s32Ret;
}

int mmf_venc_push(int ch, uint8_t *data, int w, int h, int format)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}

	return _mmf_venc_push(ch, data, w, h, format, priv.enc_chn_cfg[ch].jpg_quality);
}

int mmf_venc_pop(int ch, mmf_stream_t *stream)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_running[ch]) {
		return s32Ret;
	}

	int fd = CVI_VENC_GetFd(ch);
	if (fd < 0) {
		printf("CVI_VENC_GetFd failed with %d\n", fd);
		return -1;
	}

	fd_set readFds;
	struct timeval timeoutVal;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	timeoutVal.tv_sec = 0;
	timeoutVal.tv_usec = 80*1000;
	s32Ret = select(fd + 1, &readFds, NULL, NULL, &timeoutVal);
	if (s32Ret < 0) {
		if (errno == EINTR) {
			printf("VencChn(%d) select failed!\n", ch);
			return -1;
		}
	} else if (s32Ret == 0) {
		printf("VencChn(%d) select timeout!\n", ch);
		return -1;
	}

	priv.enc_chn_stream[ch].pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 8);
	if (!priv.enc_chn_stream[ch].pstPack) {
		printf("Malloc failed!\r\n");
		return -1;
	}

	VENC_CHN_STATUS_S stStatus;
	s32Ret = CVI_VENC_QueryStatus(ch, &stStatus);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_QueryStatus failed with %#x\n", s32Ret);
		return s32Ret;
	}

	if (stStatus.u32CurPacks > 0) {
		s32Ret = CVI_VENC_GetStream(ch, &priv.enc_chn_stream[ch], 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
			free(priv.enc_chn_stream[ch].pstPack);
			priv.enc_chn_stream[ch].pstPack = NULL;
			return s32Ret;
		}
	} else {
		printf("CVI_VENC_QueryStatus find not pack\r\n");
		free(priv.enc_chn_stream[ch].pstPack);
		priv.enc_chn_stream[ch].pstPack = NULL;
		return -1;
	}

	if (stream) {
		stream->count = priv.enc_chn_stream[ch].u32PackCount;
		if (stream->count > 8) {
			printf("pack count is too large! cnt:%d\r\n", stream->count);
			mmf_venc_free(ch);
			return -1;
		}
		for (int i = 0; i < stream->count; i++) {
			stream->data[i] = priv.enc_chn_stream[ch].pstPack[i].pu8Addr + priv.enc_chn_stream[ch].pstPack[i].u32Offset;
			stream->data_size[i] = priv.enc_chn_stream[ch].pstPack[i].u32Len - priv.enc_chn_stream[ch].pstPack[i].u32Offset;
		}
	}

	return s32Ret;
}

int mmf_venc_free(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_running[ch]) {
		return s32Ret;
	}

	if (priv.enc_chn_stream[ch].pstPack) {
		s32Ret = CVI_VENC_ReleaseStream(ch, &priv.enc_chn_stream[ch]);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_ReleaseStream failed with %#x\n", s32Ret);
			return s32Ret;
		}

		free(priv.enc_chn_stream[ch].pstPack);
		priv.enc_chn_stream[ch].pstPack = NULL;
	}

	priv.enc_chn_running[ch] = 0;
	return s32Ret;
}

int mmf_enc_h265_deinit(int ch)
{
	return mmf_venc_deinit(ch);
}

int mmf_enc_h265_push2(int ch, void *frame_info)
{
	return mmf_venc_push2(ch, frame_info);
}

int mmf_enc_h265_push(int ch, uint8_t *data, int w, int h, int format)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}
	if (!priv.enc_chn_is_init[ch]) {
		s32Ret = mmf_enc_h265_init(ch, w, h);
		if (s32Ret != CVI_SUCCESS) {
			printf("mmf_enc_h265_init failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	return _mmf_venc_push(ch, data, w, h, format, 0);
}

int mmf_enc_h265_pop(int ch, mmf_h265_stream_t *stream)
{
	return mmf_venc_pop(ch, (mmf_stream_t *)stream);
}

#ifdef __cplusplus
int mmf_enc_h265_pop(int ch, mmf_stream_t *stream)
{
	return mmf_venc_pop(ch, stream);
}
#endif

int mmf_enc_h265_free(int ch)
{
	return mmf_venc_free(ch);
}

int mmf_venc_unused_channel(void) {
	for (int i = 0; i < MMF_ENC_MAX_CHN; i++) {
		if (!priv.enc_chn_is_init[i]) {
			return i;
		}
	}
	return -1;
}

int mmf_venc_is_used(int ch)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		return -1;
	}

	return priv.enc_chn_running[ch];
}

int mmf_venc_get_cfg(int ch, mmf_venc_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		printf("%s: channel %d is out of range.\n", __func__, ch);
		return -1;
	}
	if (!priv.enc_chn_is_init[ch]) {
		printf("%s: channel %d is not initialized.\n", __func__, ch);
		return -1;
	}
	if (!cfg) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}

	memcpy(cfg, &priv.enc_chn_cfg[ch], sizeof(priv.enc_chn_cfg[ch]));

	return 0;
}

int mmf_add_venc_channel(int ch, mmf_venc_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		printf("%s: channel %d is out of range.\n", __func__, ch);
		return -1;
	}
	if (!cfg) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}
	if (mmf_invert_codec_to_mmf(cfg->type) == PT_H265)
		return _mmf_enc_h265_init(ch, cfg);
	if (mmf_invert_codec_to_mmf(cfg->type) == PT_H264)
		return _mmf_enc_h264_init(ch, cfg);
	if (mmf_invert_codec_to_mmf(cfg->type) == PT_JPEG ||
	    mmf_invert_codec_to_mmf(cfg->type) == PT_MJPEG)
		return _mmf_enc_jpg_init(ch, cfg);

	printf("%s: type %d not supported.\n", __func__, cfg->type);
	return -1;
}

int mmf_del_venc_channel(int ch)
{
	return mmf_venc_deinit(ch);
}

int mmf_del_venc_channel_all()
{
	for (int i = 0; i < MMF_ENC_MAX_CHN; i++)
		mmf_del_venc_channel(i);

	return 0;
}

static int mmf_rst_venc_channel(int ch, int w, int h, int format, int quality)
{
	mmf_venc_cfg_t cfg;

	if (ch < 0 || ch >= MMF_ENC_MAX_CHN) {
		printf("%s: channel %d is out of range.\n", __func__, ch);
		return -1;
	}
	if (!priv.enc_chn_is_init[ch]) {
		printf("%s: channel %d is not initialized.\n", __func__, ch);
		return -1;
	}

	memcpy(&cfg, &priv.enc_chn_cfg[ch], sizeof(cfg));
	mmf_del_venc_channel(ch);
	cfg.w = w;
	cfg.h = h;
	cfg.fmt = format;
	cfg.jpg_quality = quality;

	return mmf_add_venc_channel(ch, &cfg);
}

static int _mmf_vdec_init(int ch, int format_out, VDEC_CHN_ATTR_S *chn_attr_out, SIZE_S size_in, int vb_id)
{
	int format_in = PIXEL_FORMAT_YUV_PLANAR_444; //PIXEL_FORMAT_YUV_PLANAR_420;
	CVI_U32 w_in = size_in.u32Width;
	CVI_U32 h_in = size_in.u32Height;
	CVI_U32 w_out = chn_attr_out->u32PicWidth;
	CVI_U32 h_out = chn_attr_out->u32PicHeight;
	int fps_out = 30;

	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		return -1;
	}
	if (priv.dec_chn_is_init[ch])
		return 0;

	if ((format_out == PIXEL_FORMAT_RGB_888 && w_out * h_out * 3 > 640 * 480 * 3)
		|| (format_out == PIXEL_FORMAT_NV21 && w_out * h_out * 3 / 2 > 2560 * 1440 * 3 / 2)) {
		printf("image size is too large, for NV21, maximum resolution 2560x1440, for RGB888, maximum resolution 640x480!\n");
		return -1;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	VDEC_CHN_ATTR_S *chn_attr = (VDEC_CHN_ATTR_S *)malloc(sizeof(VDEC_CHN_ATTR_S));
	if (!chn_attr) {
		return -1;
	}
	memcpy(chn_attr, chn_attr_out, sizeof(VDEC_CHN_ATTR_S));
	chn_attr->u32PicWidth = w_in;
	chn_attr->u32PicHeight = h_in;
	chn_attr->u32FrameBufCnt = 1;
	chn_attr->u32FrameBufSize = VDEC_GetPicBufferSize(
			chn_attr->enType, w_in, h_in,
			(PIXEL_FORMAT_E)format_in, DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
	chn_attr->u32StreamBufSize = w_in * h_in; //ALIGN(w * h, 0x4000);

	s32Ret = CVI_VDEC_CreateChn(ch, chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		goto out_chn_attr;
	}

	s32Ret = CVI_VDEC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_ResetChn [%d] failed with %#x\n", ch, s32Ret);
		goto out_chn_attr;
	}

	s32Ret = CVI_VDEC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_DestoryChn [%d] failed with %#x\n", ch, s32Ret);
	}

	s32Ret = CVI_VDEC_CreateChn(ch, chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		goto out_chn_attr;
	}

#if 0
	VDEC_MOD_PARAM_S stModParam;
	CVI_VDEC_GetModParam(&stModParam);
	stModParam.enVdecVBSource = VB_SOURCE_COMMON;
	CVI_VDEC_SetModParam(&stModParam);
#endif

	priv.dec_chn_vpss[ch] = VPSS_INVALID_GRP;

	switch (format_out) {
	case PIXEL_FORMAT_RGB_888:
		//fallthrough;
	case PIXEL_FORMAT_NV21:
	{
		VPSS_GRP out_grp = CVI_VPSS_GetAvailableGrp();

		if (out_grp == VPSS_INVALID_GRP) {
			printf("VPSS get group failed\n");
			CVI_VDEC_DestroyChn(ch);
			s32Ret = CVI_FAILURE;
			goto out_chn_attr;
		}

		s32Ret = _mmf_vpss_init(out_grp, ch, (SIZE_S){w_in, h_in}, (SIZE_S){w_out, h_out}, (PIXEL_FORMAT_E)format_in, (PIXEL_FORMAT_E)format_out, fps_out, 0, CVI_FALSE, CVI_FALSE, 0);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS init failed with %d\n", s32Ret);
			CVI_VDEC_DestroyChn(ch);
			goto out_chn_attr;
		}

		s32Ret = SAMPLE_COMM_VDEC_Bind_VPSS(ch, out_grp);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS bind VDEC failed with %#x\n", s32Ret);
			_mmf_vpss_deinit(out_grp, ch);
			CVI_VDEC_StopRecvStream(ch);
			CVI_VDEC_DestroyChn(ch);
			goto out_chn_attr;
		}

		priv.dec_chn_vpss[ch] = out_grp;
		break;
	}
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_444:
		break;
	default:
		printf("unknown format %d (want %d)!\n", format_out, PIXEL_FORMAT_NV21);
		CVI_VDEC_StopRecvStream(ch);
		CVI_VDEC_DestroyChn(ch);
		s32Ret = CVI_FAILURE;
		goto out_chn_attr;
	}

	VDEC_CHN_PARAM_S stChnParam;
	s32Ret = CVI_VDEC_GetChnParam(ch, &stChnParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_GetChnParam failed with %#x\n", s32Ret);
		s32Ret = CVI_FAILURE;
		goto out_vpss;
	}

	stChnParam.enPixelFormat = (PIXEL_FORMAT_E)format_in; //(PIXEL_FORMAT_E)format_out;
	stChnParam.u32DisplayFrameNum = chn_attr->u32FrameBufCnt - 1;

	s32Ret = CVI_VDEC_SetChnParam(ch, &stChnParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_SetChnParam failed with %#x\n", s32Ret);
		s32Ret = CVI_FAILURE;
		goto out_vpss;
	}

	s32Ret = CVI_VDEC_StartRecvStream(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_StartRecvStream failed with %#x\n", s32Ret);
		s32Ret = CVI_FAILURE;
		goto out_vpss;
	}

	priv.dec_chn_type[ch] = chn_attr->enType;
	priv.dec_chn_vb_id[ch] = vb_id;

	priv.dec_chn_size_in[ch] = (SIZE_S){w_in, h_in};
	priv.dec_chn_cfg[ch].type = mmf_invert_codec_to_maix(priv.dec_chn_type[ch]);
	priv.dec_chn_cfg[ch].w = w_out;
	priv.dec_chn_cfg[ch].h = h_out;
	priv.dec_chn_cfg[ch].fmt = format_out;
	priv.dec_chn_cfg[ch].buffer_num = chn_attr->u32FrameBufCnt;
	priv.dec_chn_is_init[ch] = 1;
	priv.dec_chn_running[ch] = 0;

	goto out_chn_attr;

out_vpss:
	if (priv.dec_chn_vpss[ch] != VPSS_INVALID_GRP)
	{
		VPSS_GRP out_grp = priv.dec_chn_vpss[ch];
		SAMPLE_COMM_VDEC_UnBind_VPSS(ch, out_grp);
		_mmf_vpss_deinit(out_grp, ch);
		priv.dec_chn_vpss[ch] = VPSS_INVALID_GRP;
	}
out_chn_attr:
	free(chn_attr);
	return s32Ret;
}

#if 0
int mmf_dec_jpg_init(int ch, int w, int h, int format)
{
	mmf_vdec_cfg_t cfg = {
		.type = 4,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = format,
		.buffer_num = 1,
	};

	return mmf_add_vdec_channel(ch, &cfg);
}

int mmf_dec_jpg_deinit(int ch)
{
	return  mmf_vdec_deinit(ch);
}
#endif

static int _mmf_dec_jpg_get_frame_info(uint8_t *data, uint32_t size, uint32_t *width, uint32_t *height, int *format)
{
	uint8_t *j_ptr = data;
	uint8_t jpeg_b = 0, jpeg_c = 0;
	uint32_t jpeg_w = 0, jpeg_h = 0;

	if (data == NULL || size < 1024 || width == NULL || height == NULL || format == NULL) {
		printf("invalid param\n");
		return -1;
	}

	// Start of Image (SoI) marker and Application x (APPx) segment
	if (j_ptr[0] == 0xFF && j_ptr[1] == 0xD8 &&
	    j_ptr[2] == 0xFF && (j_ptr[3] & 0xF0) == 0xE0) {
		j_ptr += 2;
		while (j_ptr < data + 1024 - 2 - 8) {
			// segment length
			uint32_t jm_len = (j_ptr[2] << 8) | j_ptr[3];
			// Start of Frame (SoF) segment
			if (j_ptr[0] == 0xFF && j_ptr[1] == 0xC0 &&
			    j_ptr[2] == 0x00 && j_ptr[3] >= 0x08 && j_ptr[3] < 0x12) {
				// bits per sample
				jpeg_b = j_ptr[4];
				// image height
				jpeg_h = (j_ptr[5] << 8) | j_ptr[6];
				// image width
				jpeg_w = (j_ptr[7] << 8) | j_ptr[8];
				// component count
				jpeg_c = j_ptr[9];
				break;
			}
			j_ptr += 2;
			j_ptr += jm_len;
		}
	}

	if (jpeg_w && jpeg_h && jpeg_b && jpeg_c) {
#ifdef DEBUG_EN
		printf("%s: %dx%d %d bps %d components\n", __func__, jpeg_w, jpeg_h, jpeg_b, jpeg_c);
#endif
		*width = jpeg_w;
		*height = jpeg_h;
	}

	return 0;
}

#if 0
int mmf_dec_h265_init(int ch, int w, int h)
{
	mmf_vdec_cfg_t cfg = {
		.type = 1,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = PIXEL_FORMAT_NV21,
		.buffer_num = 1,
	};

	return mmf_add_vdec_channel(ch, &cfg);
}
#endif

#if 0
int mmf_dec_h264_init(int ch, int w, int h)
{
	mmf_vdec_cfg_t cfg = {
		.type = 2,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = PIXEL_FORMAT_NV21,
		.buffer_num = 1,
	};

	return mmf_add_vdec_channel(ch, &cfg);
}
#endif

static int mmf_vdec_deinit(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		return -1;
	}
	if (!priv.dec_chn_is_init[ch]) {
		return 0;
	}

#if 0
	VIDEO_FRAME_INFO_S frame;
	if (!mmf_vdec_pop(ch, &frame)) {
		mmf_vdec_free(ch);
	}
#endif

	if (priv.dec_chn_vpss[ch] != VPSS_INVALID_GRP) {
		VPSS_GRP out_grp = priv.dec_chn_vpss[ch];

		s32Ret = SAMPLE_COMM_VDEC_UnBind_VPSS(ch, out_grp);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS unbind VDEC failed with %d\n", s32Ret);
		}

		s32Ret = _mmf_vpss_deinit(out_grp, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS deinit failed with %d\n", s32Ret);
		}

		priv.dec_chn_vpss[ch] = VPSS_INVALID_GRP;
	}

	s32Ret = CVI_VDEC_StopRecvStream(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_StopRecvStream failed with %d\n", s32Ret);
	}

	s32Ret = CVI_VDEC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_ResetChn vechn[%d] failed with %#x!\n", ch, s32Ret);
	}

	s32Ret = CVI_VDEC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_DestroyChn [%d] failed with %d\n", ch, s32Ret);
	}

	priv.dec_chn_size_in[ch] = (SIZE_S){0, 0};
	priv.dec_chn_cfg[ch].w = 0;
	priv.dec_chn_cfg[ch].h = 0;
	priv.dec_chn_cfg[ch].fmt = 0;
	priv.dec_chn_cfg[ch].buffer_num = 0;
	priv.dec_chn_running[ch] = 0;
	priv.dec_chn_is_init[ch] = 0;

	return s32Ret;
}

static int _mmf_vdec_push(int ch, VDEC_STREAM_S *stStream)
{
	int out_ch = ch;
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		return -1;
	}
	if (priv.dec_chn_running[ch]) {
		return s32Ret;
	}

	uint32_t stream_w = priv.dec_chn_size_in[ch].u32Width;
	uint32_t stream_h = priv.dec_chn_size_in[ch].u32Height;
	int stream_fmt = 0;
	if (priv.dec_chn_type[ch] == PT_JPEG) {
		_mmf_dec_jpg_get_frame_info(stStream->pu8Addr, stStream->u32Len, &stream_w, &stream_h, &stream_fmt);
	}
	if (stream_w != priv.dec_chn_size_in[ch].u32Width || stream_h != priv.dec_chn_size_in[ch].u32Height)
	{
		s32Ret = mmf_rst_vdec_channel(ch, &priv.dec_chn_cfg[ch], (SIZE_S){stream_w, stream_h});
		if (s32Ret != CVI_SUCCESS) {
			return s32Ret;
		}
	}

	s32Ret = CVI_VDEC_SendStream(out_ch, stStream, 1000);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VDEC_SendStream failed with %d\n", s32Ret);
		return s32Ret;
	}

	if (stStream->bDisplay)
		priv.dec_chn_running[ch] = 1;

	return s32Ret;
}

int mmf_vdec_push(int ch, uint8_t *data, int size, uint8_t is_start, uint8_t is_end)
{
	VDEC_STREAM_S *stStream;
	UNUSED(is_start);

	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		return -1;
	}
	if (!data && (size != 0)) {
		return -1;
	}

	stStream = &priv.dec_chn_stream[ch];
	memset(stStream, 0, sizeof(*stStream));

	stStream->u64PTS = 0;
	stStream->pu8Addr = data;
	stStream->u32Len = size;
	stStream->bEndOfFrame = (size != 0) ? CVI_TRUE : CVI_FALSE;
	stStream->bEndOfStream = is_end ? CVI_TRUE : CVI_FALSE;
	stStream->bDisplay = (size != 0) ? 1 : 0;

	return _mmf_vdec_push(ch, &priv.dec_chn_stream[ch]);
}

static int _mmf_vdec_pop(int ch, VIDEO_FRAME_INFO_S *frame)
{
	VPSS_GRP out_grp = 0;

	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}
	if (!priv.dec_chn_is_init[ch]) {
		printf("dec ch %d not open\n", ch);
		return -1;
	}
	if (!priv.dec_chn_running[ch]) {
		return -1;
	}
	if (frame == NULL) {
		printf("invalid param\n");
		return -1;
	}

	int fd = CVI_VDEC_GetFd(ch);
	if (fd < 0) {
		printf("CVI_VDEC_GetFd failed with %d\n", fd);
		return -1;
	}

	fd_set readFds;
	struct timeval timeoutVal;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	timeoutVal.tv_sec = 0;
	timeoutVal.tv_usec = 80*1000;
	s32Ret = select(fd + 1, &readFds, NULL, NULL, &timeoutVal);
	if (s32Ret < 0) {
		if (errno == EINTR) {
			printf("VdecChn(%d) select failed!\n", ch);
			return -1;
		}
	} else if (s32Ret == 0) {
		printf("VdecChn(%d) select timeout!\n", ch);
		return -1;
	}

	if (priv.dec_chn_vpss[ch] != VPSS_INVALID_GRP) {
		out_grp = priv.dec_chn_vpss[ch];

		s32Ret = CVI_VPSS_GetChnFrame(out_grp, ch, frame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VPSS_GetChnFrame failed, grp:%d\n", out_grp);
			return -1;
		}

		int image_size = 0;
		for (int b = 0; b < 3; b++) if (frame->stVFrame.u64PhyAddr[b])
		{
			CVI_VOID *vir_addr;
			CVI_U32 vir_size = frame->stVFrame.u32Length[b];
			vir_addr = CVI_SYS_MmapCache(frame->stVFrame.u64PhyAddr[b], vir_size);
			CVI_SYS_IonInvalidateCache(frame->stVFrame.u64PhyAddr[b], vir_addr, vir_size);
			frame->stVFrame.pu8VirAddr[b] = (CVI_U8 *)vir_addr;
			image_size += vir_size;
		}

		// printf("width: %d, height: %d, total_buf_length: %d, phy:%#lx  vir:%p\n",
		// 	   frame->stVFrame.u32Width,
		// 	   frame->stVFrame.u32Height, image_size,
		// 	   frame->stVFrame.u64PhyAddr[0], vir_addr);

	} else {
		s32Ret = CVI_VDEC_GetFrame(ch, frame, priv.dec_pop_timeout);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VDEC_GetFrame failed\n");
			return -1;
		}

		// map already done by CVI_VDEC_GetFrame
	}

	if (s32Ret == 0) {
		priv.dec_pst_frame[ch] = frame;

		return 0;
	}

	return s32Ret;
}

int mmf_vdec_pop(int ch, void **data, int *len, int *width, int *height, int *format)
{
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}
	if (data == NULL || len == NULL || width == NULL || height == NULL || format == NULL) {
		printf("invalid param\n");
		return -1;
	}

	VIDEO_FRAME_INFO_S *frame = &priv.dec_chn_frame[ch];
	CVI_S32 s32Ret = _mmf_vdec_pop(ch, frame);
	if (s32Ret != 0) {
		return s32Ret;
	}

	int image_size = frame->stVFrame.u32Length[0]
		       + frame->stVFrame.u32Length[1]
		       + frame->stVFrame.u32Length[2];

	*data = frame->stVFrame.pu8VirAddr[0];
	*len = image_size;
	*width = frame->stVFrame.u32Width;
	*height = frame->stVFrame.u32Height;
	*format = frame->stVFrame.enPixelFormat;

	return s32Ret;
}

int mmf_vdec_free(int ch)
{
	int out_grp = 0;
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}
	if (!priv.dec_chn_is_init[ch]) {
		printf("dec ch %d not open\n", ch);
		return -1;
	}
	if (!priv.dec_chn_running[ch]) {
		return s32Ret;
	}

	VIDEO_FRAME_INFO_S *frame = priv.dec_pst_frame[ch];
	if (!frame)
	{
		priv.dec_chn_running[ch] = 0;
		return s32Ret;
	}

	if (priv.dec_chn_vpss[ch] != VPSS_INVALID_GRP) {
		out_grp = priv.dec_chn_vpss[ch];

		for (int b = 0; b < 3; b++) if (frame->stVFrame.pu8VirAddr[b])
		{
			CVI_VOID *vir_addr = frame->stVFrame.pu8VirAddr[b];
			CVI_U32 vir_size = frame->stVFrame.u32Length[b];
			CVI_SYS_Munmap(vir_addr, vir_size);
			frame->stVFrame.pu8VirAddr[b] = NULL;
		}

		s32Ret = CVI_VPSS_ReleaseChnFrame(out_grp, ch, frame);
	} else {
		// unmap will be done by CVI_VDEC_ReleaseFrame

		s32Ret = CVI_VDEC_ReleaseFrame(ch, frame);
	}

	if (s32Ret != 0) {
		printf("CVI_VDEC_ReleaseFrame failed\n");
		return -1;
	}

	priv.dec_pst_frame[ch] = NULL;
	priv.dec_chn_running[ch] = 0;

	return s32Ret;
}

#if 0
int mmf_dec_h265_deinit(int ch)
{
	return mmf_vdec_deinit(ch);
}
#endif

int mmf_vdec_unused_channel(void) {
	for (int i = 0; i < MMF_DEC_MAX_CHN; i++) {
		if (!priv.dec_chn_is_init[i]) {
			return i;
		}
	}
	return -1;
}

int mmf_vdec_is_used(int ch)
{
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		return -1;
	}

	return priv.dec_chn_running[ch];
}

int mmf_vdec_get_cfg(int ch, mmf_vdec_cfg_t *cfg)
{
	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		printf("%s: channel %d is out of range.\n", __func__, ch);
		return -1;
	}
	if (!priv.dec_chn_is_init[ch]) {
		printf("%s: channel %d is not initialized.\n", __func__, ch);
		return -1;
	}
	if (!cfg) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}

	memcpy(cfg, &priv.dec_chn_cfg[ch], sizeof(priv.dec_chn_cfg[ch]));

	return 0;
}

static int _mmf_add_vdec_channel(int ch, int format_out, VDEC_CHN_ATTR_S *chn_attr, SIZE_S size_in)
{
	int vb_id = MMF_VB_DEC_JPEG_ID;

	if (ch < 0 || ch >= MMF_DEC_MAX_CHN) {
		printf("%s: channel %d is out of range.\n", __func__, ch);
		return -1;
	}
	if (!chn_attr) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}

	switch (chn_attr->enType) {
		case PT_H265:
			vb_id = MMF_VB_DEC_H26X_ID;
			break;
		case PT_H264:
			vb_id = MMF_VB_DEC_H26X_ID;
			break;
		case PT_MJPEG:
			//fallthrough;
		case PT_JPEG:
			vb_id = MMF_VB_DEC_JPEG_ID;
			break;
		default:
			printf("%s: type %d may not be supported.\n", __func__, chn_attr->enType);
			//return -1;
			break;
	}

	return _mmf_vdec_init(ch, format_out, chn_attr, size_in, vb_id);
}

int mmf_add_vdec_channel(int ch, mmf_vdec_cfg_t *cfg)
{
	VDEC_CHN_ATTR_S vdec_chn_attr;

	if (!cfg) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}

	memset(&vdec_chn_attr, 0, sizeof(VDEC_CHN_ATTR_S));
	vdec_chn_attr.enType = mmf_invert_codec_to_mmf(cfg->type);
	vdec_chn_attr.enMode = VIDEO_MODE_FRAME;
	vdec_chn_attr.u32PicWidth = cfg->w;
	vdec_chn_attr.u32PicHeight = cfg->h;
	vdec_chn_attr.u32FrameBufCnt = cfg->buffer_num;
	vdec_chn_attr.u32FrameBufSize = VDEC_GetPicBufferSize(
			vdec_chn_attr.enType, cfg->w, cfg->h,
			(PIXEL_FORMAT_E)cfg->fmt, DATA_BITWIDTH_8, COMPRESS_MODE_NONE);

	return _mmf_add_vdec_channel(ch, cfg->fmt, &vdec_chn_attr, (SIZE_S){(CVI_U32)cfg->w, (CVI_U32)cfg->h});
}

int mmf_del_vdec_channel(int ch)
{
	return mmf_vdec_deinit(ch);
}

int mmf_del_vdec_channel_all()
{
	for (int i = 0; i < MMF_DEC_MAX_CHN; i++)
		mmf_del_vdec_channel(i);

	return 0;
}

static int mmf_rst_vdec_channel(int ch, mmf_vdec_cfg_t *cfg, SIZE_S size_in)
{
	int format_out;
	VDEC_CHN_ATTR_S vdec_chn_attr;

	if (!cfg) {
		printf("%s: cfg is not set.\n", __func__);
		return -1;
	}

	format_out = cfg->fmt;
	memset(&vdec_chn_attr, 0, sizeof(VDEC_CHN_ATTR_S));
	vdec_chn_attr.enType = mmf_invert_codec_to_mmf(cfg->type);
	vdec_chn_attr.enMode = VIDEO_MODE_FRAME;
	vdec_chn_attr.u32PicWidth = cfg->w;
	vdec_chn_attr.u32PicHeight = cfg->h;
	vdec_chn_attr.u32FrameBufCnt = cfg->buffer_num;
	vdec_chn_attr.u32FrameBufSize = VDEC_GetPicBufferSize(
			vdec_chn_attr.enType, cfg->w, cfg->h,
			(PIXEL_FORMAT_E)cfg->fmt, DATA_BITWIDTH_8, COMPRESS_MODE_NONE);

	if (0 != mmf_vdec_deinit(ch)) {
		return -1;
	}

	return _mmf_add_vdec_channel(ch, format_out, &vdec_chn_attr, size_in);
}

int mmf_invert_format_to_maix(int mmf_format) {
	switch (mmf_format) {
		case PIXEL_FORMAT_RGB_888:
			return 0;
		case PIXEL_FORMAT_BGR_888:
			return 1;
		case PIXEL_FORMAT_ARGB_8888:
			return 3;
		case PIXEL_FORMAT_NV21:
			return 8;
		default:
			return 0xFF;
	}
}

int mmf_invert_format_to_mmf(int maix_format) {
	switch (maix_format) {
		case 0:
			return PIXEL_FORMAT_RGB_888;
		case 1:
			return PIXEL_FORMAT_BGR_888;
		case 3:
			return PIXEL_FORMAT_ARGB_8888;
		case 8:
			return PIXEL_FORMAT_NV21;
		default:
			return -1;
	}
}

int mmf_vb_config_of_vi(uint32_t size, uint32_t count)
{
	priv.vb_size_of_vi = size;
	priv.vb_count_of_vi = count;
	priv.vb_of_vi_is_config = 1;
	return 0;
}

int mmf_vb_config_of_vo(uint32_t size, uint32_t count)
{
	priv.vb_size_of_vo = size;
	priv.vb_count_of_vo = count;
	priv.vb_of_vo_is_config = 1;
	return 0;
}

int mmf_vb_config_of_private(uint32_t size, uint32_t count)
{
	priv.vb_size_of_private = size;
	priv.vb_count_of_private = count;
	priv.vb_of_private_is_config = 1;
	return 0;
}

int mmf_set_exp_mode(int ch, int mode)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;

	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));

	CVI_ISP_GetExposureAttr(ch, &stExpAttr);

	stExpAttr.u8DebugMode = 0;

	if (mode == 0) {
		stExpAttr.bByPass = 0;
		stExpAttr.enOpType = OP_TYPE_AUTO;
		stExpAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		stExpAttr.stManual.enISONumOpType = OP_TYPE_AUTO;
		printf("AE Auto!\n");
	} else if (mode == 1) {
		stExpAttr.bByPass = 0;
		stExpAttr.enOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enISONumOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enGainType = AE_TYPE_ISO;
		printf("AE Manual!\n");
	}

	CVI_ISP_SetExposureAttr(ch, &stExpAttr);

	//usleep(100 * 1000);
	return 0;
}

// 0, auto; 1, manual
int mmf_get_exp_mode(int ch)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	return stExpAttr.enOpType;
}

int mmf_get_exptime(int ch, uint32_t *exptime)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);
	if (exptime) {
		*exptime = stExpInfo.u32ExpTime;
	}
	return 0;
}

int mmf_set_exptime(int ch, uint32_t exptime)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ExpTime = exptime;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_iso_num(int ch, uint32_t *iso_num)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);
	if (iso_num) {
		*iso_num = stExpInfo.u32ISO;
	}
	return 0;
}

int mmf_set_iso_num(int ch, uint32_t iso_num)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ISONum = iso_num;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_exptime_and_iso(int ch, uint32_t *exptime, uint32_t *iso_num)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);

	if (exptime) {
		*exptime = stExpInfo.u32ExpTime;
	}

	if (iso_num) {
		*iso_num = stExpInfo.u32ISO;
	}
	return 0;
}

int mmf_set_exptime_and_iso(int ch, uint32_t exptime, uint32_t iso_num)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ExpTime = exptime;
		stExpAttr.stManual.u32ISONum = iso_num;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_sensor_id(void)
{
	switch (priv.sensor_type) {
	case GCORE_GC4653_MIPI_4M_30FPS_10BIT: return 0x4653;
	case LONTIUM_LT6911_2M_60FPS_8BIT: return 0x6911;
	default: return -1;
	}

	return -1;
}

void mmf_get_vi_hmirror(int ch, bool *en)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	if (!en)
		return;

	*en = g_priv.vi_hmirror[ch];
}

void mmf_set_vi_hmirror(int ch, bool en)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	g_priv.vi_hmirror[ch] = en;
}

void mmf_get_vi_vflip(int ch, bool *en)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	if (!en)
		return;

	*en = g_priv.vi_vflip[ch];
}

void mmf_set_vi_vflip(int ch, bool en)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	g_priv.vi_vflip[ch] = en;
}

void mmf_get_vo_video_hmirror(int ch, bool *en)
{
	if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VO_VIDEO_MAX_CHN);
		return;
	}

	if (!en)
		return;

	*en = g_priv.vo_video_hmirror[ch];
}

void mmf_set_vo_video_hmirror(int ch, bool en)
{
	if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VO_VIDEO_MAX_CHN);
		return;
	}

	g_priv.vo_video_hmirror[ch] = en;
}

void mmf_get_vo_video_flip(int ch, bool *en)
{
	if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VO_VIDEO_MAX_CHN);
		return;
	}

	if (!en)
		return;

	*en = g_priv.vo_video_vflip[ch];
}

void mmf_set_vo_video_flip(int ch, bool en)
{
	if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VO_VIDEO_MAX_CHN);
		return;
	}

	g_priv.vo_video_vflip[ch] = en;
}

int mmf_get_constrast(int ch, uint32_t *value)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return -1;
	}

	if (!value)
		return -1;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	*value = stCscAttr.Contrast;

	return 0;
}

void mmf_set_constrast(int ch, uint32_t val)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Contrast = val;
	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}

int mmf_get_saturation(int ch, uint32_t *value)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return -1;
	}

	if (!value)
		return -1;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	*value = stCscAttr.Saturation;

	return 0;
}

void mmf_set_saturation(int ch, uint32_t val)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Saturation = val;

	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}

int mmf_get_luma(int ch, uint32_t *value)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return -1;
	}

	if (!value)
		return -1;

	ISP_CSC_ATTR_S stCscAttr;
	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	*value = stCscAttr.Luma;

	return 0;
}

void mmf_set_luma(int ch, uint32_t val)
{
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d, must be >= 0 < %d\r\n", ch, MMF_VI_MAX_CHN);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;
	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Luma = val;
	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}

int mmf_vi_get_max_size(int *width, int *height)
{
	if (!width)
		return -1;
	if (!height)
		return -1;

	*width = priv.vi_max_size.u32Width;
	*height = priv.vi_max_size.u32Height;

	return 0;
}

void mmf_vi_set_pop_timeout(int ms)
{
	priv.vi_pop_timeout = ms;
}

int mmf_vi_channel_set_windowing(int ch, int x, int y, int w, int h)
{
	return -1;
}

int mmf_get_again(int ch, uint32_t *gain)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);

	if (gain) {
		*gain = stExpInfo.u32AGain;
	}
	return 0;
}

int mmf_set_again(int ch, uint32_t gain)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.u32AGain = gain;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_dgain(int ch, uint32_t *gain)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);

	if (gain) {
		*gain = stExpInfo.u32DGain;
	}
	return 0;
}

int mmf_set_dgain(int ch, uint32_t gain)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.u32DGain = gain;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_ispdgain(int ch, uint32_t *gain)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);

	if (gain) {
		*gain = stExpInfo.u32ISPDGain;
	}
	return 0;
}

int mmf_set_ispdgain(int ch, uint32_t gain)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.u32ISPDGain = gain;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_set_wb_mode(int ch, int mode)
{
	return -1;
}

int mmf_get_wb_mode(int ch)
{
	return 0;
}

int mmf_init0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 1))
		return -1;
	va_start(ap, param);
	int reload_kmod = va_arg(ap, int);
	va_end(ap);

	UNUSED(reload_kmod);
	return mmf_init();
}

int mmf_deinit0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 1))
		return -1;
	va_start(ap, param);
	int force = va_arg(ap, int);
	va_end(ap);

	UNUSED(force);
	return mmf_deinit();
}

int mmf_vi_init0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if (priv.vi_is_inited) {
		return 0;
	}

	if ((method != 0) || (n_args < 8))
		return -1;
	va_start(ap, param);
	int width = va_arg(ap, int);
	int height = va_arg(ap, int);
	int vi_format = va_arg(ap, int);
	int vpss_format = va_arg(ap, int);
	int fps = va_arg(ap, int);
	int pool_num = va_arg(ap, int);
	SAMPLE_VI_CONFIG_S *vi_cfg = va_arg(ap, SAMPLE_VI_CONFIG_S*);
	va_end(ap);

	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.vi_is_inited) {
		return s32Ret;
	}

	priv.vi_format = (PIXEL_FORMAT_E)vi_format;
	priv.vi_size.u32Width = width;
	priv.vi_size.u32Height = height;
	UNUSED(pool_num);
	UNUSED(vi_cfg);
	return _mmf_vi_init(priv.vi_size.u32Width, priv.vi_size.u32Height, (PIXEL_FORMAT_E)vpss_format, fps);
}

int mmf_add_vi_channel0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 10))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	int width = va_arg(ap, int);
	int height = va_arg(ap, int);
	int format = va_arg(ap, int);
	int fps = va_arg(ap, int);
	int depth = va_arg(ap, int);
	int mirror = va_arg(ap, int);
	int vflip = va_arg(ap, int);
	int fit = va_arg(ap, int);
	int pool_num = va_arg(ap, int);
	va_end(ap);

	UNUSED(pool_num);
	priv.vi_pop_timeout = 3000;
	return _mmf_add_vi_channel(ch, width, height, format, fps, depth, mirror, vflip, fit);
}

int mmf_add_vo_channel0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 14))
		return -1;
	va_start(ap, param);
	int layer = va_arg(ap, int);
	int ch = va_arg(ap, int);
	int width = va_arg(ap, int);
	int height = va_arg(ap, int);
	int format_in = va_arg(ap, int);
	int format_out = va_arg(ap, int);
	int fps = va_arg(ap, int);
	int depth = va_arg(ap, int);
	int mirror = va_arg(ap, int);
	int vflip = va_arg(ap, int);
	int fit = va_arg(ap, int);
	int rotate = va_arg(ap, int);
	int pool_num_in = va_arg(ap, int);
	int pool_num_out = va_arg(ap, int);
	va_end(ap);

	priv.vo_rotate = rotate;
	UNUSED(pool_num_in);
	UNUSED(pool_num_out);
	return  _mmf_add_vo_channel(layer, ch, width, height, format_in, format_out, fps, depth, mirror, vflip, fit);
}

int mmf_add_region_channel0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 10))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	int type = va_arg(ap, int);
	int mod_id = va_arg(ap, int);
	int dev_id = va_arg(ap, int);
	int chn_id = va_arg(ap, int);
	int x = va_arg(ap, int);
	int y = va_arg(ap, int);
	int width = va_arg(ap, int);
	int height = va_arg(ap, int);
	int format = va_arg(ap, int);
	va_end(ap);

	return mmf_add_region_channel(ch, type, mod_id, dev_id, chn_id, x, y, width, height, format);
}

int mmf_add_venc_channel0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 2))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	void *cfg = va_arg(ap, void*);
	va_end(ap);

	return mmf_add_venc_channel(ch, (mmf_venc_cfg_t*)cfg);
}

int mmf_add_vdec_channel0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	int format_out = 0;
	int pool_num = 0;
	va_list ap;

	if ((method != 0) || (n_args < 2))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	if (n_args > 2)
		format_out = va_arg(ap, int);
	if (n_args > 3)
		pool_num = va_arg(ap, int);
	void *cfg = va_arg(ap, void*);
	va_end(ap);

	UNUSED(pool_num);
	if (!cfg) {
		return -1;
	}
	if (n_args > 3) {
		VDEC_CHN_ATTR_S *chn_attr = (VDEC_CHN_ATTR_S *)cfg;
		return _mmf_add_vdec_channel(ch, format_out, chn_attr, (SIZE_S){chn_attr->u32PicWidth, chn_attr->u32PicHeight});
	}
	return mmf_add_vdec_channel(ch, (mmf_vdec_cfg_t *)cfg);
}

int mmf_vdec_push0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 2))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	void *stStream = va_arg(ap, void*);
	va_end(ap);

	return _mmf_vdec_push(ch, (VDEC_STREAM_S *)stStream);
}

int mmf_vdec_pop0(uint32_t param, ...)
{
	int method = MMF_FUNC_GET_PARAM_METHOD(param);
	int n_args = MMF_FUNC_GET_PARAM_NUM(param);
	va_list ap;

	if ((method != 0) || (n_args < 2))
		return -1;
	va_start(ap, param);
	int ch = va_arg(ap, int);
	void *frame = va_arg(ap, void*);
	va_end(ap);

	return _mmf_vdec_pop(ch, (VIDEO_FRAME_INFO_S *)frame);
}
