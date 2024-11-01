
/*
 * Copyright (c) Calin Crisan
 * This file is part of streamEye.
 *
 * streamEye is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include "streameye.h"
#include "auth.h"
#ifdef __cplusplus
}
#endif

#include "cvi_buffer.h"

#include "sample_comm.h"
#ifdef __cplusplus
#include "sophgo_middleware.hpp"
#else
#include "maix_mmf.h"
#endif

    /* locals */

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

// -------------------- mmf locals end   --------------------

static int client_timeout = DEF_CLIENT_TIMEOUT;
static int max_clients = 0;
static int tcp_port = 8000;
static int listen_localhost = 1;
static char *input_separator = NULL;
static client_t **clients = NULL;
static int num_clients = 0;


#ifdef __cplusplus
extern "C" {
#endif
    /* globals */

int log_level = 1; /* 0 - quiet, 1 - info, 2 - debug */
char jpeg_buf[JPEG_BUF_LEN];
int jpeg_size = 0;
int running = 1;
pthread_cond_t jpeg_cond;
pthread_mutex_t jpeg_mutex;
pthread_mutex_t clients_mutex;


    /* local functions */

static int          init_server();
static client_t *   wait_for_client(int socket_fd);
static void         print_help();


    /* server socket */

int init_server() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        ERRNO("socket() failed");
        return -1;
    }

    int tr = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(tr)) < 0) {
        ERRNO("setsockopt() failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    if (listen_localhost) {
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    else {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    server_addr.sin_port = htons(tcp_port);

    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        ERRNO("bind() failed");
        close(socket_fd);
        return -1;
    }

    if (listen(socket_fd, 5) < 0) {
        ERRNO("listen() failed");
        close(socket_fd);
        return -1;
    }

    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) < 0) {
        ERRNO("fcntl() failed");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

client_t *wait_for_client(int socket_fd) {
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(client_addr);

    /* wait for a connection */
    int stream_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &client_len);
    if (stream_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            ERRNO("accept() failed");
        }

        return NULL;
    }

    /* set socket timeout */
    struct timeval tv;

    tv.tv_sec = client_timeout;
    tv.tv_usec = 0;

    setsockopt(stream_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));
    setsockopt(stream_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(struct timeval));

    /* create client structure */
    client_t *client = (client_t*)malloc(sizeof(client_t));
    if (!client) {
        ERROR("malloc() failed");
        return NULL;
    }

    memset(client, 0, sizeof(client_t));

    client->stream_fd = stream_fd;
    inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client->addr, INET_ADDRSTRLEN);
    client->port = ntohs(client_addr.sin_port);

    INFO("new client connection from %s:%d", client->addr, client->port);

    return client;
}

void cleanup_client(client_t *client) {
    DEBUG_CLIENT(client, "cleaning up");

    if (pthread_mutex_lock(&clients_mutex)) {
        ERROR("pthread_mutex_lock() failed");
    }

    int i, j;
    for (i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            /* move all further entries back with one position */
            for (j = i; j < num_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }

            break;
        }
    }

    close(client->stream_fd);
    if (client->auth_basic_hash) {
        free(client->auth_basic_hash);
    }
    if (client->jpeg_tmp_buf) {
        free(client->jpeg_tmp_buf);
    }
    free(client);

    clients = (client_t**)realloc(clients, sizeof(client_t *) * (--num_clients));
    DEBUG("current clients: %d", num_clients);

    if (pthread_mutex_unlock(&clients_mutex)) {
        ERROR("pthread_mutex_unlock() failed");
    }
}


    /* main */

char *str_timestamp() {
    static __thread char s[20];

    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);

    strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", tmp);

    return s;
}

void print_help() {
    fprintf(stderr, "\n");
    fprintf(stderr, "streamEye %s\n\n", STREAM_EYE_VERSION);
    fprintf(stderr, "Usage: <jpeg stream> | streameye [options]\n");
    fprintf(stderr, "Available options:\n");
    fprintf(stderr, "    -a off|basic       HTTP authentication mode (defaults to off)\n");
    fprintf(stderr, "    -c user:pass:realm credentials for HTTP authentication\n");
    fprintf(stderr, "    -d                 debug mode, increased log verbosity\n");
    fprintf(stderr, "    -h                 print this help text\n");
    fprintf(stderr, "    -l                 listen only on localhost interface\n");
    fprintf(stderr, "    -m max_clients     the maximal number of simultaneous clients (defaults to unlimited)\n");
    fprintf(stderr, "    -p port            tcp port to listen on (defaults to %d)\n", DEF_TCP_PORT);
    fprintf(stderr, "    -q                 quiet mode, log only errors\n");
    fprintf(stderr, "    -s separator       a separator between jpeg frames received at input\n");
    fprintf(stderr, "                       (will autodetect jpeg frame starts by default)\n");
    fprintf(stderr, "    -t timeout         client read/write timeout, in seconds (defaults to %d)\n", DEF_CLIENT_TIMEOUT);
    fprintf(stderr, "\n");
}
#ifdef __cplusplus
}
#endif

double get_now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void bye_handler(int signal) {
    if (!running) {
        INFO("interrupt already received, ignoring signal");
        return;
    }

    INFO("interrupt received, quitting");
    running = 0;
}

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

int main(int argc, char *argv[]) {

    /* read command line arguments */
    int c;
    char *err = NULL;
    char *p, *q;

    int auth_mode = AUTH_OFF;
    char *auth_username = NULL;
    char *auth_password = NULL;
    char *auth_realm = NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "a:c:dhlm:p:qs:t:")) != -1) {
        switch (c) {
            case 'a': /* authentication */
                if (!strcmp(optarg, "basic")) {
                    auth_mode = AUTH_BASIC;
                }
                break;

            case 'c': /* credentials */
                p = q = optarg;
                while (*q && *q != ':') {
                    q++;
                }
                auth_username = strndup(p, q - p);

                if (!*q) {
                    ERROR("invalid credentials");
                    return -1;
                }
                p = q = q + 1;
                while (*q && *q != ':') {
                    q++;
                }
                auth_password = strndup(p, q - p);

                if (!*q) {
                    ERROR("invalid credentials");
                    return -1;
                }
                p = q = q + 1;
                while (*q && *q != ':') {
                    q++;
                }
                auth_realm = strndup(p, q - p);

                break;

            case 'd': /* debug */
                log_level = 2;
                break;

            case 'h': /* help */
                print_help();
                return 0;

            case 'l': /* listen on localhost */
                listen_localhost = 1;
                break;

            case 'm': /* max clients */
                max_clients = strtol(optarg, &err, 10);
                if (*err != 0) {
                    ERROR("invalid clients number \"%s\"", optarg);
                    return -1;
                }
                break;

            case 'p': /* tcp port */
                tcp_port = strtol(optarg, &err, 10);
                if (*err != 0) {
                    ERROR("invalid port \"%s\"", optarg);
                    return -1;
                }
                break;

            case 'q': /* quiet */
                log_level = 0;
                break;

            case 's': /* input separator */
                input_separator = strdup(optarg);
                break;

            case 't': /* client timeout */
                client_timeout = strtol(optarg, &err, 10);
                if (*err != 0) {
                    ERROR("invalid client timeout \"%s\"", optarg);
                    return -1;
                }
                break;

            case '?':
                ERROR("unknown or incomplete option \"-%c\"", optopt);
                return -1;

            default:
                print_help();
                return -1;
        }
    }

    if (auth_mode) {
        if (!auth_username || !auth_password || !auth_realm) {
            ERROR("credentials are required when using authentication");
            return -1;
        }

        set_auth(auth_mode, auth_username, auth_password, auth_realm);
    }

    if (!tcp_port) {
        tcp_port = DEF_TCP_PORT;
    }

    INFO("streamEye %s", STREAM_EYE_VERSION);
    INFO("hello!");

    if (input_separator && strlen(input_separator) < 4) {
        INFO("the input separator supplied is very likely to appear in the actual frame data (consider a longer one)");
    }

    /* signals */
    DEBUG("installing signal handlers");
    struct sigaction act;
    act.sa_handler = bye_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGINT, &act, NULL) < 0) {
        ERRNO("sigaction() failed");
        return -1;
    }
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        ERRNO("sigaction() failed");
        return -1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        ERRNO("signal() failed");
        return -1;
    }

    /* threading */
    DEBUG("initializing thread synchronization");
    if (pthread_cond_init(&jpeg_cond, NULL)) {
        ERROR("pthread_cond_init() failed");
        return -1;
    }
    if (pthread_mutex_init(&jpeg_mutex, NULL)) {
        ERROR("pthread_mutex_init() failed");
        return -1;
    }
    if (pthread_mutex_init(&clients_mutex, NULL)) {
        ERROR("pthread_mutex_init() failed");
        return -1;
    }

    /* tcp server */
    DEBUG("starting server");
    int socket_fd = init_server();
    if (socket_fd < 0) {
        ERROR("failed to start server");
        return -1;
    }

    INFO("listening on %s:%d", listen_localhost ? "127.0.0.1" : "0.0.0.0", tcp_port);

    /* main loop */
    char input_buf[INPUT_BUF_LEN];
    char *sep = NULL;
    int size, rem_len = 0, i;

    double now, min_client_frame_int;
    double frame_int_adj;
    double frame_int = 0;
    double last_frame_time = get_now();

    int auto_separator = 0;
    int input_separator_len;
    if (!input_separator) {
        auto_separator = 1;
        input_separator_len = 4; /* strlen(JPEG_START) + strlen(JPEG_END) */;
        input_separator = (char*)malloc(input_separator_len + 1);
        snprintf(input_separator, input_separator_len + 1, "%s%s", JPEG_END, JPEG_START);
    }
    else {
        input_separator_len = strlen(input_separator);
    }

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

	//printf("http://%s:%d/stream\n", stream_server_get_ip(), stream_server_get_port());

	uint64_t start = _get_time_us();
	uint64_t last_loop_us = start;
	uint64_t timestamp = 0;
	uint64_t loop_count = 0;
	int last_vi_pop = -1;
	int last_size = 0;
	// -------------------- mmf init end   --------------------

    while (running) {
	// -------------------- mmf loop begin --------------------
	{
		void *data;
		int data_size, width, height, format;

		if (kvm_cfg_read()) {
			if (0 != kvm_reset_mmf_channels(mmf_cfg))
				break;
		}

		if (!last_vi_pop) {
			start = _get_time_us();
			mmf_vi_frame_free(mmf_cfg->vi_ch);
			DEBUG("use %ld us\r\n", _get_time_us() - start);
		}

		start = _get_time_us();
		int vi_ret = mmf_vi_frame_pop(mmf_cfg->vi_ch, &data, &data_size, &width, &height, &format);
		if (vi_ret != last_vi_pop) {
			uint64_t vi_stamp = timestamp;
			vi_stamp += (_get_time_us() - last_loop_us) / 1000;
			printf("[%.6ld.%.3ld] %s\n", vi_stamp / 1000, vi_stamp % 1000,
				vi_ret ? "no input signal" : "got input signal");
			mmf_del_venc_channel(mmf_cfg->enc_ch);
			kvm_stream_venc_init(mmf_cfg->enc_ch, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt, mmf_cfg->img_qlty);
			//kvm_write_state(vi_ret ? 0 : 1);
			last_vi_pop = vi_ret;
		}
		if (vi_ret)
			data = mmf_cfg->filebuf;
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_venc_push(mmf_cfg->enc_ch, (uint8_t*)data, mmf_cfg->img_w, mmf_cfg->img_h, mmf_cfg->img_fmt)) {
			printf("mmf_venc_push failed\n");
			break;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_stream_t stream;
		stream.count = 0;
		if (mmf_venc_pop(mmf_cfg->enc_ch, &stream)) {
			printf("mmf_venc_pop failed\n");
			break;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		size = last_size;
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
					if (!size && INPUT_BUF_LEN >= copy_length) {
						last_size = copy_length;
						size = last_size;
						memcpy(&input_buf, stream_buffer, size);
					}
					loop_count++;
					free(stream_buffer);
				} else {
					DEBUG("malloc failed!\r\n");
				}
			} else if (stream.count == 1) {
				if (INPUT_BUF_LEN >= stream.data_size[0]) {
					last_size = stream.data_size[0];
					size = last_size;
					memcpy(&input_buf, (uint8_t *)stream.data[0], size);
				}
				loop_count++;
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_venc_free(mmf_cfg->enc_ch)) {
			printf("mmf_venc_free failed\n");
			break;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		DEBUG("use %ld us\r\n", _get_time_us() - last_loop_us);
		timestamp += (_get_time_us() - last_loop_us) / 1000;
		last_loop_us = _get_time_us();
	}
	// -------------------- mmf loop end   --------------------

	/* input_buf is filled by mmf */
        //size = read(STDIN_FILENO, input_buf, INPUT_BUF_LEN);
        if (size < 0) {
            if (errno == EINTR) {
                break;
            }

            ERRNO("input: read() failed");
            return -1;
        }
        else if (size == 0) {
            DEBUG("input: end of stream");
            running = 0;
            break;
        }

        if (size > JPEG_BUF_LEN - 1 - jpeg_size) {
            ERROR("input: jpeg size too large, discarding buffer");
            jpeg_size = 0;
            continue;
        }

        if (pthread_mutex_lock(&jpeg_mutex)) {
            ERROR("pthread_mutex_lock() failed");
            return -1;
        }

        /* clear the ready flag for all clients,
         * as we start building the next frame */
        for (i = 0; i < num_clients; i++) {
            clients[i]->jpeg_ready = 0;
        }

        if (rem_len) {
            /* copy the remainder of data from the previous iteration back to the jpeg buffer */
            memmove(jpeg_buf, sep + (auto_separator ? 2 /* strlen(JPEG_END) */ : input_separator_len), rem_len);
            jpeg_size = rem_len;
        }

        /* always use recent image */
        jpeg_size = 0;
        memcpy(jpeg_buf + jpeg_size, input_buf, size);
        jpeg_size += size;

        /* look behind at most 2 * INPUT_BUF_LEN for a separator */
        //sep = (char *) memmem(jpeg_buf + jpeg_size - MIN(2 * INPUT_BUF_LEN, jpeg_size), MIN(2 * INPUT_BUF_LEN, jpeg_size),
        //        input_separator, input_separator_len);
        /* no need to search we always get a full jpeg image */
        if (auto_separator)
                sep = jpeg_buf + jpeg_size - 2;
        else
                sep = jpeg_buf + jpeg_size;

        if (sep) { /* found a separator, jpeg frame is ready */
            if (auto_separator) {
                rem_len = jpeg_size - (sep - jpeg_buf) - 2 /* strlen(JPEG_START) */;
                jpeg_size = sep - jpeg_buf + 2 /* strlen(JPEG_END) */;
            }
            else {
                rem_len = jpeg_size - (sep - jpeg_buf) - input_separator_len;
                jpeg_size = sep - jpeg_buf;
            }

            DEBUG("input: jpeg buffer ready with %d bytes", jpeg_size);

            /* set the ready flag and notify all client threads about it */
            for (i = 0; i < num_clients; i++) {
                clients[i]->jpeg_ready = 1;
            }
            if (pthread_cond_broadcast(&jpeg_cond)) {
                ERROR("pthread_cond_broadcast() failed");
                return -1;
            }

            now = get_now();
            frame_int = frame_int * 0.7 + (now - last_frame_time) * 0.3;
            last_frame_time = now;
        }
        else {
            rem_len = 0;
        }

        if (pthread_mutex_unlock(&jpeg_mutex)) {
            ERROR("pthread_mutex_unlock() failed");
            return -1;
        }

        if (sep) {
            kvm_write_now_fps(frame_int);
            DEBUG("current fps: %.01lf", 1 / frame_int);

            if (num_clients) {
                min_client_frame_int = clients[0]->frame_int;
                for (i = 0; i < num_clients; i++) {
                    if (clients[i]->frame_int < min_client_frame_int) {
                        min_client_frame_int = clients[i]->frame_int;
                    }
                }

                frame_int_adj = (min_client_frame_int - frame_int) * 1000000;
                if (frame_int_adj > 0) {
                    DEBUG("input frame int.: %.0lf us, client frame int.: %.0lf us, frame int. adjustment: %.0lf us",
                            frame_int * 1000000, min_client_frame_int * 1000000, frame_int_adj);

                    /* sleep between 1000 and 50000 us, depending on the frame interval adjustment */
                    usleep(MAX(1000, MIN(4 * frame_int_adj, 50000)));
                }
            }

            /* check for incoming clients;
             * placing this code inside the if (sep) will simply
             * reduce the number of times we check for incoming clients,
             * with no particular relation to the frame separator we've just found */
            client_t *client = NULL;

            if (!max_clients || num_clients < max_clients) {
                client = wait_for_client(socket_fd);
            }

            if (client) {
                if (pthread_create(&client->thread, NULL, (void *(*) (void *)) handle_client, client)) {
                    ERROR("pthread_create() failed");
                    return -1;
                }

                if (pthread_mutex_lock(&clients_mutex)) {
                    ERROR("pthread_mutex_lock() failed");
                    return -1;
                }

                clients = (client_t**)realloc(clients, sizeof(client_t *) * (num_clients + 1));
                clients[num_clients++] = client;

                DEBUG("current clients: %d", num_clients);

                if (pthread_mutex_unlock(&clients_mutex)) {
                    ERROR("pthread_mutex_unlock() failed");
                    return -1;
                }
            }
        }
    }
    
    running = 0;

    DEBUG("closing server");
    close(socket_fd);

    DEBUG("waiting for clients to finish");
    for (i = 0; i < num_clients; i++) {
        clients[i]->jpeg_ready = 1;
    }
    if (pthread_cond_broadcast(&jpeg_cond)) {
        ERROR("pthread_cond_broadcast() failed");
        return -1;
    }

    for (i = 0; i < num_clients; i++) {
        pthread_join(clients[i]->thread, NULL);
    }

    if (pthread_mutex_destroy(&clients_mutex)) {
        ERROR("pthread_mutex_destroy() failed");
        return -1;
    }
    if (pthread_mutex_destroy(&jpeg_mutex)) {
        ERROR("pthread_mutex_destroy() failed");
        return -1;
    }
    if (pthread_cond_destroy(&jpeg_cond)) {
        ERROR("pthread_cond_destroy() failed");
        return -1;
    }

	//  ------------------- mmf deinit begin --------------------
	kvm_write_state(0);
	kvm_write_now_fps(0);

	if (0 != kvm_deinit_mmf_channels(mmf_cfg))
		return -1;

	mmf_vi_deinit();

	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	// -------------------- mmf deinit end ----------------------

    INFO("bye!");

    return 0;
}
