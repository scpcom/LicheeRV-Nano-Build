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
#include "cvi_sns_ctrl.h"
#include "cvi_ae.h"
#include "cvi_isp.h"
#include "3A_internal.h"

CVI_S32 CVI_AE_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	UNUSED(ViPipe);
	UNUSED(pstAeLib);

	return CVI_SUCCESS;
}

CVI_S32 CVI_AE_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	UNUSED(ViPipe);
	UNUSED(pstAeLib);

	return CVI_SUCCESS;
}

CVI_S32 CVI_AE_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
				     AE_SENSOR_REGISTER_S *pstRegister)
{
	UNUSED(ViPipe);
	UNUSED(pstAeLib);
	UNUSED(pstSnsAttrInfo);
	UNUSED(pstRegister);

	return CVI_SUCCESS;
}

CVI_S32 CVI_AE_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, SENSOR_ID SensorId)
{
	UNUSED(ViPipe);
	UNUSED(pstAeLib);
	UNUSED(SensorId);

	return CVI_SUCCESS;
}

CVI_U8 AE_GetSensorPeriod(CVI_U8 sID)
{
	UNUSED(sID);

	return 0;
}

CVI_U8 AE_GetSensorExpGainPeriod(CVI_U8 sID)
{
	UNUSED(sID);

	return 0;
}

CVI_S32 CVI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstExpAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
	UNUSED(ViPipe);

	if (!pstExpAttr)
		return CVI_FAILURE;

	memset(pstExpAttr, 0, sizeof(*pstExpAttr));

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo)
{
	UNUSED(ViPipe);

	if (!pstExpInfo)
		return CVI_FAILURE;

	memset(pstExpInfo, 0, sizeof(*pstExpInfo));

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetFrameID(VI_PIPE ViPipe, CVI_U32 *frameID)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_QueryFps(VI_PIPE ViPipe, CVI_FLOAT *pFps)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	UNUSED(ViPipe);
	UNUSED(pstWDRExpAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{
	UNUSED(ViPipe);

	if (!pstWDRExpAttr)
		return CVI_FAILURE;

	memset(pstWDRExpAttr, 0, sizeof(*pstWDRExpAttr));

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetSmartExposureAttr(VI_PIPE ViPipe, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetSmartExposureAttr(VI_PIPE ViPipe, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAEStatisticsConfig(VI_PIPE ViPipe, const ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAEStatisticsConfig(VI_PIPE ViPipe, ISP_AE_STATISTICS_CFG_S *pstAeStatCfg)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteSFAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteSFAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteSFAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetAERouteSFAttrEx(VI_PIPE ViPipe, const ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAERouteSFAttrEx(VI_PIPE ViPipe, ISP_AE_ROUTE_EX_S *pstAERouteSFAttrEx)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetWDRLEOnly(VI_PIPE ViPipe, CVI_BOOL wdrLEOnly)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetIrisAttr(VI_PIPE ViPipe, const ISP_IRIS_ATTR_S *pstIrisAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetIrisAttr(VI_PIPE ViPipe, ISP_IRIS_ATTR_S *pstIrisAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetDcirisAttr(VI_PIPE ViPipe, const ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetDcirisAttr(VI_PIPE ViPipe, ISP_DCIRIS_ATTR_S *pstDcirisAttr)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

// 3A_internal.h

CVI_S32 CVI_ISP_GetAELogBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAELogBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AESetRawDumpFrameID(VI_PIPE ViPipe, CVI_U32 fid, CVI_U16 frmNum)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AEGetRawReplayExpBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 *bufSize)
{
	UNUSED(ViPipe);
	return CVI_SUCCESS;
}
