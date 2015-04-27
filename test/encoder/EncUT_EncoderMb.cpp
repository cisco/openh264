#include <gtest/gtest.h>

#include "memory_align.h"
#include "utils/DataGenerator.h"

#include "encode_mb_aux.h"
using namespace WelsEnc;

ALIGNED_DECLARE (const int16_t, g_kiQuantInterFFCompare[104][8], 16) = {
  /* 0*/    {   0,     1,     0,     1,     1,     1,     1,     1 },
  /* 1*/    {   0,     1,     0,     1,     1,     1,     1,     1 },
  /* 2*/    {   1,     1,     1,     1,     1,     1,     1,     1 },
  /* 3*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 4*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 5*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 6*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 7*/    {   1,     2,     1,     2,     2,     2,     2,     2 },
  /* 8*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /* 9*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /*10*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /*11*/    {   2,     2,     2,     2,     2,     4,     2,     4 },
  /*12*/    {   2,     3,     2,     3,     3,     4,     3,     4 },
  /*13*/    {   2,     3,     2,     3,     3,     5,     3,     5 },
  /*14*/    {   2,     3,     2,     3,     3,     5,     3,     5 },
  /*15*/    {   2,     4,     2,     4,     4,     6,     4,     6 },
  /*16*/    {   3,     4,     3,     4,     4,     7,     4,     7 },
  /*17*/    {   3,     5,     3,     5,     5,     8,     5,     8 },
  /*18*/    {   3,     6,     3,     6,     6,     9,     6,     9 },
  /*19*/    {   4,     6,     4,     6,     6,    10,     6,    10 },
  /*20*/    {   4,     7,     4,     7,     7,    11,     7,    11 },
  /*21*/    {   5,     8,     5,     8,     8,    12,     8,    12 },
  /*22*/    {   6,     9,     6,     9,     9,    13,     9,    13 },
  /*23*/    {   6,    10,     6,    10,    10,    16,    10,    16 },
  /*24*/    {   7,    11,     7,    11,    11,    17,    11,    17 },
  /*25*/    {   8,    12,     8,    12,    12,    19,    12,    19 },
  /*26*/    {   9,    14,     9,    14,    14,    21,    14,    21 },
  /*27*/    {  10,    15,    10,    15,    15,    25,    15,    25 },
  /*28*/    {  11,    17,    11,    17,    17,    27,    17,    27 },
  /*29*/    {  12,    20,    12,    20,    20,    31,    20,    31 },
  /*30*/    {  14,    22,    14,    22,    22,    34,    22,    34 },
  /*31*/    {  15,    24,    15,    24,    24,    39,    24,    39 },
  /*32*/    {  18,    27,    18,    27,    27,    43,    27,    43 },
  /*33*/    {  19,    31,    19,    31,    31,    49,    31,    49 },
  /*34*/    {  22,    34,    22,    34,    34,    54,    34,    54 },
  /*35*/    {  25,    40,    25,    40,    40,    62,    40,    62 },
  /*36*/    {  27,    45,    27,    45,    45,    69,    45,    69 },
  /*37*/    {  30,    48,    30,    48,    48,    77,    48,    77 },
  /*38*/    {  36,    55,    36,    55,    55,    86,    55,    86 },
  /*39*/    {  38,    62,    38,    62,    62,    99,    62,    99 },
  /*40*/    {  44,    69,    44,    69,    69,   107,    69,   107 },
  /*41*/    {  49,    79,    49,    79,    79,   125,    79,   125 },
  /*42*/    {  55,    89,    55,    89,    89,   137,    89,   137 },
  /*43*/    {  61,    96,    61,    96,    96,   154,    96,   154 },
  /*44*/    {  71,   110,    71,   110,   110,   171,   110,   171 },
  /*45*/    {  77,   124,    77,   124,   124,   198,   124,   198 },
  /*46*/    {  88,   137,    88,   137,   137,   217,   137,   217 },
  /*47*/    {  99,   159,    99,   159,   159,   250,   159,   250 },
  /*48*/    { 110,   179,   110,   179,   179,   275,   179,   275 },
  /*49*/    { 121,   191,   121,   191,   191,   313,   191,   313 },
  /*50*/    { 143,   221,   143,   221,   221,   341,   221,   341 },
  /*51*/    { 154,   245,   154,   245,   245,   402,   245,   402 },
//from here below is intra
  /* 0*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 1*/    {   1,     1,     1,     1,     1,     2,     1,     2 },
  /* 2*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /* 3*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /* 4*/    {   1,     2,     1,     2,     2,     3,     2,     3 },
  /* 5*/    {   1,     2,     1,     2,     2,     4,     2,     4 },
  /* 6*/    {   2,     3,     2,     3,     3,     4,     3,     4 },
  /* 7*/    {   2,     3,     2,     3,     3,     5,     3,     5 },
  /* 8*/    {   2,     3,     2,     3,     3,     5,     3,     5 },
  /* 9*/    {   2,     4,     2,     4,     4,     6,     4,     6 },
  /*10*/    {   3,     4,     3,     4,     4,     6,     4,     6 },
  /*11*/    {   3,     5,     3,     5,     5,     7,     5,     7 },
  /*12*/    {   3,     5,     3,     5,     5,     8,     5,     8 },
  /*13*/    {   4,     6,     4,     6,     6,     9,     6,     9 },
  /*14*/    {   4,     7,     4,     7,     7,    10,     7,    10 },
  /*15*/    {   5,     7,     5,     7,     7,    12,     7,    12 },
  /*16*/    {   5,     8,     5,     8,     8,    13,     8,    13 },
  /*17*/    {   6,     9,     6,     9,     9,    15,     9,    15 },
  /*18*/    {   7,    11,     7,    11,    11,    16,    11,    16 },
  /*19*/    {   7,    11,     7,    11,    11,    18,    11,    18 },
  /*20*/    {   9,    13,     9,    13,    13,    20,    13,    20 },
  /*21*/    {   9,    15,     9,    15,    15,    24,    15,    24 },
  /*22*/    {  11,    16,    11,    16,    16,    26,    16,    26 },
  /*23*/    {  12,    19,    12,    19,    19,    30,    19,    30 },
  /*24*/    {  13,    21,    13,    21,    21,    33,    21,    33 },
  /*25*/    {  14,    23,    14,    23,    23,    37,    23,    37 },
  /*26*/    {  17,    26,    17,    26,    26,    41,    26,    41 },
  /*27*/    {  18,    30,    18,    30,    30,    47,    30,    47 },
  /*28*/    {  21,    33,    21,    33,    33,    51,    33,    51 },
  /*29*/    {  24,    38,    24,    38,    38,    59,    38,    59 },
  /*30*/    {  26,    43,    26,    43,    43,    66,    43,    66 },
  /*31*/    {  29,    46,    29,    46,    46,    74,    46,    74 },
  /*32*/    {  34,    52,    34,    52,    52,    82,    52,    82 },
  /*33*/    {  37,    59,    37,    59,    59,    94,    59,    94 },
  /*34*/    {  42,    66,    42,    66,    66,   102,    66,   102 },
  /*35*/    {  47,    75,    47,    75,    75,   119,    75,   119 },
  /*36*/    {  52,    85,    52,    85,    85,   131,    85,   131 },
  /*37*/    {  58,    92,    58,    92,    92,   147,    92,   147 },
  /*38*/    {  68,   105,    68,   105,   105,   164,   105,   164 },
  /*39*/    {  73,   118,    73,   118,   118,   189,   118,   189 },
  /*40*/    {  84,   131,    84,   131,   131,   205,   131,   205 },
  /*41*/    {  94,   151,    94,   151,   151,   239,   151,   239 },
  /*42*/    { 105,   171,   105,   171,   171,   262,   171,   262 },
  /*43*/    { 116,   184,   116,   184,   184,   295,   184,   295 },
  /*44*/    { 136,   211,   136,   211,   211,   326,   211,   326 },
  /*45*/    { 147,   236,   147,   236,   236,   377,   236,   377 },
  /*46*/    { 168,   262,   168,   262,   262,   414,   262,   414 },
  /*47*/    { 189,   303,   189,   303,   303,   478,   303,   478 },
  /*48*/    { 211,   341,   211,   341,   341,   524,   341,   524 },
  /*49*/    { 231,   364,   231,   364,   364,   597,   364,   597 },
  /*50*/    { 272,   422,   272,   422,   422,   652,   422,   652 },
  /*51*/    { 295,   467,   295,   467,   467,   768,   467,   768 }
};

#define ThValue 2

void TestQuant (uint32_t qp, uint8_t* pSrc, uint8_t* pPred, int16_t* pDct,
                int16_t* pDctCompare, int16_t iWidth, int16_t iHeight) {
  const int16_t* pMf = g_kiQuantMF[qp];
  const int16_t* pFfCompareI = g_kiQuantInterFFCompare[52 + qp];
  const int16_t* pFfCompareP = g_kiQuantInterFFCompare[qp];
  const int16_t* pFfI = g_kiQuantInterFF[6 + qp];
  const int16_t* pFfP = g_kiQuantInterFF[qp];
  //quant4x4  Intra MB
  RandomPixelDataGenerator (pSrc, iWidth, iHeight, iWidth);
  RandomPixelDataGenerator (pPred, iWidth, iHeight, iWidth);

  for (int16_t i = 0; i < 16; i++) {
    pDct[i] = pSrc[i] - pPred[i];
    pDctCompare[i] = pSrc[i] - pPred[i];
  }

  WelsQuant4x4_c (pDct, pFfI, pMf);
  WelsQuant4x4_c (pDctCompare, pFfCompareI, pMf);

  for (int16_t i = 0; i < 16; i++) {
    int16_t iDeta = WELS_ABS (pDct[i] - pDctCompare[i]);
    iDeta = (iDeta < ThValue) ? 0 : iDeta;
    EXPECT_EQ (iDeta, 0);
  }

  //quant4x4 DC
  RandomPixelDataGenerator (pSrc, iWidth, iHeight, iWidth);
  RandomPixelDataGenerator (pPred, iWidth, iHeight, iWidth);

  for (int16_t i = 0; i < 16; i++) {
    pDct[i] = pSrc[i] - pPred[i];
    pDctCompare[i] = pSrc[i] - pPred[i];
  }

  WelsQuant4x4Dc_c (pDct, pFfI[0] << 1, pMf[0]>>1);
  WelsQuant4x4Dc_c (pDctCompare, pFfCompareI[0] << 1, pMf[0]>>1);

  for (int16_t i = 0; i < 16; i++) {
    int16_t iDeta = WELS_ABS (pDct[i] - pDctCompare[i]);
    iDeta = (iDeta < ThValue) ? 0 : iDeta;
    EXPECT_EQ (iDeta, 0);
  }

  //quant4x4 Inter MB
  RandomPixelDataGenerator (pSrc, iWidth, iHeight, iWidth);
  RandomPixelDataGenerator (pPred, iWidth, iHeight, iWidth);

  for (int16_t i = 0; i < 64; i++) {
    pDct[i] =  pSrc[i] - pPred[i];
    pDctCompare[i] = pSrc[i] - pPred[i];
  }

  WelsQuantFour4x4_c (pDct, pFfP, pMf);
  WelsQuantFour4x4_c (pDctCompare, pFfCompareP, pMf);

  for (int16_t i = 0; i < 64; i++) {
    int16_t iDeta = WELS_ABS (pDct[i] - pDctCompare[i]);
    iDeta = (iDeta < ThValue) ? 0 : iDeta;
    EXPECT_EQ (iDeta, 0);
  }
}

TEST (EncoderMbTest, TestQuantTable) {
  CMemoryAlign cMemoryAlign (0);

  int16_t iWidth = 16;
  int16_t iHeight = 16;

  uint8_t* pSrc = (uint8_t*)cMemoryAlign.WelsMalloc (iWidth * iHeight, "quant_src");
  uint8_t* pPred = (uint8_t*)cMemoryAlign.WelsMalloc (iWidth * iHeight, "quant_pred");
  int16_t* pDct = (int16_t*)cMemoryAlign.WelsMalloc (64 * sizeof (int16_t), "Dct Buffer");
  int16_t* pDctCompare = (int16_t*)cMemoryAlign.WelsMalloc (64 * sizeof (int16_t), "DctCompare Buffer");

  for (int32_t iQP = 0; iQP < 51; iQP++) {
    TestQuant (iQP, pSrc, pPred, pDct, pDctCompare, iWidth, iHeight);
  }

  cMemoryAlign.WelsFree (pSrc, "quant_src");
  cMemoryAlign.WelsFree (pPred, "quant_pred");
  cMemoryAlign.WelsFree (pDct, "Dct Buffer");
  cMemoryAlign.WelsFree (pDctCompare, "DctCompare Buffer");
}
