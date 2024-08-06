#ifndef _MIPI_TX_PARAM_D240SI31_H_
#define _MIPI_TX_PARAM_D240SI31_H_

#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

/*
#define D240SI31_VACT	854
#define D240SI31_VSA		2
#define D240SI31_VBP		20
#define D240SI31_VFP		20

#define D240SI31_HACT	480
#define D240SI31_HSA		10
#define D240SI31_HBP		40
#define D240SI31_HFP		40
*/

#define D240SI31_VACT		640
#define D240SI31_VSA		10
#define D240SI31_VBP		12
#define D240SI31_VFP		3

#define D240SI31_HACT		480
#define D240SI31_HSA		48
#define D240SI31_HBP		64
#define D240SI31_HFP		16

#define DXQ_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_d240si31 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = D240SI31_HSA,
		.vid_hbp_pixels = D240SI31_HBP,
		.vid_hfp_pixels = D240SI31_HFP,
		.vid_hline_pixels = D240SI31_HACT,
		.vid_vsa_lines = D240SI31_VSA,
		.vid_vbp_lines = D240SI31_VBP,
		.vid_vfp_lines = D240SI31_VFP,
		.vid_active_lines = D240SI31_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = DXQ_PIXEL_CLK(D240SI31),
};

const struct hs_settle_s hs_timing_cfg_d240si31 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_d240si31_0[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x13};
static CVI_U8 data_d240si31_1[] = {0xef, 0x08};
static CVI_U8 data_d240si31_2[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x10};
static CVI_U8 data_d240si31_3[] = {0xc0, 0x4f, 0x00};
static CVI_U8 data_d240si31_4[] = {0xc1, 0x0d, 0x12};
static CVI_U8 data_d240si31_5[] = {0xc2, 0x00, 0x0A};
static CVI_U8 data_d240si31_6[] = {0xcc, 0x10};
static CVI_U8 data_d240si31_7[] = {0xb0, 0x00, 0x0D, 0x0B, 0x08, 0x0e, 0x07, 0x00, 0x07, 0x07, 0x16, 0x07, 0x10, 0x13, 0x1b, 0x29, 0x1f};
static CVI_U8 data_d240si31_8[] = {0xb1, 0x00, 0x01, 0x15, 0x12, 0x12, 0x02, 0x00, 0x08, 0x08, 0x21, 0x03, 0x14, 0x0f, 0x2d, 0x2e, 0x1f};
static CVI_U8 data_d240si31_9[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x11};
static CVI_U8 data_d240si31_10[] = {0xb0, 0x6d};
static CVI_U8 data_d240si31_11[] = {0xb1, 0x4d};
static CVI_U8 data_d240si31_12[] = {0xb2, 0x87};
static CVI_U8 data_d240si31_13[] = {0xb3, 0x80};
static CVI_U8 data_d240si31_14[] = {0xb5, 0x49};
static CVI_U8 data_d240si31_15[] = {0xb7, 0x87};
static CVI_U8 data_d240si31_16[] = {0xb8, 0x23};
static CVI_U8 data_d240si31_17[] = {0xc1, 0x78};
static CVI_U8 data_d240si31_18[] = {0xc2, 0x78};
static CVI_U8 data_d240si31_19[] = {0xd0, 0x88};
//delay 100 ms
static CVI_U8 data_d240si31_20[] = {0xe0, 0x00, 0x00, 0x00, 0x00};
static CVI_U8 data_d240si31_21[] = {0xe1, 0x06, 0xa8, 0x08, 0xa8, 0x05, 0xa8, 0x07, 0xa8, 0x00, 0x44, 0x44};
static CVI_U8 data_d240si31_22[] = {0xe2, 0x22, 0x22, 0x44, 0x44, 0x8b, 0xa8, 0x8d, 0xa8, 0x8a, 0xa8, 0x8c, 0xa8, 0x00};
static CVI_U8 data_d240si31_23[] = {0xe3, 0x00, 0x00, 0x22, 0x22};
static CVI_U8 data_d240si31_24[] = {0xe4, 0x44, 0x44};
static CVI_U8 data_d240si31_25[] = {0xe5, 0x08, 0x8c, 0xd1, 0xd9, 0x0a, 0x8e, 0xd1, 0xd9, 0x0c, 0x90, 0xd1, 0xd9, 0x0e, 0x93, 0xd1, 0xd9};
static CVI_U8 data_d240si31_26[] = {0xe6, 0x00, 0x00, 0x22, 0x22};
static CVI_U8 data_d240si31_27[] = {0xe7, 0x44, 0x44};
static CVI_U8 data_d240si31_28[] = {0xe8, 0x07, 0x8b, 0xd1, 0xd9, 0x09, 0x8d, 0xd1, 0xd9, 0x0b, 0x8f, 0xd1, 0xd9, 0x0d, 0x91, 0xd1, 0xd9};
static CVI_U8 data_d240si31_29[] = {0xe9, 0x36, 0x00};
static CVI_U8 data_d240si31_30[] = {0xeb, 0x00, 0x00, 0x4e, 0x4e, 0xee, 0x44, 0x40};
static CVI_U8 data_d240si31_31[] = {0xed, 0xff, 0xff, 0x32, 0xbf, 0x01, 0xc4, 0x56, 0x7F, 0x76, 0x54, 0xc1, 0x0f, 0xb2, 0x3f, 0xff, 0xff};
static CVI_U8 data_d240si31_32[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x00};
static CVI_U8 data_d240si31_33[] = {0x11};
//delay 120 ms
//彩条
// static CVI_U8 data_d240si31_37[] = {0xFF,0x77,0x01,0x00,0x00,0x12};
// static CVI_U8 data_d240si31_38[] = {0xd1,0x81,0x10,0x03,0x03,0x08,0x01,0xA0,0x01,0xe0,0xB0,0x01,0xe0,0x03,0x20};
// static CVI_U8 data_d240si31_39[] = {0xd2,0x08};///彩条
// static CVI_U8 data_d240si31_40[] = {0xFF,0x77,0x01,0x00,0x00,0x10};
// static CVI_U8 data_d240si31_41[] = {0xC2,0x31,0x08};
//delay 120 ms

static CVI_U8 data_d240si31_34[] = {0x35, 0x00};
static CVI_U8 data_d240si31_35[] = {0x36, 0x00};
static CVI_U8 data_d240si31_36[] = {0x29};
//delay 30 ms

// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29
const struct dsc_instr dsi_init_cmds_d240si31[] = {
	{.delay = 60, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_0 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_1 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_2 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_3 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_4 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_5 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_6 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_d240si31_7 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_d240si31_8 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_9 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_10 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_11 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_12 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_13 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_14 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_15 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_16 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_17 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_18 },
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_19 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_d240si31_20 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 12, .data = data_d240si31_21 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 14, .data = data_d240si31_22 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_d240si31_23 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_24 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_d240si31_25 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_d240si31_26 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_27 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_d240si31_28 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_29 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 8, .data = data_d240si31_30 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_d240si31_31 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_32 },
	{.delay = 120, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_d240si31_33 },

	// {.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_37 },
	// {.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 15, .data = data_d240si31_38 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_39 },
	// {.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_d240si31_40 },
	// {.delay = 120, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_d240si31_41 },

	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_34 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_d240si31_35 },
	{.delay = 30, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_d240si31_36 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif