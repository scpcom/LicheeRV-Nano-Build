#include "cvi_miniz.h"

MINIZ_EXPORT int cvi_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level)
{
	return 0;
}

MINIZ_EXPORT mz_ulong cvi_compressBound(mz_ulong source_len)
{
	return 0;
}

MINIZ_EXPORT int cvi_uncompress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong *pSource_len)
{
	return 0;
}
