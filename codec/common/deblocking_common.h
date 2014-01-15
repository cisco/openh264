#ifndef WELS_DEBLOCKING_COMMON_H__
#define WELS_DEBLOCKING_COMMON_H__
#include "typedefs.h"
void_t DeblockLumaLt4V_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void_t DeblockLumaEq4V_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockLumaLt4H_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void_t DeblockLumaEq4H_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockChromaLt4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* pTc);
void_t DeblockChromaEq4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockChromaLt4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* pTc);
void_t DeblockChromaEq4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#ifdef  X86_ASM
void DeblockLumaLt4V_ssse3 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void DeblockLumaEq4V_ssse3 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockLumaTransposeH2V_sse2 (uint8_t* pPixY, int32_t iStride, uint8_t* pDst);
void DeblockLumaTransposeV2H_sse2 (uint8_t* pPixY, int32_t iStride, uint8_t* pSrc);
void DeblockLumaLt4H_ssse3 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void DeblockLumaEq4H_ssse3 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockChromaEq4V_ssse3 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockChromaLt4V_ssse3 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                             int8_t* pTC);
void DeblockChromaEq4H_ssse3 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockChromaLt4H_ssse3 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                             int8_t* pTC);
#endif
#if defined(__cplusplus)
}
#endif//__cplusplus
#endif //WELS_DEBLOCKING_COMMON_H__
