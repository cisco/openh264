#include<gtest/gtest.h>
#include<stdint.h>
// copy reference C code from codec
#define MAX_NEG_CROP 1024
uint8_t g_ClipTable[256 + 2 *
                    MAX_NEG_CROP];	//the front 1024 is 0, the back 1024 is 255, the middle 256 elements is 0-255


/* init pClip table to pClip the final dct data */
void InitDctClipTable (void) {
  uint8_t* p		        = &g_ClipTable[0];
  const int32_t kiLength	= MAX_NEG_CROP * sizeof (uint8_t);
  int32_t i               = 0;

  do {
    const int32_t kiIdx = MAX_NEG_CROP + i;

    p[kiIdx]	= i;
    p[1 + kiIdx]	= 1 + i;
    p[2 + kiIdx]	= 2 + i;
    p[3 + kiIdx]	= 3 + i;

    i += 4;
  } while (i < 256);

  memset (p, 0, kiLength);
  memset (p + MAX_NEG_CROP + 256, 0xFF, kiLength);
}

//NOTE::: p_RS should NOT be modified and it will lead to mismatch with JSVM.
//        so should allocate kA array to store the temporary value (idct).
void IdctResAddPred_c (uint8_t* pPred, const int32_t kiStride, int16_t* pRs) {
  int16_t iSrc[16];

  uint8_t* pDst			= pPred;
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
  uint8_t* pClip			= &g_ClipTable[MAX_NEG_CROP];
  int32_t i;

  for (i = 0; i < 4; i++) {
    const int32_t kiY  = i << 2;
    const int32_t kiT0 = pRs[kiY] + pRs[kiY + 2];
    const int32_t kiT1 = pRs[kiY] - pRs[kiY + 2];
    const int32_t kiT2 = (pRs[kiY + 1] >> 1) - pRs[kiY + 3];
    const int32_t kiT3 = pRs[kiY + 1] + (pRs[kiY + 3] >> 1);

    iSrc[kiY] = kiT0 + kiT3;
    iSrc[kiY + 1] = kiT1 + kiT2;
    iSrc[kiY + 2] = kiT1 - kiT2;
    iSrc[kiY + 3] = kiT0 - kiT3;
  }

  for (i = 0; i < 4; i++) {
    int32_t kT1	= iSrc[i]	+ iSrc[i + 8];
    int32_t kT2	= iSrc[i + 4] + (iSrc[i + 12] >> 1);
    int32_t kT3	= (32 + kT1 + kT2) >> 6;
    int32_t kT4	= (32 + kT1 - kT2) >> 6;

    pDst[i] = pClip[ kT3 + pPred[i] ];
    pDst[i + kiStride3] = pClip[ kT4 + pPred[i + kiStride3] ];

    kT1	= iSrc[i] - iSrc[i + 8];
    kT2	= (iSrc[i + 4] >> 1) - iSrc[i + 12];
    pDst[i + kiStride] = pClip[ ((32 + kT1 + kT2) >> 6) + pDst[i + kiStride] ];
    pDst[i + kiStride2] = pClip[ ((32 + kT1 - kT2) >> 6) + pDst[i + kiStride2] ];
  }
}

extern "C" void IdctResAddPred_mmx( uint8_t *pPred, const int32_t kiStride, int16_t *pRs );
TEST(dctTest, IdctResAddPred_mmx)
{
   int16_t pRS[16];
   uint8_t *pPred_ref, *pPred_mmx;
   int32_t iStride = 64;
   pPred_ref = new uint8_t[4*iStride];
   pPred_mmx = new uint8_t[4*iStride];
   InitDctClipTable();
   srand(time(NULL));
   const int bits = 13;
   const int range = (1 << bits) - 1;
   const int offset = 1 << (bits - 1);
   int times = 100;
   while(times-- > 0) {
   for(int i = 0; i < 16; i++) {
      pRS[i] = (rand() & range) - offset;
   }
   for(int i = 0; i < 4; i++)
     for(int j = 0; j < 4; j++)
        pPred_ref[i*iStride+j] = pPred_mmx[i*iStride+j] = rand() & 255;
  IdctResAddPred_mmx(pPred_mmx, iStride, pRS);
  IdctResAddPred_c(pPred_ref, iStride, pRS);
  for(int i = 0; i < 4; i++)
    for(int j = 0; j < 4; j++)
      ASSERT_EQ(pPred_ref[i*iStride+j], pPred_mmx[i*iStride+j]);
  }
}
