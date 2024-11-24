#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include "cvi_buffer.h"

#include "sample_comm.h"
#ifdef __cplusplus
#include "sophgo_middleware.hpp"
#else
#include "maix_mmf.h"
#endif
#include "kvm_vision.h"

// -------------------- mmf locals begin --------------------

typedef struct {
	uint8_t type;
	uint32_t width;
	uint32_t height;
	uint32_t qlty;
	uint8_t gop;
	uint32_t fps;
	uint32_t bitrate;
	uint32_t res;
} kvm_cfg_t;

static kvm_cfg_t kvm_cfg;

typedef struct {
	uint64_t last_update;
	int now_fps;
	int state;
} kvm_state_t;

static kvm_state_t kvm_state;

typedef struct {
	int img_w;
	int img_h;
	int img_fps;
	int img_fmt;
	int img_qlty;
	int enc_ch;
	int vi_ch;
	uint8_t *filebuf;
} kvm_mmf_t;

static kvm_mmf_t kvm_mmf;

static kvm_cfg_t vis_cfg = {
	.type = 0,
	.width = 0,
	.height = 0,
	.qlty = 0,
	.gop = 0,
	.fps = 0,
	.bitrate = 0,
	.res = 0,
};

#define OUT_BUF_LEN           1024 * 1024 /* 1MB */

typedef struct {
	char out_data[OUT_BUF_LEN];
	int size;
	int offset;
	uint8_t new_gop;
	unsigned char *p_sps_data, *p_pps_data;
	int sps_size, pps_size;

	uint64_t start;
	uint64_t last_loop_us;
	uint64_t timestamp;
	int last_vi_pop;
	int kvm_is_init;
	int chn_is_init;
	int is_running;
} priv_t;

static priv_t priv = { {0}, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// -------------------- mmf locals end   --------------------

// #define DEBUG_EN
#ifdef DEBUG_EN
#define DEBUG(fmt, args...) printf("[%s][%d]: "fmt, __func__, __LINE__, ##args)
#else
#define DEBUG(fmt, args...)
#endif

#define INFO printf

#define MIN(a, b)                       ((a) < (b) ? (a) : (b))
#define MAX(a, b)                       ((a) > (b) ? (a) : (b))

#define USE_H264_ITERATE

// -------------------- mmf helpers begin --------------------

static uint64_t _get_time_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec + tv.tv_sec * 1000000;
}

static void _rgb888_to_nv21(uint8_t* data, Uint32 w, Uint32 h, uint8_t* yuv)
{
    Uint32 row_bytes;
    uint8_t* uv;
    uint8_t* y;
    uint8_t r, g, b;
    uint8_t y_val, u_val, v_val;

    uint8_t * img;
    Uint32 i, j;
    y = yuv;
    uv = yuv + w * h;

    row_bytes = (w * 3 );
    h = h & ~1;
    //先转换Y
    for (i = 0; i < h; i++)
    {
	img = data + row_bytes * i;
	for (j = 0; j <w; j++)
	{
	    r = *(img+3*j);
	    g = *(img+3*j+1);
	    b = *(img+3*j+2);
	    if(r>=254&&g>=254&&b>=254)
	    {
		y_val=254;
		*y++ = y_val;
		continue;
	    }
	    y_val = (uint8_t)(((int)(299 * r) + (int)(597 * g) + (int)(114 * b)) / 1000);
	    *y++ = y_val;
	}
    }
    //转换uv
    for (i = 0; i <h; i += 2)
    {
	img = data + row_bytes * i;
	for (j = 0; j < w; j+=2)
	{
	    r = *(img+3*j);
	    g = *(img+3*j+1);
	    b = *(img+3*j+2);
	    u_val= (uint8_t)(((int)(-168.7 * r) - (int)(331.3 * g) + (int)(500 * b) + 128000) / 1000);
	    v_val= (uint8_t)(((int)(500 * r) - (int)(418.7 * g) - (int)(81.3 * b) + 128000) / 1000);
	    *uv++ = v_val;
	    *uv++ = u_val;
	}
   }
}

static uint8_t *_prepare_image(int width, int height, int format)
{
	switch (format) {
	case PIXEL_FORMAT_RGB_888:
	{
		uint8_t *rgb_data = (uint8_t *)malloc(width * height * 3);
		int x_oft = 0;
		int remain_width = width;
		int segment_width = width / 6;
		int idx = 0;
		while (remain_width > 0) {
			int seg_w = (remain_width > segment_width) ? segment_width : remain_width;
			uint8_t r,g,b;
			switch (idx) {
			case 0: r = 0xff, g = 0x00, b = 0x00; break;
			case 1: r = 0x00, g = 0xff, b = 0x00; break;
			case 2: r = 0x00, g = 0x00, b = 0xff; break;
			case 3: r = 0xff, g = 0xff, b = 0x00; break;
			case 4: r = 0xff, g = 0x00, b = 0xff; break;
			case 5: r = 0x00, g = 0xff, b = 0xff; break;
			default: r = 0x00, g = 0x00, b = 0x00; break;
			}
			idx ++;
			for (int i = 0; i < height; i ++) {
				for (int j = 0; j < seg_w; j ++) {
					rgb_data[(i * width + x_oft + j) * 3 + 0] = r;
					rgb_data[(i * width + x_oft + j) * 3 + 1] = g;
					rgb_data[(i * width + x_oft + j) * 3 + 2] = b;
				}
			}
			x_oft += seg_w;
			remain_width -= seg_w;
		}

		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}
		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i + width - height) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}

		return rgb_data;
	}
	case PIXEL_FORMAT_ARGB_8888:
	{
		uint8_t *rgb_data = (uint8_t *)malloc(width * height * 4);
		memset(rgb_data, 0x00, width * height * 4);
		int x_oft = 0;
		int remain_width = width;
		int segment_width = width / 6;
		int idx = 0;
		while (remain_width > 0) {
			int seg_w = (remain_width > segment_width) ? segment_width : remain_width;
			uint8_t r,g,b,a;
			switch (idx) {
			case 0: r = 0xff, g = 0x00, b = 0x00; a = 0x10; break;
			case 1: r = 0x00, g = 0xff, b = 0x00; a = 0x20; break;
			case 2: r = 0x00, g = 0x00, b = 0xff; a = 0x40; break;
			case 3: r = 0xff, g = 0xff, b = 0x00; a = 0x60; break;
			case 4: r = 0xff, g = 0x00, b = 0xff; a = 0x80; break;
			case 5: r = 0x00, g = 0xff, b = 0xff; a = 0xA0; break;
			default: r = 0x00, g = 0x00, b = 0x00; a = 0xC0; break;
			}
			idx ++;
			for (int i = 0; i < height; i ++) {
				for (int j = 0; j < seg_w; j ++) {
					rgb_data[(i * width + x_oft + j) * 4 + 0] = r;
					rgb_data[(i * width + x_oft + j) * 4 + 1] = g;
					rgb_data[(i * width + x_oft + j) * 4 + 2] = b;
					rgb_data[(i * width + x_oft + j) * 4 + 3] = a;
				}
			}
			x_oft += seg_w;
			remain_width -= seg_w;
		}

		// for (int i = 0; i < height; i ++) {
		// 	uint8_t *buff = &rgb_data[(i * width + i) * 4];
		// 	buff[0] = 0xff;
		// 	buff[1] = 0xff;
		// 	buff[2] = 0xff;
		// }
		// for (int i = 0; i < height; i ++) {
		// 	uint8_t *buff = &rgb_data[(i * width + i + width - height) * 4];
		// 	buff[0] = 0xff;
		// 	buff[1] = 0xff;
		// 	buff[2] = 0xff;
		// }

		return rgb_data;
	}
	case PIXEL_FORMAT_NV21:
	{
		uint8_t *rgb_data = _prepare_image(width, height, PIXEL_FORMAT_RGB_888);
		uint8_t *nv21 = (uint8_t *)malloc(width * height * 3 / 2);
		_rgb888_to_nv21(rgb_data, width, height, nv21);
		free(rgb_data);
		return nv21;
	}
	break;
	default:
		DEBUG("Only support PIXEL_FORMAT_RGB_888\r\n");
		break;
	}
	return NULL;
}

static inline int is_h26x_stream(uint8_t type)
{
	return (type == 1 || type == 2);
}

int kvm_stream_venc_init(int ch, int w, int h, int fmt, int qlty)
{
	int cfg_qlty = is_h26x_stream(kvm_cfg.type) ? 0 : qlty;
	int cfg_gop = is_h26x_stream(kvm_cfg.type) ? kvm_cfg.gop : 0;
	int cfg_bitrate = kvm_cfg.bitrate;
	mmf_venc_cfg_t cfg = {
		.type = kvm_cfg.type,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = fmt,
		.jpg_quality = (uint8_t)cfg_qlty,
		.gop = cfg_gop,
		.intput_fps = 60,
		.output_fps = 60,
		.bitrate = cfg_bitrate,
	};

	return mmf_add_venc_channel(ch, &cfg);
}

#if 0
int kvm_stream_venc_init(int ch, int w, int h, int fmt, int qlty)
{
	return mmf_enc_jpg_init(ch, w, h, fmt, qlty);
}
#endif

static char* file_to_string(const char *file, size_t max_len)
{
	char *m_ptr = NULL;
	size_t m_capacity = 0;
	FILE* fp = fopen(file, "rb");

	if(fp) {
		fseek(fp, 0, SEEK_END);
		m_capacity = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (max_len && m_capacity > max_len) {
			m_capacity = max_len;
		}
		if (m_capacity) {
			m_ptr = (char*)malloc(m_capacity+1);
		}
		if (m_ptr) {
			fread(m_ptr, 1, m_capacity, fp);
			m_ptr[m_capacity] = 0;
		}

		fclose(fp);
	}

	if (m_ptr) {
	        uint8_t j=0;
	        while (m_ptr[j] != '\0' && m_ptr[j] != '\r' && m_ptr[j] != '\n')
			j++;
		m_ptr[j] = 0;
	}

	return m_ptr;
}

static uint32_t file_to_uint(const char *file, uint32_t def)
{
	uint32_t ret = def;
	char* str = file_to_string(file, 32);
	if (str)
	{
		if (sscanf(str, "%u", &ret) != 1)
			ret = def;
		free(str);
	}
	return ret;
}

static int string_to_file(const char *file, char * str)
{
	char *m_ptr = str;
	size_t m_capacity;
	FILE* fp = fopen(file, "wb+");

	if(fp) {
		if (m_ptr) {
			m_capacity = strlen(m_ptr);
			fwrite(m_ptr, 1, m_capacity, fp);
		}

		fclose(fp);
		return 0;
	}

	return -1;
}

static int uint_to_file(const char *file, uint32_t val)
{
	char str[32];
	int len = snprintf(str, sizeof(str), "%u\n", val);
	if (len > 0)
		return string_to_file(file, str);
	return -1;
}

int kvm_cfg_read(void)
{
	kvm_cfg_t new_cfg;
	int changed;
	char *str_type;

	memset(&new_cfg, 0, sizeof(new_cfg));
	new_cfg.type = 4;
	str_type = file_to_string("/kvmapp/kvm/type", 32);
	if (str_type) {
		new_cfg.type = strcmp(str_type, "h264") ? new_cfg.type : 2;
		free(str_type);
	}
	new_cfg.gop = priv.new_gop;
	new_cfg.width = file_to_uint("/kvmapp/kvm/width", 1920);
	new_cfg.height = file_to_uint("/kvmapp/kvm/height", 1080);
	new_cfg.fps = file_to_uint("/kvmapp/kvm/fps", 30);
	new_cfg.qlty = file_to_uint("/kvmapp/kvm/qlty", 80);
	new_cfg.res = file_to_uint("/kvmapp/kvm/res", 720);
	if (is_h26x_stream(new_cfg.type) && new_cfg.qlty >= 500) {
		new_cfg.bitrate = new_cfg.qlty;
		new_cfg.qlty = kvm_cfg.qlty;
	} else {
		new_cfg.bitrate = kvm_cfg.bitrate;
	}

	changed = memcmp(&new_cfg, &kvm_cfg, sizeof(kvm_cfg)) || (!priv.chn_is_init);
	if (!changed)
		return changed;

	if (new_cfg.type != kvm_cfg.type)
		printf("kvm_cfg.type = %u\n", new_cfg.type);
	if (new_cfg.width != kvm_cfg.width)
		printf("kvm_cfg.width = %u\n", new_cfg.width);
	if (new_cfg.height != kvm_cfg.height)
		printf("kvm_cfg.height = %u\n", new_cfg.height);
	if (new_cfg.fps != kvm_cfg.fps)
		printf("kvm_cfg.fps = %u\n", new_cfg.fps);
	if (new_cfg.qlty != kvm_cfg.qlty)
		printf("kvm_cfg.qlty = %u\n", new_cfg.qlty);
	if (new_cfg.bitrate != kvm_cfg.bitrate)
		printf("kvm_cfg.bitrate = %u\n", new_cfg.bitrate);
	if (new_cfg.res != kvm_cfg.res)
		printf("kvm_cfg.res = %u\n", new_cfg.res);

	memcpy(&kvm_cfg, &new_cfg, sizeof(kvm_cfg));

	return changed;
}

int kvm_write_now_fps(double now_frame_int)
{
	int now_fps = 0;
	uint64_t now_update = _get_time_us();
	int changed;

        if (now_frame_int > 0)
		now_fps = (100 / now_frame_int + 15) / 100;

	changed = now_fps != kvm_state.now_fps;
	if (!changed)
		return changed;

	if ((now_update - kvm_state.last_update) < 1000 * 1000)
		return 0;
	if (MAX(now_fps, kvm_state.now_fps) - MIN(now_fps, kvm_state.now_fps) < 2)
		return 0;

	DEBUG("kvm_state.now_fps = %d\n", now_fps);
	kvm_state.now_fps = now_fps;
	kvm_state.last_update = now_update;

	uint_to_file("/kvmapp/kvm/now_fps", kvm_state.now_fps);

	return changed;
}

int kvm_write_state(int state)
{
	int changed = state != kvm_state.state;

	if (!changed)
		return changed;

	printf("kvm_state.state = %d\n", state);
	kvm_state.state = state;

	uint_to_file("/kvmapp/kvm/state", kvm_state.state);

	return changed;
}

int kvm_deinit_mmf_channels(kvm_mmf_t *mmf_cfg)
{
	if (!priv.chn_is_init)
		return 0;

	free_all_kvmv_data();

	if (mmf_del_venc_channel(mmf_cfg->enc_ch)) {
		printf("mmf_del_venc_channel failed\n");
		return -1;
	}

	if (mmf_cfg->filebuf) {
		free(mmf_cfg->filebuf);
		mmf_cfg->filebuf = NULL;
	}

	mmf_del_vi_channel(mmf_cfg->vi_ch);

	priv.chn_is_init = 0;

	return 0;
}

int kvm_init_mmf_channels(kvm_mmf_t *mmf_cfg)
{
	if (priv.chn_is_init)
		return 0;

	mmf_cfg->img_w = 2560; mmf_cfg->img_h = 1440; mmf_cfg->img_fps = 30;
	mmf_cfg->img_fmt = PIXEL_FORMAT_NV21; mmf_cfg->img_qlty = 80;

	char *sensor_name = mmf_get_sensor_name();
	if (!strcmp(sensor_name, "lt6911")) {
		mmf_cfg->img_w = 1280; mmf_cfg->img_h = 720; mmf_cfg->img_fps = 60;
	}

	if (kvm_cfg.res == 1440) {
		mmf_cfg->img_w = 2560; mmf_cfg->img_h = 1440;
	}
	if (kvm_cfg.res == 1080) {
		mmf_cfg->img_w = 1920; mmf_cfg->img_h = 1080;
	}
	if (kvm_cfg.res == 720) {
		mmf_cfg->img_w = 1280; mmf_cfg->img_h = 720;
	}
	if (kvm_cfg.res == 600) {
		mmf_cfg->img_w =  800; mmf_cfg->img_h = 600;
	}
	if (kvm_cfg.res == 480) {
		mmf_cfg->img_w = 640; mmf_cfg->img_h = 480;
	}
	mmf_cfg->img_fps = kvm_cfg.fps > 60 ? mmf_cfg->img_fps : (int32_t)kvm_cfg.fps;
	mmf_cfg->img_qlty = (kvm_cfg.qlty < 50 || kvm_cfg.qlty > 100) ? mmf_cfg->img_qlty : (int32_t)kvm_cfg.qlty;

	mmf_cfg->vi_ch = mmf_get_vi_unused_channel();
	int vi_al = mmf_vi_aligned_width(mmf_cfg->vi_ch);
	if (vi_al > 0)
		mmf_cfg->img_w = ALIGN(mmf_cfg->img_w, vi_al);

	mmf_cfg->enc_ch = mmf_venc_unused_channel();
	if (kvm_stream_venc_init(mmf_cfg->enc_ch, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt, mmf_cfg->img_qlty)) {
		printf("kvm_stream_venc_init failed\n");
		return -1;
	}

	mmf_cfg->filebuf = _prepare_image(mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt);

	mmf_cfg->vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel_v2(mmf_cfg->vi_ch, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt, mmf_cfg->img_fps, 2, !true, !true, 2, 3)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		return -1;
	}

	mmf_vi_set_pop_timeout(100);

	priv.chn_is_init = 1;

	return 0;
}

int kvm_reset_mmf_channels(kvm_mmf_t *mmf_cfg)
{
	int ret = kvm_deinit_mmf_channels(mmf_cfg);
	if (ret)
		return ret;
	return kvm_init_mmf_channels(mmf_cfg);
}

// -------------------- mmf helpers end   --------------------

// -------------------- h264 helpers begin --------------------

static int _kvmv_h264_get_one_nalu(unsigned char *p_in_data, int in_size, unsigned char **pp_nalu, int *p_nalu_size)
{
	unsigned char *p = p_in_data;
	int start_pos = 0, end_pos = 0;

	if (in_size < 4)
		return 0;

	while (1)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
		{
			start_pos = p - p_in_data;
			break;
		}
		p++;
		if (p - p_in_data >= in_size - 4)
			return 0;
	}
	p++;
	while (1)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
		{
			end_pos = p - p_in_data;
			break;
		}
		p++;
		if (p - p_in_data >= in_size - 4)
		{
			end_pos = in_size;
			break;
		}
	}
	*p_nalu_size = end_pos - start_pos;
	*pp_nalu = p_in_data + start_pos;

	return 1;
}

static int _kvmv_h264_scan_one_nalu(char *p_nalu, int nalu_size)
{
	if (p_nalu == NULL || nalu_size <= 4)
		return IMG_NOT_EXIST;

	int nalu_type = p_nalu[4] & 0x1f;

	if (nalu_type == 0x07)
	{
		if (priv.p_sps_data != NULL) free(priv.p_sps_data);
		priv.p_sps_data = (unsigned char*)malloc(nalu_size);
		priv.sps_size = nalu_size;
		memcpy(priv.p_sps_data, p_nalu, nalu_size);

		return IMG_H264_TYPE_SPS;
	}
	if (nalu_type == 0x08)
	{
		if (priv.p_pps_data != NULL) free(priv.p_pps_data);
		priv.p_pps_data = (unsigned char*)malloc(nalu_size);
		priv.pps_size = nalu_size;
		memcpy(priv.p_pps_data, p_nalu, nalu_size);

		return IMG_H264_TYPE_PPS;
	}
	if (nalu_type == 0x05)
	{
		return IMG_H264_TYPE_IF;
	}

	return IMG_H264_TYPE_PF;
}

#ifdef USE_H264_ITERATE
static int _kvmv_h264_iterate(uint8_t **nalu, uint32_t *size)
{
	unsigned char *p_one_nalu_data = NULL;
	int one_nalu_size = 0;
	uint8_t *p_it_data = (uint8_t *)&priv.out_data;
	uint32_t it_data_size = priv.size;

	if (p_it_data == NULL || (uint32_t)priv.offset + 4 >= it_data_size) {
		return IMG_NOT_EXIST;
	}

	if (_kvmv_h264_get_one_nalu(p_it_data + priv.offset, it_data_size - priv.offset, &p_one_nalu_data, &one_nalu_size) == 0) {
		return IMG_NOT_EXIST;
	}
	if (p_one_nalu_data == NULL || one_nalu_size < 1) {
		return IMG_NOT_EXIST;
	}

	int ret = _kvmv_h264_scan_one_nalu((char *)p_one_nalu_data, one_nalu_size);
	if (ret == IMG_NOT_EXIST) {
		return IMG_NOT_EXIST;
	}

	priv.offset += one_nalu_size;

	if (nalu) {
		*nalu = p_one_nalu_data;
	}
	if (size) {
		*size = one_nalu_size;
	}

	return ret;
}

#else
static int _kvmv_h264_scan(void *nalu, int nalu_size)
{
	int offset = 0;
	int got_sps = 0;
	int got_pps = 0;
	int got_idr = 0;

	if (nalu == NULL || nalu_size == 0)
	{
		return IMG_NOT_EXIST;
	}

	while (1)
	{
		unsigned char *p_one_nalu_data = NULL;
		int one_nalu_size = 0;
		if (_kvmv_h264_get_one_nalu((uint8_t *)nalu + offset, nalu_size - offset, &p_one_nalu_data, &one_nalu_size) == 0)
			break;

		switch (_kvmv_h264_scan_one_nalu((char *)p_one_nalu_data, one_nalu_size))
		{
			case IMG_H264_TYPE_SPS:
				got_sps = 1;
				break;
			case IMG_H264_TYPE_PPS:
				got_pps = 1;
				break;
			case IMG_H264_TYPE_IF:
				got_idr = 1;
			default:
				break;
		}

		offset += one_nalu_size;
		if (offset + 4 >= nalu_size)
			break;
	}

	return got_idr ? IMG_H264_TYPE_IF: IMG_H264_TYPE_PF;
}
#endif

// -------------------- h264 helpers end   --------------------

static int _kvmv_init(uint8_t _debug_info_en)
{
	UNUSED(_debug_info_en);

	if (priv.kvm_is_init) {
		return 0;
	}

	priv.offset = 0;
	priv.size = 0;
	priv.new_gop = 30;
	priv.p_sps_data = NULL;
	priv.p_pps_data = NULL;
	priv.sps_size = 0;
	priv.pps_size = 0;

	// -------------------- mmf init begin --------------------
	if (0 != mmf_init()) {
		printf("mmf deinit\n");
		return 0;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	memset(&kvm_cfg, 0, sizeof(kvm_cfg));
	kvm_cfg.qlty = 80;
	kvm_cfg.bitrate = 3000;
	kvm_cfg_read();

	memset(&kvm_mmf, 0, sizeof(kvm_mmf));
	priv.chn_is_init = 0;
	priv.is_running = 0;

	memset(&kvm_state, 0, sizeof(kvm_state));
	kvm_write_now_fps(0);
	kvm_write_state(1);

	priv.start = _get_time_us();
	priv.last_loop_us = priv.start;
	priv.timestamp = 0;
	priv.last_vi_pop = -1;
	// -------------------- mmf init end   --------------------

	priv.kvm_is_init = 1;
	return 0;
}

void kvmv_init(uint8_t _debug_info_en)
{
	_kvmv_init(_debug_info_en);
}

static int _kvmv_read_cfg(uint16_t _width, uint16_t _height, uint8_t _type, uint16_t _qlty_bitrate)
{
	uint16_t _qlty = vis_cfg.qlty;
	uint16_t _bitrate = vis_cfg.bitrate;

	if (is_h26x_stream(_type) && _qlty_bitrate >= 500) {
		_bitrate = _qlty_bitrate;
	} else {
		_qlty = _qlty_bitrate;
	}

	if (_type != vis_cfg.type)
		printf("vis_cfg.type = %u\n", _type);
	if (_width != vis_cfg.width)
		printf("vis_cfg.width = %u\n", _width);
	if (_height != vis_cfg.height)
		printf("vis_cfg.height = %u\n", _height);
	if (_qlty != vis_cfg.qlty)
		printf("vis_cfg.qlty = %u\n", _qlty);
	if (_bitrate != vis_cfg.bitrate)
		printf("vis_cfg.bitrate = %u\n", _bitrate);

	vis_cfg.width = _width;
	vis_cfg.height = _height;
	vis_cfg.type = _type;
	vis_cfg.qlty = _qlty;
	vis_cfg.bitrate = _bitrate;

	return 0;
}

int kvmv_read_img(uint16_t _width, uint16_t _height, uint8_t _type, uint16_t _qlty, uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	// -------------------- mmf loop begin --------------------
	kvm_mmf_t *mmf_cfg = &kvm_mmf;
	{
		void *data;
		int data_size, width, height, format;

		if (priv.is_running) {
			return IMG_NOT_EXIST;
		}

		priv.is_running = 1;

		_kvmv_read_cfg(_width, _height, _type, _qlty);

		if (kvm_cfg_read()) {
			if (0 != kvm_reset_mmf_channels(mmf_cfg)) {
				priv.is_running = 0;
				return IMG_VENC_ERROR;
			}
		}

#ifdef USE_H264_ITERATE
		if (is_h26x_stream(kvm_cfg.type)) {
			int ret = _kvmv_h264_iterate(_pp_kvm_data, _p_kvmv_data_size);
			if (ret != IMG_NOT_EXIST) {
				priv.is_running = 0;
				return ret;
			}
		}
#endif

		priv.start = _get_time_us();
		int vi_ret = mmf_vi_frame_pop(mmf_cfg->vi_ch, &data, &data_size, &width, &height, &format);
		if (vi_ret != priv.last_vi_pop) {
			uint64_t vi_stamp = priv.timestamp;
			vi_stamp += (_get_time_us() - priv.last_loop_us) / 1000;
			printf("[%.6ld.%.3ld] %s\n", vi_stamp / 1000, vi_stamp % 1000,
				vi_ret ? "no input signal" : "got input signal");
			mmf_del_venc_channel(mmf_cfg->enc_ch);
			kvm_stream_venc_init(mmf_cfg->enc_ch, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt, mmf_cfg->img_qlty);
			//kvm_write_state(vi_ret ? 0 : 1);
			priv.last_vi_pop = vi_ret;
		}
		if (vi_ret)
			data = mmf_cfg->filebuf;
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		if (mmf_venc_push(mmf_cfg->enc_ch, (uint8_t*)data, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt)) {
			printf("mmf_venc_push failed\n");
			priv.is_running = 0;
			return IMG_VENC_ERROR;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		mmf_stream_t stream;
		stream.count = 0;
		if (mmf_venc_pop(mmf_cfg->enc_ch, &stream)) {
			printf("mmf_venc_pop failed\n");
			priv.is_running = 0;
			return IMG_VENC_ERROR;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		if (stream.count > 0) {
			int stream_size = 0;
			for (int i = 0; i < stream.count; i ++) {
				DEBUG("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
				stream_size += stream.data_size[i];
			}

			if (OUT_BUF_LEN >= stream_size) {
				int copy_length = 0;
				for (int i = 0; i < stream.count; i ++) {
					memcpy((uint8_t *)&priv.out_data + copy_length, stream.data[i], stream.data_size[i]);
					copy_length += stream.data_size[i];
				}
				priv.size = copy_length;
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		if (mmf_venc_free(mmf_cfg->enc_ch)) {
			printf("mmf_venc_free failed\n");
			priv.is_running = 0;
			return IMG_VENC_ERROR;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		if (!priv.last_vi_pop) {
			priv.start = _get_time_us();
			mmf_vi_frame_free(mmf_cfg->vi_ch);
			DEBUG("use %ld us\r\n", _get_time_us() - priv.start);
		}

		DEBUG("use %ld us\r\n", _get_time_us() - priv.last_loop_us);
		priv.timestamp += (_get_time_us() - priv.last_loop_us) / 1000;
		priv.last_loop_us = _get_time_us();
	}
	// -------------------- mmf loop end   --------------------

	priv.offset = 0;
#ifdef USE_H264_ITERATE
	if (is_h26x_stream(kvm_cfg.type)) {
		int ret = _kvmv_h264_iterate(_pp_kvm_data, _p_kvmv_data_size);
		priv.is_running = 0;
		return ret;
	}
#endif

	if (_pp_kvm_data)
		*_pp_kvm_data = (uint8_t*)&priv.out_data;
	if (_p_kvmv_data_size)
		*_p_kvmv_data_size = priv.size;

#ifndef USE_H264_ITERATE
	if (is_h26x_stream(kvm_cfg.type)) {
		int ret = _kvmv_h264_scan(&priv.out_data, priv.size);
		priv.is_running = 0;
		return ret;
	}
#endif

	priv.is_running = 0;
	return IMG_MJPEG_TYPE;
}

int kvmv_get_sps_frame(uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	if (_pp_kvm_data)
		*_pp_kvm_data = priv.p_sps_data;
	if (_p_kvmv_data_size)
		*_p_kvmv_data_size = priv.sps_size;

	return 0;
}

int kvmv_get_pps_frame(uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	if (_pp_kvm_data)
		*_pp_kvm_data = priv.p_pps_data;
	if (_p_kvmv_data_size)
		*_p_kvmv_data_size = priv.pps_size;

	return 0;
}

int free_kvmv_data(uint8_t ** _pp_kvm_data)
{
	uint8_t *p_out_data = (uint8_t*)&priv.out_data;

	if (!_pp_kvm_data)
		return 0;
	if (!*_pp_kvm_data)
		return 0;

	if (*_pp_kvm_data >= p_out_data && *_pp_kvm_data < (p_out_data + OUT_BUF_LEN))
	{
		// no need to free static buffer
		// free must be added if switching to dynamic allocation
		*_pp_kvm_data = NULL;
	}

	return 0;
}

void free_all_kvmv_data()
{
	priv.offset = 0;
	priv.size = 0;

	if (priv.p_sps_data != NULL) free(priv.p_sps_data);
	if (priv.p_pps_data != NULL) free(priv.p_pps_data);

	priv.p_sps_data = NULL;
	priv.p_pps_data = NULL;
	priv.sps_size = 0;
	priv.pps_size = 0;
}

void set_h264_gop(uint8_t _gop)
{
	priv.new_gop = _gop;
}

void kvmv_deinit()
{
	//  ------------------- mmf deinit begin --------------------
	kvm_mmf_t *mmf_cfg = &kvm_mmf;

	if (!priv.kvm_is_init) {
		return;
	}
	if (priv.is_running) {
		return;
	}

	kvm_write_state(0);
	kvm_write_now_fps(0);

	if (0 != kvm_deinit_mmf_channels(mmf_cfg)) {
		return;
	}

	mmf_vi_deinit();

	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	// -------------------- mmf deinit end ----------------------

	priv.kvm_is_init = 0;
	INFO("bye!");
}
