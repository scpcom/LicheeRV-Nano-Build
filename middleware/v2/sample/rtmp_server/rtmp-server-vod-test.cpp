#include "sockutil.h"
#include "rtmp-server.h"
#include "flv-header.h"
#include "flv-reader.h"
#include "flv-proto.h"
#include "sys/thread.h"
#include "sys/system.h"
#include "sys/sync.hpp"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "cvi_buffer.h"
#include "sample_comm.h"
#include "sophgo_middleware.hpp"
#include "maix_avc2flv.h"


// #define DEBUG_EN
#ifdef DEBUG_EN
#define DEBUG(fmt, args...) printf("[%s][%d]: " fmt, __func__, __LINE__, ##args)
#else
#define DEBUG(fmt, args...)
#endif

// #define USE_AVC2FLV_ITERATE

// #define FILE_RECORDER_EN

#define RTSP_SERVER_TYPE 2

int exit_flag = 0;
static void sig_handle(CVI_S32 signo)
{
	UNUSED(signo);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	exit_flag = 1;
}

typedef struct {
	ThreadLocker s_locker;
	pthread_t t_rtmp;
	rtmp_server_t* s_rtmp;
	const char* s_file;

	int rtmp_is_init;
	int rtmp_playing;
	int rtmp_stop;
#ifdef FILE_RECORDER_EN
	FILE* out_fp;
	uint32_t out_cnt;
#endif

	char ip[16];
	int port;
} priv_t;

static priv_t priv;

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

// static void _rgb8888_to_rgb888(uint8_t *src, uint8_t *dst, int width, int height)
// {
// 	for (int i = 0; i < height; i ++) {
// 		for (int j = 0; j < width; j ++) {
// 			dst[(i * width + j) * 3 + 0] = src[(i * width + j) * 4 + 0];
// 			dst[(i * width + j) * 3 + 1] = src[(i * width + j) * 4 + 1];
// 			dst[(i * width + j) * 3 + 2] = src[(i * width + j) * 4 + 2];
// 		}
// 	}
// }

// // rgb888 to gray
// static void _rgb888_to_gray(uint8_t *src, uint8_t *dst, int width, int height)
// {
// 	for (int i = 0; i < height; i ++) {
// 		for (int j = 0; j < width; j ++) {
// 			dst[i * width + j] = (src[(i * width + j) * 3 + 0] * 38 + src[(i * width + j) * 3 + 1] * 75 + src[(i * width + j) * 3 + 2] * 15) >> 7;
// 		}
// 	}
// }


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

int mmf_enc_h264_init(int ch, int w, int h)
{
	mmf_venc_cfg_t cfg = {
		.type = 2,  //1, h265, 2, h264
		.w = w,
		.h = h,
		.fmt = PIXEL_FORMAT_NV21,
		.jpg_quality = 0,       // unused
		.gop = 50,
		.intput_fps = 30,
		.output_fps = 30,
		.bitrate = 3000,
	};

	return mmf_add_venc_channel(ch, &cfg);
}

int test_mmf_venc_init(int ch, int w, int h)
{
	if (RTSP_SERVER_TYPE == 2)
		return mmf_enc_h264_init(ch, w, h);
	else
		return mmf_enc_h265_init(ch, w, h);
}


#define FLV_TAG_HEAD_LEN 11
#define FLV_PRE_TAG_LEN 4

#ifdef DEBUG_EN
#define FLV_TAG_HEADER_SIZE	11 // StreamID included

static int flv_buffer_read(void* inbuf, int* tagtype, uint32_t* timestamp, size_t* taglen, uint8_t** tagdata, uint8_t** tagnext, size_t bytes)
{
	int r;
	uint32_t sz;
	uint8_t header[FLV_TAG_HEADER_SIZE];
	struct flv_tag_header_t tag;
	uint8_t *readpos = (uint8_t *)inbuf;

	r = FLV_TAG_HEADER_SIZE;
	memcpy(&header, readpos, r);
	readpos += r;
	if (r != FLV_TAG_HEADER_SIZE)
		return r < 0 ? r : 0; // 0-EOF

	if (FLV_TAG_HEADER_SIZE != flv_tag_header_read(&tag, header, FLV_TAG_HEADER_SIZE))
		return -1;

	if (bytes < tag.size)
		return -1;

	// FLV stream
	r = tag.size;
	*tagdata = readpos;
	readpos += r;
	if(tag.size != (uint32_t)r)
		return r < 0 ? r : 0; // 0-EOF

	// PreviousTagSizeN
	r = 4;
	memcpy(&header, readpos, r);
	readpos += r;
	if (4 != r)
		return r < 0 ? r : 0; // 0-EOF

	*tagnext = readpos;
	*taglen = tag.size;
	*tagtype = tag.type;
	*timestamp = tag.timestamp;
	flv_tag_size_read(header, 4, &sz);
	if (0 != tag.streamId)
		printf("%s: id %d != %d\n", __func__, 0, tag.streamId);
	if (sz != tag.size + FLV_TAG_HEADER_SIZE)
		printf("%s: sz %d != %d + %d\n", __func__, sz, tag.size, FLV_TAG_HEADER_SIZE);
	return (sz == tag.size + FLV_TAG_HEADER_SIZE) ? 1 : -2;
}

int flv_buffer_send(void* inbuf, size_t bytes, uint32_t ts)
{
	int r, type;
	size_t taglen;
	uint32_t timestamp;
#if 0
	static uint64_t clock0 = system_clock() - 200; // send more data, open fast
#endif
	uint8_t* packet = NULL;
	uint8_t* next = (uint8_t*)inbuf;

	while (1 == flv_buffer_read(next, &type, &timestamp, &taglen, &packet, &next, bytes))
	{
#if 0
		uint64_t t = system_clock();
		if (clock0 + timestamp > t && clock0 + timestamp < t + 3 * 1000)
			system_sleep(clock0 + timestamp - t);
		else if (clock0 + timestamp > t + 3 * 1000)
			clock0 = t - timestamp;
#endif

		if (ts != timestamp)
		{
			printf("%s: ts %d != %d\n", __func__, ts, timestamp);
		}
		if (priv.rtmp_stop || exit_flag)
		{
			break;
		}
		if (FLV_TYPE_AUDIO == type)
		{
			AutoThreadLocker locker(priv.s_locker);
			r = rtmp_server_send_audio(priv.s_rtmp, packet, taglen, timestamp);
		}
		else if (FLV_TYPE_VIDEO == type)
		{
			AutoThreadLocker locker(priv.s_locker);
			r = rtmp_server_send_video(priv.s_rtmp, packet, taglen, timestamp);
		}
		else if (FLV_TYPE_SCRIPT == type)
		{
			AutoThreadLocker locker(priv.s_locker);
			r = rtmp_server_send_script(priv.s_rtmp, packet, taglen, timestamp);
		}
		else
		{
			r = 0;
		}

		if (next >= (uint8_t *)inbuf + bytes)
		{
			break;
		}

		if (0 != r)
		{
			break; // TODO: handle send failed
		}
	}

	return 0;
}
#endif

static uint8_t* h264_startcode(uint8_t* data, size_t bytes)
{
	size_t i;
	for (i = 2; i + 1 < bytes; i++)
	{
		if (0x01 == data[i] && 0x00 == data[i - 1] && 0x00 == data[i - 2])
			return data + i + 1;
	}

	return NULL;
}

static int maix_rtmp_server_push_h264(mmf_stream_t *vec, uint32_t timestamp)
{
	int num = vec->count;
	int tmp_offset = 0;
	int tmp_max_size = 256 * 1024;
	uint8_t *tmp = (uint8_t *)malloc(tmp_max_size);
	if (!tmp) return -1;

	for (int i = 0; i < num; i ++) {
		uint8_t *nalu = h264_startcode((uint8_t *)vec->data[i], vec->data_size[i]);
		uint8_t *raw = (uint8_t *)vec->data[i];
		int raw_size = (uint8_t *)vec->data[i] + vec->data_size[i] - raw;
		uint8_t nalutype = nalu[0] & 0x1f;
		if (nalutype == 0x7 || nalutype == 0x8 || nalutype == 6) {
			if (tmp_offset + raw_size > tmp_max_size) {
				free(tmp);
				tmp = NULL;
				return -1;
			}
			memcpy(tmp + tmp_offset, raw, raw_size);
			tmp_offset += raw_size;

		}  else if (nalutype == 0x5 || nalutype == 0x1) {
			if (tmp_offset + raw_size > tmp_max_size) {
				free(tmp);
				tmp = NULL;
				return -1;
			}
			memcpy(tmp + tmp_offset, raw, raw_size);
			tmp_offset += raw_size;

			uint8_t *flv;
			int flv_size;
#if defined(USE_AVC2FLV_ITERATE) && !defined(DEBUG_EN)
			if (0 != maix_avc2flv_prepare(tmp, tmp_offset)) {
#else
			if (0 != maix_avc2flv(tmp, tmp_offset, timestamp, timestamp, &flv, &flv_size)) {
#endif
				printf("maix_avc2flv failed!\r\n");
				free(tmp);
				tmp = NULL;
				return -1;
			}
#ifdef DEBUG_EN
			//printf("tmp_offset:%d flv:%p flv_size:%d timestamp:%d\r\n", tmp_offset, flv, flv_size, timestamp);
			flv_buffer_send(flv, flv_size, timestamp);
#ifdef FILE_RECORDER_EN
			if (priv.out_fp) {
				fwrite(flv, 1, flv_size, priv.out_fp);
			}
#endif
#else // DEBUG_EN
			uint8_t *flv_next = flv;
			while (!priv.rtmp_stop && !exit_flag) {
#ifdef USE_AVC2FLV_ITERATE
				flv_size = 0;
				if (0 != maix_avc2flv_iterate((void**)&flv, &flv_size)) {
					break;
				}
				if (flv_size < 1) {
					continue;
				}
				flv_next = flv;
#endif
				if (flv_next >= (flv + flv_size)) break;
				struct flv_tag_header_t tag_header;
				memset(&tag_header, 0, sizeof(tag_header));
				flv_tag_header_read(&tag_header, flv_next, FLV_TAG_HEAD_LEN);

				// printf("flv:%p flv_next:%p tag len:%d timestamp:%d\r\n", flv, flv_next, tag_header.size, timestamp);

				rtmp_server_send_video(priv.s_rtmp, flv_next + FLV_TAG_HEAD_LEN, tag_header.size, timestamp);
#ifdef FILE_RECORDER_EN
				if (priv.out_fp) {
					fwrite(flv_next, 1, FLV_PRE_TAG_LEN + FLV_TAG_HEAD_LEN + tag_header.size, priv.out_fp);
				}
#endif

				flv_next += FLV_PRE_TAG_LEN + FLV_TAG_HEAD_LEN + tag_header.size;
			}
#endif // DEBUG_EN

			tmp_offset = 0;
		}
	}

	if (tmp) {
		free(tmp);
		tmp = NULL;
	}

	return 0;
}

static void rtmp_send_memory_data(uint64_t timestamp, mmf_stream_t *stream)
{
	if (stream->count < 1)
		return;

#ifdef FILE_RECORDER_EN
	if (!priv.out_fp && !priv.out_cnt) {
		priv.out_fp = fopen("./test.flv", "wb+");
	}
	if (priv.out_fp && !priv.out_cnt) {
		uint8_t *flv;
		int flv_size;
		maix_flv_get_header(0, 1, &flv, &flv_size);
		fwrite(flv, 1, flv_size, priv.out_fp);
		printf("%s: file recording\n", __func__);
	}
#endif

	if (0 != maix_rtmp_server_push_h264(stream, timestamp)) {
		printf("rtmp push failed!\r\n");
	}

#ifdef FILE_RECORDER_EN
	if (!priv.out_fp) {
		return;
	}
	if (priv.out_cnt >= 3000) {
		uint8_t *flv;
		int flv_size;
		maix_flv_get_tail(&flv, &flv_size);
		fwrite(flv, 1, flv_size, priv.out_fp);

		fclose(priv.out_fp);
		priv.out_fp = NULL;
		printf("%s: file written.\n", __func__);
		return;
	}

	priv.out_cnt++;
#endif
}

static int STDCALL rtmp_server_file_worker(void* param)
{
	int r, type;
	size_t taglen;
	uint32_t timestamp;
	static uint64_t clock0 = system_clock() - 200; // send more data, open fast
	void* f = flv_reader_create(priv.s_file);
	UNUSED(param);

	static unsigned char packet[8 * 1024 * 1024];
	while (1 == flv_reader_read(f, &type, &timestamp, &taglen, packet, sizeof(packet)))
	{
		assert(taglen < sizeof(packet));
		uint64_t t = system_clock();
		if (clock0 + timestamp > t && clock0 + timestamp < t + 3 * 1000)
			system_sleep(clock0 + timestamp - t);
		else if (clock0 + timestamp > t + 3 * 1000)
			clock0 = t - timestamp;

		if (priv.rtmp_stop || exit_flag)
		{
			break;
		}
		if (FLV_TYPE_AUDIO == type)
		{
			r = rtmp_server_send_audio(priv.s_rtmp, packet, taglen, timestamp);
		}
		else if (FLV_TYPE_VIDEO == type)
		{
			r = rtmp_server_send_video(priv.s_rtmp, packet, taglen, timestamp);
		}
		else if (FLV_TYPE_SCRIPT == type)
		{
			r = rtmp_server_send_script(priv.s_rtmp, packet, taglen, timestamp);
		}
		else
		{
			assert(0);
			r = 0;
		}

		if (0 != r)
		{
			assert(0);
			break; // TODO: handle send failed
		}
	}

	flv_reader_destroy(f);

	priv.rtmp_playing = 0;
	thread_destroy(priv.t_rtmp);
	return 0;
}

static int STDCALL rtmp_server_camera_worker(void* param)
{
	UNUSED(param);

	if (0 != mmf_init()) {
		printf("mmf deinit\n");
		return 0;
	}

	int img_w = 2560, img_h = 1440, img_fps = 30, fit = 0, img_fmt = PIXEL_FORMAT_NV21;
	(void)fit;
	int ch = 0;
	char *sensor_name = mmf_get_sensor_name();
	if (!strcmp(sensor_name, "lt6911")) {
		img_w = 1280; img_h = 720; img_fps = 60;
	}
	if (test_mmf_venc_init(ch, img_w, img_h)) {
		printf("test_mmf_venc_init failed\n");
		return -1;
	}

	uint8_t *filebuf = _prepare_image(img_w, img_h, img_fmt);

	if (0 != mmf_vi_init()) {
		printf("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel_v2(vi_ch, img_w, img_h, img_fmt, img_fps, 2, !true, !true, 2, 3)) {
		printf("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	mmf_vi_set_pop_timeout(100);

	//printf("rtmp://%s:%d/live/stream\n", rtmp_get_server_ip(), rtmp_get_server_port());

	maix_avc2flv_init(3000 * 1000 / 2);

	printf("%s: sending\n", __func__);

	uint64_t start = _get_time_us();
	uint64_t last_loop_us = start;
	uint64_t timestamp = 0;
	int last_vi_pop = -1;
	while (!exit_flag && !priv.rtmp_stop) {
		void *data;
		int data_size, width, height, format;

		if (!last_vi_pop) {
			start = _get_time_us();
			mmf_vi_frame_free(vi_ch);
			DEBUG("use %ld us\r\n", _get_time_us() - start);
		}

		start = _get_time_us();
		int vi_ret = mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format);
		if (vi_ret != last_vi_pop) {
			uint64_t vi_stamp = timestamp;
			vi_stamp += (_get_time_us() - last_loop_us) / 1000;
			printf("[%.6ld.%.3ld] %s\n", vi_stamp / 1000, vi_stamp % 1000,
				vi_ret ? "no input signal" : "got input signal");
			mmf_del_venc_channel(ch);
			test_mmf_venc_init(ch, img_w, img_h);
			last_vi_pop = vi_ret;
		}
		if (vi_ret)
			data = filebuf;
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_venc_push(ch, (uint8_t*)data, img_w, img_h, img_fmt)) {
			printf("mmf_venc_push failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_stream_t stream;
		if (mmf_venc_pop(ch, &stream)) {
			printf("mmf_venc_pop failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		{
			int stream_size = 0;
			for (int i = 0; i < stream.count; i ++) {
				DEBUG("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
				stream_size += stream.data_size[i];
			}

			if (stream_size > 0) {
				rtmp_send_memory_data(timestamp, &stream);
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_venc_free(ch)) {
			printf("mmf_venc_free failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		DEBUG("use %ld us\r\n", _get_time_us() - last_loop_us);
		timestamp += (_get_time_us() - last_loop_us) / 1000;
		last_loop_us = _get_time_us();
	}

	if (mmf_del_venc_channel(ch)) {
		printf("mmf_del_venc_channel failed\n");
		return -1;
	}

	printf("%s: done\n", __func__);
_exit:
	mmf_del_vi_channel(vi_ch);
	mmf_vi_deinit();
	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	free(filebuf);

	maix_avc2flv_deinit();

	priv.rtmp_playing = 0;
	thread_destroy(priv.t_rtmp);

	return 0;
}

static int rtmp_server_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	socket_t* socket = (socket_t*)param;
	socket_bufvec_t vec[2];
	socket_setbufvec(vec, 0, (void*)header, len);
	socket_setbufvec(vec, 1, (void*)data, bytes);
	return socket_send_v_all_by_time(*socket, vec, bytes > 0 ? 2 : 1, 0, 20000);
}

static int rtmp_server_onplay(void* param, const char* app, const char* stream, double start, double duration, uint8_t reset)
{
	printf("rtmp_server_onplay(%s, %s, %f, %f, %d)\n", app, stream, start, duration, (int)reset);

	if (priv.rtmp_playing)
		return 0;

	priv.rtmp_playing = 1;
	if (priv.s_file) {
		return thread_create(&priv.t_rtmp, rtmp_server_file_worker, param);
	}
	return thread_create(&priv.t_rtmp, rtmp_server_camera_worker, param);
}

static int rtmp_server_onpause(void* param, int pause, uint32_t ms)
{
	UNUSED(param);
	printf("rtmp_server_onpause(%d, %u)\n", pause, (unsigned int)ms);
	return 0;
}

static int rtmp_server_onseek(void* param, uint32_t ms)
{
	UNUSED(param);
	printf("rtmp_server_onseek(%u)\n", (unsigned int)ms);
	return 0;
}

static int rtmp_server_ongetduration(void* param, const char* app, const char* stream, double* duration)
{
	UNUSED(param);
	UNUSED(app);
	UNUSED(stream);
	*duration = 24 * 60 * 60;
	return 0;
}

static int get_ip(char *hw, char ip[16])
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
            }

            if (!strcmp(ifa->ifa_name, hw)) {
				strncpy(ip, host, 16);
				freeifaddrs(ifaddr);
				return 0;
            }
        }
    }

    freeifaddrs(ifaddr);
	return -1;
}

char *rtmp_get_server_ip(void)
{
	return priv.ip;
}

int rtmp_get_server_port(void)
{
	return priv.port;
}

int rtmp_server_init(char *ip, int port, const char* flv)
{
	int r;
	socket_t s;
	struct rtmp_server_handler_t handler;

	if (priv.rtmp_is_init)
		return 0;

	char new_ip[16] = {0};
	if (ip == NULL) {
		// if (get_ip((char *)"eth0", new_ip) && get_ip((char *)"usb0", new_ip)) {
		// 	strcpy(new_ip, "0.0.0.0");
		// }
		if (get_ip((char *)"eth0", new_ip) && get_ip((char *)"wlan0", new_ip) && get_ip((char *)"usb0", new_ip)) {
			strcpy(new_ip, "0.0.0.0");
		}
	} else {
		strcpy(new_ip, ip);
	}

	socket_init();

	// create server socket
	//s = socket_tcp_listen(0 /*AF_UNSPEC*/, new_ip, port, SOMAXCONN, 0, 0);
	s = socket_tcp_listen_ipv4(NULL, port, SOMAXCONN);
	if (socket_invalid == s)
		return -1;

	strcpy(priv.ip, new_ip);
	priv.port = port;

	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_server_send;
	//handler.oncreate_stream = rtmp_server_oncreate_stream;
	//handler.ondelete_stream = rtmp_server_ondelete_stream;
	handler.onplay = rtmp_server_onplay;
	handler.onpause = rtmp_server_onpause;
	handler.onseek = rtmp_server_onseek;
	//handler.onpublish = rtmp_server_onpublish;
	//handler.onvideo = rtmp_server_onvideo;
	//handler.onaudio = rtmp_server_onaudio;
	handler.ongetduration = rtmp_server_ongetduration;

	priv.rtmp_playing = 0;
	priv.rtmp_stop = 0;
#ifdef FILE_RECORDER_EN
	priv.out_fp = NULL;
	priv.out_cnt = 0;
#endif

	printf("rtmp://%s:%d/live/stream\n", rtmp_get_server_ip(), rtmp_get_server_port());

	printf("%s: started\n", __func__);

	priv.rtmp_is_init = 1;
	exit_flag = 0;
	while (!exit_flag) {
		socklen_t n;
		struct sockaddr_storage ss;
		socket_t c = socket_accept_by_time(s, &ss, &n, 10);
		if (socket_invalid == c) {
			continue;
		}

		priv.rtmp_playing = 0;
		priv.rtmp_stop = 0;
		priv.s_file = flv;
		priv.s_rtmp = rtmp_server_create(&c, &handler);

		printf("%s: open\n", __func__);

		static unsigned char packet[2 * 1024 * 1024];
		while (!exit_flag)
		{
			r = socket_recv_by_time(c, packet, sizeof(packet), 0, 5);
			if (r > 0)
			{
				AutoThreadLocker locker(priv.s_locker);
				if (0 != rtmp_server_input(priv.s_rtmp, packet, r)) {
					break;
				}
			}
			else if (r <= 0 && r != SOCKET_TIMEDOUT)
			{
				printf("%s: closing\n", __func__);
				break;
			}
		}

		priv.rtmp_stop = 1;
		while (priv.rtmp_playing)
		{
			usleep(1);
		}

		rtmp_server_destroy(priv.s_rtmp);
		socket_close(c);
		printf("%s: close\n", __func__);
	}

	socket_close(s);
	socket_cleanup();

	priv.rtmp_is_init = 0;

	printf("%s: done\n", __func__);

	return 0;
}

int main(int argc, char *argv[])
{
	char *file = NULL;

	if (argc > 1) {
		file = argv[1];
	}

	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	rtmp_server_init(NULL, 1935, file);

	return 0;
}
