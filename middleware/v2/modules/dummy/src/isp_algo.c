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
// #include "sample_comm.h"
#include "dpcm_api.h"
#include "isp_algo_dis.h"
#include "isp_algo_dpc.h"
#include "isp_iir_api.h"

CVI_S32 decoderRaw(RAW_INFO rawInfo, CVI_U8 *outPut)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_bnr_main(struct bnr_param_in *bnr_param_in, struct bnr_param_out *bnr_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ca2_main(
	struct ca2_param_in *ca2_param_in, struct ca2_param_out *ca2_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_cac_main(struct cac_param_in *cac_param_in, struct cac_param_out *cac_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ccm_main(struct ccm_param_in *ccm_param_in, struct ccm_param_out *ccm_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_clut_main(CVI_U32 ViPipe, struct clut_param_in *clut_param_in, struct clut_param_out *clut_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_clut_init(CVI_U32 ViPipe)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_clut_uninit(CVI_U32 ViPipe)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_cnr_main(struct cnr_param_in *cnr_param_in, struct cnr_param_out *cnr_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_csc_main(struct csc_param_in *csc_param_in, struct csc_param_out *csc_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_dci_main(
	struct dci_param_in *dci_param_in, struct dci_param_out *dci_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_dci_init(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_dci_uninit(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_dehaze_main(
	struct dehaze_param_in *dehaze_param_in, struct dehaze_param_out *dehaze_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_demosaic_main(
	struct demosaic_param_in *demosaic_param_in,
	struct demosaic_param_out *demosaic_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 disMainInit(VI_PIPE ViPipe)
{
	return CVI_SUCCESS;
}

CVI_S32 disMainUnInit(VI_PIPE ViPipe)
{
	return CVI_SUCCESS;
}

CVI_S32 disMain(VI_PIPE ViPipe, DIS_MAIN_INPUT_S *inPtr, DIS_MAIN_OUTPUT_S *outPtr)
{
	return CVI_SUCCESS;
}

CVI_S32 disAlgoCreate(VI_PIPE ViPipe)
{
	return CVI_SUCCESS;
}

CVI_S32 disAlgoDestroy(VI_PIPE ViPipe)
{
	return CVI_SUCCESS;
}

int32_t Bayer_12bit_2_16bit(uint8_t *bayerBuffer, uint16_t *outBuffer,
	uint16_t width, uint16_t height, uint16_t stride)
{
	return CVI_SUCCESS;
}

uint16_t DPC_Calib(const DPC_Input *input, DPC_Output *output)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_drc_main(
	struct drc_param_in *drc_param_in, struct drc_param_out *drc_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_drc_init(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_drc_uninit(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_fswdr_main(
	struct fswdr_param_in *fswdr_param_in, struct fswdr_param_out *fswdr_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_lcac_get_internal_memory(CVI_U32 *memory_size)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_lcac_init(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_lcac_uninit(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_lcac_main(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ldci_get_init_coef(int W, int H, struct ldci_init_coef *pldci_init_coef)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ldci_get_internal_memory(CVI_U32 *memory_size)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ldci_init(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ldci_uninit(void)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ldci_main(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_mlsc_main(struct mlsc_param_in *mlsc_param_in, struct mlsc_param_out *mlsc_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_presharpen_main(
	struct presharpen_param_in *presharpen_param_in, struct presharpen_param_out *presharpen_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_rgbcac_main(struct rgbcac_param_in *rgbcac_param_in, struct rgbcac_param_out *rgbcac_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_sharpen_main(struct sharpen_param_in *sharpen_param_in, struct sharpen_param_out *sharpen_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_tnr_main(struct tnr_param_in *tnr_param_in, struct tnr_param_out *tnr_param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_tnr_generateblocksize(
	struct tnr_motion_block_size_in *ptIn, struct tnr_motion_block_size_out *ptOut)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ycontrast_main(
	struct ycontrast_param_in *param_in, struct ycontrast_param_out *param_out)
{
	return CVI_SUCCESS;
}

CVI_S32 isp_algo_ynr_main(struct ynr_param_in *ynr_param_in, struct ynr_param_out *ynr_param_out)
{
	return CVI_SUCCESS;
}

int IIR_U8_Once(TIIR_U8_Ctrl *ptIIRCoef)
{
	return CVI_SUCCESS;
}

int IIR_U16_Once(TIIR_U16_Ctrl *ptIIRCoef)
{
	return CVI_SUCCESS;
}

