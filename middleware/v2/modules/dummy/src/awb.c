#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/cvi_defines.h>
#include "cvi_buffer.h"
#include "cvi_awb.h"

CVI_S32 CVI_AWB_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_AWB_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_AWB_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
				AWB_SENSOR_REGISTER_S *pstRegister)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_AWB_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, SENSOR_ID SensorId)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_AWB_QueryInfo(VI_PIPE ViPipe, ISP_WB_Q_INFO_S *pstWB_Q_Info)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBAttr(VI_PIPE ViPipe, const ISP_WB_ATTR_S *pstWBAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWBAttr(VI_PIPE ViPipe, ISP_WB_ATTR_S *pstWBAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAWBAttrEx(VI_PIPE ViPipe, const ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAWBAttrEx(VI_PIPE ViPipe, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetGrayWorldAwbInfo(VI_PIPE ViPipe, CVI_U16 *pRgain, CVI_U16 *pBgain)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBCalibration(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWBCalibration(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWBCalibrationEx(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWBCalibrationEx(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S_EX *pstWBCalib)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}
