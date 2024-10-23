#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "cvi_sys.h"
#include <linux/cvi_type.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "fomat.h"


// #define DEBUG_EN
#ifdef DEBUG_EN
#define DEBUG(fmt, args...) printf("[%s][%d]: "fmt, __func__, __LINE__, ##args)
#else
#define DEBUG(fmt, args...)
#endif

int exit_flag = 0;
static void sig_handle(CVI_S32 signo)
{
	UNUSED(signo);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	exit_flag = 1;
}

#define OLED_DISABLE 	0
#define OLED_ENABLE		1
#define oled_i2c_bus		5	// 4.1.0 sdk: 3
#define oled_i2c_addr		0x3D
#define OLED_CMD		0x00
#define OLED_DATA		0x40

#define OLED_MAX_LINES		8
#define OLED_MAX_CHARS		21

typedef struct {
	int oled_fb;
	char ip[16];
	uint8_t pos_x;
	uint8_t pos_y;
	uint8_t size_x;
	uint8_t size_y;
	char name[OLED_MAX_LINES][OLED_MAX_CHARS+1];
	char data[OLED_MAX_LINES][OLED_MAX_CHARS+1];
} priv_t;

static priv_t priv;

// int oled_dev;
int oled_i2c_init(uint8_t _EN, int * oled_dev)
{
	// PinMux
	system("devmem 0x030010E0 32 0x2");
	system("devmem 0x030010E4 32 0x2");
	int ret; 
	if(_EN) {
		char i2c_dev[12];
		sprintf(i2c_dev, "/dev/i2c-%hhd", oled_i2c_bus);
		*oled_dev = open(i2c_dev, O_RDWR, 0600);
		if (*oled_dev < 0) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Open %s error!\n", i2c_dev);
			return CVI_FAILURE;
		}

		ret = ioctl(*oled_dev, I2C_SLAVE_FORCE, oled_i2c_addr);
		if (ret < 0) {
			printf("I2C_SLAVE_FORCE error! = %d\n", ret);
			close(*oled_dev);
			*oled_dev = -1;
			return ret;
		}
		printf("I2C_SLAVE_FORCE OK! = %d\n", ret);
		printf("OLED opened.\n");
		return ret;
	} else {
		if (*oled_dev >= 0) {
			close(*oled_dev);
			*oled_dev = -1;
			ret = 0;
			printf("OLED closed.\n");
			return ret;
		}
	}
	return -1;
}

int oled_read_register(int _fb, uint8_t addr)
{
	int ret, data;
	CVI_U8 buf[2];

	if (_fb < 0)
		return CVI_FAILURE;

	buf[0] = addr;

	ret = write(_fb, buf, 1);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(_fb, buf, 1);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = buf[0];

	DEBUG("i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

/* mode = OLED_CMD
 * 		= OLED_DATA         */
int oled_write_register(int _fb, uint8_t mode, uint8_t data)
{
	int ret;
	CVI_U8 buf[2];

	if (_fb < 0)
		return CVI_SUCCESS;
	buf[0] = mode;
	buf[1] = data;

	ret = write(_fb, buf, 2);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", mode, data);
	return CVI_SUCCESS;
}

// 坐标设置
void OLED_Set_Pos(int _fb, uint8_t x, uint8_t y) 
{ 
	oled_write_register(_fb, OLED_CMD, 0xb0+y);
	oled_write_register(_fb, OLED_CMD, ((x&0xf0)>>4)|0x10);
	oled_write_register(_fb, OLED_CMD, (x&0x0f));
}

//开启OLED显示    
void OLED_Display_On(int _fb)
{
	oled_write_register(_fb, OLED_CMD, 0X8D);
	oled_write_register(_fb, OLED_CMD, 0X14);
	oled_write_register(_fb, OLED_CMD, 0XAF);
}

//关闭OLED显示     
void OLED_Display_Off(int _fb)
{
	oled_write_register(_fb, OLED_CMD, 0X8D);
	oled_write_register(_fb, OLED_CMD, 0X10);
	oled_write_register(_fb, OLED_CMD, 0XAE);
}

//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	  
void OLED_Clear(int _fb)  
{  
	uint8_t i,n;		    
	for(i=0;i<8;i++)  
	{  
		oled_write_register(_fb, OLED_CMD, 0xb0+i);
		oled_write_register(_fb, OLED_CMD, 0x00);
		oled_write_register(_fb, OLED_CMD, 0x10); 
		for(n=0;n<128;n++)oled_write_register(_fb, OLED_DATA, 0x00); 
	} //更新显示
}

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63				 
//sizey:选择字体 6x8  8x16
void OLED_ShowChar(int _fb, uint8_t x,uint8_t y,uint8_t chr,uint8_t sizey)
{      	
	uint8_t c=0,sizex=sizey/2;
	uint16_t i=0,size1;
	if(sizey==8)size1=6;
	else size1=(sizey/8+((sizey%8)?1:0))*(sizey/2);
	c=chr-' ';//得到偏移后的值
	OLED_Set_Pos(_fb, x, y);
	for(i=0; i<size1; i++)
	{
		if(i%sizex==0&&sizey!=8) OLED_Set_Pos(_fb, x, y++);
		if(sizey==8) oled_write_register(_fb, OLED_DATA, oled_asc2_0806[c][i]); //6X8字号
		else if(sizey==16) oled_write_register(_fb, OLED_DATA, oled_asc2_1608[c][i]);//8x16字号
		else return;
	}
}

uint32_t oled_pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}

//显示数字
//x,y :起点坐标
//num:要显示的数字
//len :数字的位数
//sizey:字体大小		  
void OLED_ShowNum(int _fb, uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey)
{         	
	uint8_t t, temp, m = 0;
	uint8_t enshow = 0;
	if(sizey == 8) m = 2;
	for(t=0; t<len; t++)
	{
		temp = (num / oled_pow(10,len-t-1)) % 10;
		if(enshow == 0 && t < (len-1))
		{
			if(temp == 0)
			{
				OLED_ShowChar(_fb, x+(sizey/2+m)*t, y, ' ',sizey);
				continue;
			}else enshow=1;
		}
	 	OLED_ShowChar(_fb, x+(sizey/2+m)*t, y, temp+'0', sizey);
	}
}

//显示一个字符号串
void OLED_ShowString(int _fb, uint8_t x, uint8_t y, char *chr, uint8_t sizey)
{
	uint8_t j=0;
	while (chr[j]!='\0')
	{		
		OLED_ShowChar(_fb, x, y, chr[j++], sizey);
		if(sizey==8)x+=6;
		else x+=sizey/2;
	}
}

//反显函数
void OLED_ColorTurn(int _fb, uint8_t i)
{
	if(i==0)
	{
		oled_write_register(_fb, OLED_CMD, 0xA6); //正常显示
	}
	if(i==1)
	{
		oled_write_register(_fb, OLED_CMD, 0xA7);//反色显示
	}
}

//屏幕旋转180度
void OLED_DisplayTurn(int _fb, uint8_t i)
{
	if(i==0)
	{
		oled_write_register(_fb, OLED_CMD, 0xC8);//正常显示
		oled_write_register(_fb, OLED_CMD, 0xA1);
	}
	if(i==1)
	{
		oled_write_register(_fb, OLED_CMD, 0xC0);//反转显示
		oled_write_register(_fb, OLED_CMD, 0xA0);
	}
}

//初始化				    
int OLED_Init(int _fb)
{
	int ret;

	// OLED_RES_Clr();
  	// delay_ms(200);
	// OLED_RES_Set();
	
	ret = oled_write_register(_fb, OLED_CMD, 0xAE);//--turn off oled panel
	if (ret != CVI_SUCCESS)
		return ret;

	oled_write_register(_fb, OLED_CMD, 0x00);//---set low column address
	oled_write_register(_fb, OLED_CMD, 0x10);//---set high column address
	oled_write_register(_fb, OLED_CMD, 0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	oled_write_register(_fb, OLED_CMD, 0x81);//--set contrast control register
	oled_write_register(_fb, OLED_CMD, 0xCF);// Set SEG Output Current Brightness
	oled_write_register(_fb, OLED_CMD, 0xA1);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	oled_write_register(_fb, OLED_CMD, 0xC8);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	oled_write_register(_fb, OLED_CMD, 0xA6);//--set normal display
	oled_write_register(_fb, OLED_CMD, 0xA8);//--set multiplex ratio(1 to 64)
	oled_write_register(_fb, OLED_CMD, 0x3f);//--1/64 duty
	oled_write_register(_fb, OLED_CMD, 0xD3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	oled_write_register(_fb, OLED_CMD, 0x00);//-not offset
	oled_write_register(_fb, OLED_CMD, 0xd5);//--set display clock divide ratio/oscillator frequency
	oled_write_register(_fb, OLED_CMD, 0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
	oled_write_register(_fb, OLED_CMD, 0xD9);//--set pre-charge period
	oled_write_register(_fb, OLED_CMD, 0xF1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	oled_write_register(_fb, OLED_CMD, 0xDA);//--set com pins hardware configuration
	oled_write_register(_fb, OLED_CMD, 0x12);//
	oled_write_register(_fb, OLED_CMD, 0xDB);//--set vcomh
	oled_write_register(_fb, OLED_CMD, 0x40);//Set VCOM Deselect Level
	oled_write_register(_fb, OLED_CMD, 0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	oled_write_register(_fb, OLED_CMD, 0x02);//
	oled_write_register(_fb, OLED_CMD, 0x8D);//--set Charge Pump enable/disable
	oled_write_register(_fb, OLED_CMD, 0x14);//--set(0x10) disable
	oled_write_register(_fb, OLED_CMD, 0xA4);// Disable Entire Display On (0xa4/0xa5)
	oled_write_register(_fb, OLED_CMD, 0xA6);// Disable Inverse Display On (0xa6/a7) 
	OLED_Clear(_fb);
	oled_write_register(_fb, OLED_CMD, 0xAF); /*display ON*/ 

	return ret;
}

void OLED_ShowStringtoend(int _fb, uint8_t x, uint8_t y, uint8_t *chr, uint8_t sizey, uint8_t end)
{
	uint8_t j=0;
	while (chr[j] != end)
	{		
		OLED_ShowChar(_fb, x, y, chr[j++], sizey);
		if(sizey==8)x+=6;
		else x+=sizey/2;
	}
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

char *get_server_ip(void)
{
	char new_ip[16] = {0};

	if (!strcmp("0.0.0.0", priv.ip)) {
		if (!get_ip((char *)"eth0", new_ip)) {
			strcpy(priv.ip, new_ip);
		}
		if (!get_ip((char *)"usb0", new_ip)) {
			strcpy(priv.ip, new_ip);
		}
		if (!get_ip((char *)"wlan0", new_ip)) {
			strcpy(priv.ip, new_ip);
		}
	}

	return priv.ip;
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

void show_string_on_oled(int olde_fb, char* name, const char *format, char *indata)
{
	char outdata[OLED_MAX_CHARS*2];
	char *predata;

	if (priv.pos_y >= OLED_MAX_LINES)
		return;
	if (strlen(name) > OLED_MAX_CHARS)
		return;

	predata = (char*)&priv.data[priv.pos_y];
	if (indata)
		sprintf(outdata, format, indata ? indata : "-");
	else
		strcpy(outdata, predata);
	if (strlen(outdata) > OLED_MAX_CHARS)
		return;

	if (strcmp(priv.name[priv.pos_y], name)) {
		OLED_ShowString(olde_fb, priv.pos_x, priv.pos_y, name, priv.size_y);
		strcpy(priv.name[priv.pos_y], name);
	}

	if (strlen(outdata) < strlen(predata)) {
		memset(predata, ' ', strlen(predata));
		strcpy(predata + strlen(predata) - strlen(outdata), outdata);
		OLED_ShowString(olde_fb, priv.pos_x + (OLED_MAX_CHARS - strlen(predata)) * priv.size_x, priv.pos_y, predata, priv.size_y);
		strcpy(priv.data[priv.pos_y], outdata);
	} else if (strcmp(priv.data[priv.pos_y], outdata)) {
		OLED_ShowString(olde_fb, priv.pos_x + (OLED_MAX_CHARS - strlen(outdata)) * priv.size_x, priv.pos_y, outdata, priv.size_y);
		strcpy(priv.data[priv.pos_y], outdata);
	}

	priv.pos_y += 1;
}

void show_string_on_oled_from_file(int olde_fb, char* name, const char *format, const char *filename)
{
	char *indata = file_to_string(filename, OLED_MAX_CHARS);

	show_string_on_oled(olde_fb, name, format, indata);

	if (indata)
		free(indata);
}

int show_info_prepare_oled(void)
{
	int ret;
	int olde_fb;

	if(oled_i2c_init(OLED_ENABLE, &olde_fb) < 0){
		printf("I2C_SLAVE_FORCE error!!\n");
		return CVI_FAILURE;
	}
	ret = OLED_Init(olde_fb);
	if (ret != CVI_SUCCESS)
		goto oled_disable;
	OLED_ColorTurn(olde_fb, 0);	//0正常显示 1 反色显示
  	OLED_DisplayTurn(olde_fb, 0);	//0正常显示 1 屏幕翻转显示
	OLED_Clear(olde_fb);

	priv.oled_fb = olde_fb;
	return CVI_SUCCESS;

oled_disable:
        oled_i2c_init(OLED_DISABLE, &olde_fb);
	return ret;
}

void show_info_on_oled(void)
{
	int olde_fb = priv.oled_fb;

	if (olde_fb < 0)
		return;

	priv.pos_y = 0;
	if(priv.size_y==8) priv.size_x = 6;
	else priv.size_x = priv.size_y / 2;

	priv.pos_y += 1;
	show_string_on_oled(olde_fb, "IP:", "%s", get_server_ip());
	priv.pos_y += 1;
	show_string_on_oled_from_file(olde_fb, "|RES:", "%sP", "/kvmapp/kvm/res");
	show_string_on_oled_from_file(olde_fb, "|TYPE:", "%s", "/kvmapp/kvm/type");
	show_string_on_oled_from_file(olde_fb, "|STREAM:", "%s FPS", "/kvmapp/kvm/now_fps");
	show_string_on_oled_from_file(olde_fb, "|QUALITY:", "%s %%", "/kvmapp/kvm/qlty");
}

void show_info_close_oled()
{
	int olde_fb = priv.oled_fb;

	if (olde_fb < 0)
		return;

	OLED_ColorTurn(olde_fb, 0);	//0正常显示 1 反色显示
  	OLED_DisplayTurn(olde_fb, 0);	//0正常显示 1 屏幕翻转显示
	OLED_Clear(olde_fb);
	oled_i2c_init(OLED_DISABLE, &olde_fb);

	priv.oled_fb = -1;
}

int main(int argc, char *argv[])
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	UNUSED(argc);
	UNUSED(argv);

	priv.oled_fb = -1;
	memset(&priv.name, 0, sizeof(priv.name));
	memset(&priv.data, 0, sizeof(priv.data));
	strcpy(priv.ip, "0.0.0.0");
	priv.pos_x = 2;
	priv.pos_y = 0;
	priv.size_y = 8; //16;

	s32Ret = show_info_prepare_oled();
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	while (!exit_flag) {
		show_info_on_oled();
		usleep(1000 * 1000);
	}

	show_info_close_oled();

	return s32Ret;
}
