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

#define STREAM_SERVER_TYPE 4

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	uint32_t qlty;
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

#define INPUT_BUF_LEN           1024 * 1024 /* 1MB */

typedef struct {
	char out_data[INPUT_BUF_LEN];
	int size;

	uint64_t start;
	uint64_t last_loop_us;
	uint64_t timestamp;
	uint64_t loop_count;
	int last_vi_pop;
	int last_size;
} priv_t;

static priv_t priv;

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

#if 0
int kvm_stream_venc_init(int ch, int w, int h, int fmt, int qlty)
{
	mmf_venc_cfg_t cfg = {
		.type = STREAM_SERVER_TYPE,  //1, h265, 2, h264, 3, mjpeg, 4, jpeg
		.w = w,
		.h = h,
		.fmt = fmt,
		.jpg_quality = qlty,
		.gop = 0,	// unused
		.intput_fps = 30,
		.output_fps = 30,
		.bitrate = 3000,
	};

	return mmf_add_venc_channel(ch, &cfg);
}
#endif

int kvm_stream_venc_init(int ch, int w, int h, int fmt, int qlty)
{
	return mmf_enc_jpg_init(ch, w, h, fmt, qlty);
}

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

	memset(&new_cfg, 0, sizeof(new_cfg));
	new_cfg.width = file_to_uint("/kvmapp/kvm/width", 1920);
	new_cfg.height = file_to_uint("/kvmapp/kvm/height", 1080);
	new_cfg.fps = file_to_uint("/kvmapp/kvm/fps", 30);
	new_cfg.qlty = file_to_uint("/kvmapp/kvm/qlty", 80);
	new_cfg.res = file_to_uint("/kvmapp/kvm/res", 720);

	changed = memcmp(&new_cfg, &kvm_cfg, sizeof(kvm_cfg));
	if (!changed)
		return changed;

	if (new_cfg.width != kvm_cfg.width)
		printf("kvm_cfg.width = %u\n", new_cfg.width);
	if (new_cfg.height != kvm_cfg.height)
		printf("kvm_cfg.height = %u\n", new_cfg.height);
	if (new_cfg.fps != kvm_cfg.fps)
		printf("kvm_cfg.fps = %u\n", new_cfg.fps);
	if (new_cfg.qlty != kvm_cfg.qlty)
		printf("kvm_cfg.qlty = %u\n", new_cfg.qlty);
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
	if (mmf_del_venc_channel(mmf_cfg->enc_ch)) {
		printf("mmf_del_venc_channel failed\n");
		return -1;
	}

	if (mmf_cfg->filebuf) {
		free(mmf_cfg->filebuf);
		mmf_cfg->filebuf = NULL;
	}

	mmf_del_vi_channel(mmf_cfg->vi_ch);

	return 0;
}

int kvm_init_mmf_channels(kvm_mmf_t *mmf_cfg)
{
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

static int _kvmv_init(uint8_t _debug_info_en)
{
	UNUSED(_debug_info_en);

	// -------------------- mmf init begin --------------------
	kvm_mmf_t *mmf_cfg = &kvm_mmf;

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
	kvm_cfg_read();

	memset(&kvm_mmf, 0, sizeof(kvm_mmf));
	if (kvm_init_mmf_channels(mmf_cfg))
		return -1;

	memset(&kvm_state, 0, sizeof(kvm_state));
	kvm_write_now_fps(0);
	kvm_write_state(1);

	priv.start = _get_time_us();
	priv.last_loop_us = priv.start;
	priv.timestamp = 0;
	priv.loop_count = 0;
	priv.last_vi_pop = -1;
	priv.last_size = 0;
	// -------------------- mmf init end   --------------------
	return 0;
}

void kvmv_init(uint8_t _debug_info_en)
{
	_kvmv_init(_debug_info_en);
}

int kvmv_read_img(uint16_t _width, uint16_t _height, uint8_t _type, uint16_t _qlty, uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	// -------------------- mmf loop begin --------------------
	kvm_mmf_t *mmf_cfg = &kvm_mmf;
	{
		void *data;
		int data_size, width, height, format;

		if (kvm_cfg_read()) {
			if (0 != kvm_reset_mmf_channels(mmf_cfg))
				return -2;
		}

		if (!priv.last_vi_pop) {
			priv.start = _get_time_us();
			mmf_vi_frame_free(mmf_cfg->vi_ch);
			DEBUG("use %ld us\r\n", _get_time_us() - priv.start);
		}

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
			return -2;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		mmf_stream_t stream;
		stream.count = 0;
		if (mmf_venc_pop(mmf_cfg->enc_ch, &stream)) {
			printf("mmf_venc_pop failed\n");
			return -2;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		priv.size = priv.last_size;
		{
			int stream_size = 0;
			for (int i = 0; i < stream.count; i ++) {
				DEBUG("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
				stream_size += stream.data_size[i];
			}

			if (stream.count > 1) {
				uint8_t *stream_buffer = (uint8_t *)malloc(stream_size);
				if (stream_buffer) {
					int copy_length = 0;
					for (int i = 0; i < stream.count; i ++) {
						memcpy(stream_buffer + copy_length, stream.data[i], stream.data_size[i]);
						copy_length += stream.data_size[i];
					}
					if (!priv.size && INPUT_BUF_LEN >= copy_length) {
						priv.last_size = copy_length;
						priv.size = priv.last_size;
						memcpy(&priv.out_data, stream_buffer, priv.size);
					}
					priv.loop_count++;
					free(stream_buffer);
				} else {
					DEBUG("malloc failed!\r\n");
				}
			} else if (stream.count == 1) {
				if (INPUT_BUF_LEN >= stream.data_size[0]) {
					priv.last_size = stream.data_size[0];
					priv.size = priv.last_size;
					memcpy(&priv.out_data, (uint8_t *)stream.data[0], priv.size);
				}
				priv.loop_count++;
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		priv.start = _get_time_us();
		if (mmf_venc_free(mmf_cfg->enc_ch)) {
			printf("mmf_venc_free failed\n");
			return -2;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - priv.start);

		DEBUG("use %ld us\r\n", _get_time_us() - priv.last_loop_us);
		priv.timestamp += (_get_time_us() - priv.last_loop_us) / 1000;
		priv.last_loop_us = _get_time_us();
	}
	// -------------------- mmf loop end   --------------------

	if (_pp_kvm_data)
		*_pp_kvm_data = (uint8_t*)&priv.out_data;
	if (_p_kvmv_data_size)
		*_p_kvmv_data_size = priv.size;

	return IMG_MJPEG_TYPE;
}

int kvmv_get_sps_frame(uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	return -1;
}

int kvmv_get_pps_frame(uint8_t** _pp_kvm_data, uint32_t* _p_kvmv_data_size)
{
	return -1;
}

int free_kvmv_data(uint8_t ** _pp_kvm_data)
{
	return 0;
}

void free_all_kvmv_data()
{
	//
}

void set_h264_gop(uint8_t _gop)
{
	//
}

void kvmv_deinit()
{
	//  ------------------- mmf deinit begin --------------------
	kvm_mmf_t *mmf_cfg = &kvm_mmf;

	kvm_write_state(0);
	kvm_write_now_fps(0);

	if (0 != kvm_deinit_mmf_channels(mmf_cfg))
		return;

	mmf_vi_deinit();

	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	// -------------------- mmf deinit end ----------------------

	INFO("bye!");
}
