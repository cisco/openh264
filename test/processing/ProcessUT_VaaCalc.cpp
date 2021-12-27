#include <gtest/gtest.h>
#include "cpu.h"
#include "cpu_core.h"
#include "util.h"
#include "macros.h"
#include "IWelsVP.h"
#include "vaacalculation.h"

using namespace WelsVP;

void VAACalcSadSsd_ref (const uint8_t* pCurData, const uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                        int32_t iPicStride,
                        int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* psqsum16x16, int32_t* psqdiff16x16) {
  const uint8_t* tmp_ref = pRefData;
  const uint8_t* tmp_cur = pCurData;
  int32_t iMbWidth = (iPicWidth >> 4);
  int32_t mb_height = (iPicHeight >> 4);
  int32_t mb_index = 0;
  int32_t pic_stride_x8 = iPicStride << 3;
  int32_t step = (iPicStride << 4) - iPicWidth;

  *pFrameSad = 0;
  for (int32_t i = 0; i < mb_height; i ++) {
    for (int32_t j = 0; j < iMbWidth; j ++) {
      int32_t k, l;
      int32_t l_sad, l_sqdiff, l_sum, l_sqsum;
      const uint8_t* tmp_cur_row;
      const uint8_t* tmp_ref_row;

      pSum16x16[mb_index] = 0;
      psqsum16x16[mb_index] = 0;
      psqdiff16x16[mb_index] = 0;

      l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur;
      tmp_ref_row = tmp_ref;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sqdiff += diff * diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 0] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;

      l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + 8;
      tmp_ref_row = tmp_ref + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sqdiff += diff * diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 1] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;

      l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8;
      tmp_ref_row = tmp_ref + pic_stride_x8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sqdiff += diff * diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 2] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;

      l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8 + 8;
      tmp_ref_row = tmp_ref + pic_stride_x8 + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sqdiff += diff * diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 3] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;


      tmp_ref += 16;
      tmp_cur += 16;
      ++mb_index;
    }
    tmp_ref += step;
    tmp_cur += step;
  }
}
void VAACalcSadVar_ref (const uint8_t* pCurData, const uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                        int32_t iPicStride,
                        int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* psqsum16x16) {
  const uint8_t* tmp_ref = pRefData;
  const uint8_t* tmp_cur = pCurData;
  int32_t iMbWidth = (iPicWidth >> 4);
  int32_t mb_height = (iPicHeight >> 4);
  int32_t mb_index = 0;
  int32_t pic_stride_x8 = iPicStride << 3;
  int32_t step = (iPicStride << 4) - iPicWidth;

  *pFrameSad = 0;
  for (int32_t i = 0; i < mb_height; i ++) {
    for (int32_t j = 0; j < iMbWidth; j ++) {
      int32_t k, l;
      int32_t l_sad, l_sum, l_sqsum;
      const uint8_t* tmp_cur_row;
      const uint8_t* tmp_ref_row;

      pSum16x16[mb_index] = 0;
      psqsum16x16[mb_index] = 0;

      l_sad =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur;
      tmp_ref_row = tmp_ref;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 0] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;

      l_sad =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + 8;
      tmp_ref_row = tmp_ref + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 1] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;

      l_sad =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8;
      tmp_ref_row = tmp_ref + pic_stride_x8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 2] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;

      l_sad =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8 + 8;
      tmp_ref_row = tmp_ref + pic_stride_x8 + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 3] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;


      tmp_ref += 16;
      tmp_cur += 16;
      ++mb_index;
    }
    tmp_ref += step;
    tmp_cur += step;
  }
}

void VAACalcSad_ref (const uint8_t* pCurData, const uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                     int32_t iPicStride,
                     int32_t* pFrameSad, int32_t* pSad8x8) {
  const uint8_t* tmp_ref = pRefData;
  const uint8_t* tmp_cur = pCurData;
  int32_t iMbWidth = (iPicWidth >> 4);
  int32_t mb_height = (iPicHeight >> 4);
  int32_t mb_index = 0;
  int32_t pic_stride_x8 = iPicStride << 3;
  int32_t step = (iPicStride << 4) - iPicWidth;

  *pFrameSad = 0;
  for (int32_t i = 0; i < mb_height; i ++) {
    for (int32_t j = 0; j < iMbWidth; j ++) {
      int32_t k, l;
      int32_t l_sad;
      const uint8_t* tmp_cur_row;
      const uint8_t* tmp_ref_row;

      l_sad =  0;
      tmp_cur_row = tmp_cur;
      tmp_ref_row = tmp_ref;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 0] = l_sad;

      l_sad =  0;
      tmp_cur_row = tmp_cur + 8;
      tmp_ref_row = tmp_ref + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 1] = l_sad;

      l_sad =  0;
      tmp_cur_row = tmp_cur + pic_stride_x8;
      tmp_ref_row = tmp_ref + pic_stride_x8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 2] = l_sad;

      l_sad =  0;
      tmp_cur_row = tmp_cur + pic_stride_x8 + 8;
      tmp_ref_row = tmp_ref + pic_stride_x8 + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = WELS_ABS (tmp_cur_row[l] - tmp_ref_row[l]);
          l_sad += diff;
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 3] = l_sad;

      tmp_ref += 16;
      tmp_cur += 16;
      ++mb_index;
    }
    tmp_ref += step;
    tmp_cur += step;
  }
}

void VAACalcSadSsdBgd_ref (const uint8_t* pCurData, const uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                           int32_t iPicStride,
                           int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSum16x16, int32_t* psqsum16x16, int32_t* psqdiff16x16, int32_t* pSd8x8,
                           uint8_t* pMad8x8) {
  const uint8_t* tmp_ref = pRefData;
  const uint8_t* tmp_cur = pCurData;
  int32_t iMbWidth = (iPicWidth >> 4);
  int32_t mb_height = (iPicHeight >> 4);
  int32_t mb_index = 0;
  int32_t pic_stride_x8 = iPicStride << 3;
  int32_t step = (iPicStride << 4) - iPicWidth;

  *pFrameSad = 0;
  for (int32_t i = 0; i < mb_height; i ++) {
    for (int32_t j = 0; j < iMbWidth; j ++) {
      int32_t k, l;
      int32_t l_sad, l_sqdiff, l_sum, l_sqsum, l_sd, l_mad;
      const uint8_t* tmp_cur_row;
      const uint8_t* tmp_ref_row;

      pSum16x16[mb_index] = 0;
      psqsum16x16[mb_index] = 0;
      psqdiff16x16[mb_index] = 0;

      l_sd = l_mad = l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur;
      tmp_ref_row = tmp_ref;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);

          l_sd += diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
          l_sad += abs_diff;
          l_sqdiff += abs_diff * abs_diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 0] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;
      pSd8x8[ (mb_index << 2) + 0] = l_sd;
      pMad8x8[ (mb_index << 2) + 0] = l_mad;


      l_sd = l_mad = l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + 8;
      tmp_ref_row = tmp_ref + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);

          l_sd += diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
          l_sad += abs_diff;
          l_sqdiff += abs_diff * abs_diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 1] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;
      pSd8x8[ (mb_index << 2) + 1] = l_sd;
      pMad8x8[ (mb_index << 2) + 1] = l_mad;

      l_sd = l_mad = l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8;
      tmp_ref_row = tmp_ref + pic_stride_x8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);

          l_sd += diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
          l_sad += abs_diff;
          l_sqdiff += abs_diff * abs_diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 2] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;
      pSd8x8[ (mb_index << 2) + 2] = l_sd;
      pMad8x8[ (mb_index << 2) + 2] = l_mad;

      l_sd = l_mad = l_sad =  l_sqdiff =  l_sum =  l_sqsum = 0;
      tmp_cur_row = tmp_cur + pic_stride_x8 + 8;
      tmp_ref_row = tmp_ref + pic_stride_x8 + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);

          l_sd += diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
          l_sad += abs_diff;
          l_sqdiff += abs_diff * abs_diff;
          l_sum += tmp_cur_row[l];
          l_sqsum += tmp_cur_row[l] * tmp_cur_row[l];
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 3] = l_sad;
      pSum16x16[mb_index] += l_sum;
      psqsum16x16[mb_index] += l_sqsum;
      psqdiff16x16[mb_index] += l_sqdiff;
      pSd8x8[ (mb_index << 2) + 3] = l_sd;
      pMad8x8[ (mb_index << 2) + 3] = l_mad;

      tmp_ref += 16;
      tmp_cur += 16;
      ++mb_index;
    }
    tmp_ref += step;
    tmp_cur += step;
  }
}

void VAACalcSadBgd_ref (const uint8_t* pCurData, const uint8_t* pRefData, int32_t iPicWidth, int32_t iPicHeight,
                        int32_t iPicStride,
                        int32_t* pFrameSad, int32_t* pSad8x8, int32_t* pSd8x8, uint8_t* pMad8x8) {
  const uint8_t* tmp_ref = pRefData;
  const uint8_t* tmp_cur = pCurData;
  int32_t iMbWidth = (iPicWidth >> 4);
  int32_t mb_height = (iPicHeight >> 4);
  int32_t mb_index = 0;
  int32_t pic_stride_x8 = iPicStride << 3;
  int32_t step = (iPicStride << 4) - iPicWidth;

  *pFrameSad = 0;
  for (int32_t i = 0; i < mb_height; i ++) {
    for (int32_t j = 0; j < iMbWidth; j ++) {
      int32_t k, l;
      int32_t l_sad, l_sd, l_mad;
      const uint8_t* tmp_cur_row;
      const uint8_t* tmp_ref_row;

      l_mad = l_sd = l_sad =  0;
      tmp_cur_row = tmp_cur;
      tmp_ref_row = tmp_ref;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);
          l_sd += diff;
          l_sad += abs_diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 0] = l_sad;
      pSd8x8[ (mb_index << 2) + 0] = l_sd;
      pMad8x8[ (mb_index << 2) + 0] = l_mad;

      l_mad = l_sd = l_sad =  0;
      tmp_cur_row = tmp_cur + 8;
      tmp_ref_row = tmp_ref + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);
          l_sd += diff;
          l_sad += abs_diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 1] = l_sad;
      pSd8x8[ (mb_index << 2) + 1] = l_sd;
      pMad8x8[ (mb_index << 2) + 1] = l_mad;

      l_mad = l_sd = l_sad =  0;
      tmp_cur_row = tmp_cur + pic_stride_x8;
      tmp_ref_row = tmp_ref + pic_stride_x8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);
          l_sd += diff;
          l_sad += abs_diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 2] = l_sad;
      pSd8x8[ (mb_index << 2) + 2] = l_sd;
      pMad8x8[ (mb_index << 2) + 2] = l_mad;

      l_mad = l_sd = l_sad =  0;
      tmp_cur_row = tmp_cur + pic_stride_x8 + 8;
      tmp_ref_row = tmp_ref + pic_stride_x8 + 8;
      for (k = 0; k < 8; k ++) {
        for (l = 0; l < 8; l ++) {
          int32_t diff = tmp_cur_row[l] - tmp_ref_row[l];
          int32_t abs_diff = WELS_ABS (diff);
          l_sd += diff;
          l_sad += abs_diff;
          if (abs_diff > l_mad) {
            l_mad = abs_diff;
          }
        }
        tmp_cur_row += iPicStride;
        tmp_ref_row += iPicStride;
      }
      *pFrameSad += l_sad;
      pSad8x8[ (mb_index << 2) + 3] = l_sad;
      pSd8x8[ (mb_index << 2) + 3] = l_sd;
      pMad8x8[ (mb_index << 2) + 3] = l_mad;

      tmp_ref += 16;
      tmp_cur += 16;
      ++mb_index;
    }
    tmp_ref += step;
    tmp_cur += step;
  }
}

#define BUFFER_SIZE (320*320)

#define GENERATE_VAACalcSad_UT(func, ASM, CPUFLAGS) \
TEST (VAACalcFuncTest, func) { \
    if (ASM) {\
        int32_t iCpuCores = 0; \
        uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
        if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
            return; \
    } \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_c, BUFFER_SIZE/64, 16); \
    int32_t pic_width_c; \
    int32_t pic_height_c; \
    int32_t pic_stride_c; \
    int32_t psadframe_c; \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_a, BUFFER_SIZE/64, 16); \
    int32_t pic_width_a; \
    int32_t pic_height_a; \
    int32_t pic_stride_a; \
    int32_t psadframe_a; \
    for (int i=0; i<4; i++) { \
        pic_width_c  = pic_width_a = 320-16*i; \
        pic_height_c = pic_height_a = 320; \
        pic_stride_c = pic_stride_a = 320; \
        psadframe_c = psadframe_a = 0; \
        for (int j=0; j<BUFFER_SIZE; j++) { \
            cur_data_c[j] = cur_data_a[j] = (rand()%256); \
            ref_data_c[j] = ref_data_a[j] = (rand()%256); \
            psad8x8_c[j%(BUFFER_SIZE/64)] = psad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
        } \
        VAACalcSad_ref (cur_data_c, ref_data_c, pic_width_c, pic_height_c, pic_stride_c, &psadframe_c, psad8x8_c); \
        func (cur_data_a, ref_data_a, pic_width_a, pic_height_a, pic_stride_a, &psadframe_a, psad8x8_a); \
        ASSERT_EQ (psadframe_a, psadframe_c); \
        for (int j=0; j<(BUFFER_SIZE/64); j++) \
            ASSERT_EQ (psad8x8_a[j], psad8x8_c[j]); \
    } \
}


#define GENERATE_VAACalcSadBgd_UT(func, ASM, CPUFLAGS) \
TEST (VAACalcFuncTest, func) { \
    if (ASM) {\
        int32_t iCpuCores = 0; \
        uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
        if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
        return; \
    } \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psd8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, pmad8x8_c, BUFFER_SIZE/64, 16); \
    int32_t pic_width_c; \
    int32_t pic_height_c; \
    int32_t pic_stride_c; \
    int32_t psadframe_c; \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psd8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, pmad8x8_a, BUFFER_SIZE/64, 16); \
    int32_t pic_width_a; \
    int32_t pic_height_a; \
    int32_t pic_stride_a; \
    int32_t psadframe_a; \
    for (int i=0; i<4; i++) { \
        pic_width_c  = pic_width_a = 320-16*i; \
        pic_height_c = pic_height_a = 320; \
        pic_stride_c = pic_stride_a = 320; \
        psadframe_c = psadframe_a = 0; \
        for (int j=0; j<BUFFER_SIZE; j++) { \
            cur_data_c[j] = cur_data_a[j] = (rand()%256); \
            ref_data_c[j] = ref_data_a[j] = (rand()%256); \
            psad8x8_c[j%(BUFFER_SIZE/64)] = psad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
            psd8x8_c[j%(BUFFER_SIZE/64)]  = psd8x8_a[j%(BUFFER_SIZE/64)]  = (rand()%256); \
            pmad8x8_c[j%(BUFFER_SIZE/64)] = pmad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
        } \
        VAACalcSadBgd_ref (cur_data_c, ref_data_c, pic_width_c, pic_height_c, pic_stride_c, &psadframe_c, psad8x8_c, psd8x8_c, pmad8x8_c); \
        func (cur_data_a, ref_data_a, pic_width_a, pic_height_a, pic_stride_a, &psadframe_a, psad8x8_a, psd8x8_a, pmad8x8_a); \
        ASSERT_EQ (psadframe_a, psadframe_c); \
        for (int j=0; j<(BUFFER_SIZE/64); j++) {\
            ASSERT_EQ (psad8x8_a[j], psad8x8_c[j]); \
            ASSERT_EQ (psd8x8_a[j], psd8x8_c[j]); \
            ASSERT_EQ (pmad8x8_a[j], pmad8x8_c[j]); \
        } \
    } \
}

#define GENERATE_VAACalcSadSsd_UT(func, ASM, CPUFLAGS) \
TEST (VAACalcFuncTest, func) { \
    if (ASM) {\
        int32_t iCpuCores = 0; \
        uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
        if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
        return; \
    } \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_c, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_c, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqdiff16x16_c, BUFFER_SIZE/256, 16); \
    int32_t pic_width_c; \
    int32_t pic_height_c; \
    int32_t pic_stride_c; \
    int32_t psadframe_c; \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_a, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_a, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqdiff16x16_a, BUFFER_SIZE/256, 16); \
    int32_t pic_width_a; \
    int32_t pic_height_a; \
    int32_t pic_stride_a; \
    int32_t psadframe_a; \
    for (int i=0; i<4; i++) { \
        pic_width_c  = pic_width_a = 320-16*i; \
        pic_height_c = pic_height_a = 320; \
        pic_stride_c = pic_stride_a = 320; \
        psadframe_c = psadframe_a = 0; \
        for (int j=0; j<BUFFER_SIZE; j++) { \
            cur_data_c[j] = cur_data_a[j] = (rand()%256); \
            ref_data_c[j] = ref_data_a[j] = (rand()%256); \
            psad8x8_c[j%(BUFFER_SIZE/64)] = psad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
            psum16x16_c[j%(BUFFER_SIZE/256)]    = psum16x16_a[j%(BUFFER_SIZE/256)] = (rand()%256); \
            psqsum16x16_c[j%(BUFFER_SIZE/256)]  = psqsum16x16_a[j%(BUFFER_SIZE/256)]  = (rand()%256); \
            psqdiff16x16_c[j%(BUFFER_SIZE/256)] = psqdiff16x16_a[j%(BUFFER_SIZE/256)] = (rand()%256); \
        } \
        VAACalcSadSsd_ref (cur_data_c, ref_data_c, pic_width_c, pic_height_c, pic_stride_c, &psadframe_c, psad8x8_c, psum16x16_c, psqsum16x16_c, psqdiff16x16_c); \
        func (cur_data_a, ref_data_a, pic_width_a, pic_height_a, pic_stride_a, &psadframe_a, psad8x8_a, psum16x16_a, psqsum16x16_a, psqdiff16x16_a); \
        ASSERT_EQ (psadframe_a, psadframe_c); \
        for (int j=0; j<(BUFFER_SIZE/64); j++) {\
            ASSERT_EQ (psad8x8_a[j], psad8x8_c[j]); \
        } \
        for (int j=0; j<(BUFFER_SIZE/256); j++) {\
            ASSERT_EQ (psum16x16_a[j], psum16x16_c[j]); \
            ASSERT_EQ (psqsum16x16_a[j], psqsum16x16_c[j]); \
            ASSERT_EQ (psqdiff16x16_a[j], psqdiff16x16_c[j]); \
        } \
    } \
}

#define GENERATE_VAACalcSadVar_UT(func, ASM, CPUFLAGS) \
TEST (VAACalcFuncTest, func) { \
    if (ASM) {\
        int32_t iCpuCores = 0; \
        uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
        if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
        return; \
    } \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_c, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_c, BUFFER_SIZE/256, 16); \
    int32_t pic_width_c; \
    int32_t pic_height_c; \
    int32_t pic_stride_c; \
    int32_t psadframe_c; \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_a, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_a, BUFFER_SIZE/256, 16); \
    int32_t pic_width_a; \
    int32_t pic_height_a; \
    int32_t pic_stride_a; \
    int32_t psadframe_a; \
    for (int i=0; i<4; i++) { \
        pic_width_c  = pic_width_a = 320-16*i; \
        pic_height_c = pic_height_a = 320; \
        pic_stride_c = pic_stride_a = 320; \
        psadframe_c = psadframe_a = 0; \
        for (int j=0; j<BUFFER_SIZE; j++) { \
            cur_data_c[j] = cur_data_a[j] = (rand()%256); \
            ref_data_c[j] = ref_data_a[j] = (rand()%256); \
            psad8x8_c[j%(BUFFER_SIZE/64)] = psad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
            psum16x16_c[j%(BUFFER_SIZE/256)]    = psum16x16_a[j%(BUFFER_SIZE/256)] = (rand()%256); \
            psqsum16x16_c[j%(BUFFER_SIZE/256)]  = psqsum16x16_a[j%(BUFFER_SIZE/256)]  = (rand()%256); \
        } \
        VAACalcSadVar_ref (cur_data_c, ref_data_c, pic_width_c, pic_height_c, pic_stride_c, &psadframe_c, psad8x8_c, psum16x16_c, psqsum16x16_c); \
        func (cur_data_a, ref_data_a, pic_width_a, pic_height_a, pic_stride_a, &psadframe_a, psad8x8_a, psum16x16_a, psqsum16x16_a); \
        ASSERT_EQ (psadframe_a, psadframe_c); \
        for (int j=0; j<(BUFFER_SIZE/64); j++) {\
            ASSERT_EQ (psad8x8_a[j], psad8x8_c[j]); \
        } \
        for (int j=0; j<(BUFFER_SIZE/256); j++) {\
            ASSERT_EQ (psum16x16_a[j], psum16x16_c[j]); \
            ASSERT_EQ (psqsum16x16_a[j], psqsum16x16_c[j]); \
        } \
    } \
}

#define GENERATE_VAACalcSadSsdBgd_UT(func, ASM, CPUFLAGS) \
TEST (VAACalcFuncTest, func) { \
    if (ASM) {\
        int32_t iCpuCores = 0; \
        uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
        if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
        return; \
    } \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_c, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psd8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, pmad8x8_c, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_c, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_c, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqdiff16x16_c, BUFFER_SIZE/256, 16); \
    int32_t pic_width_c; \
    int32_t pic_height_c; \
    int32_t pic_stride_c; \
    int32_t psadframe_c; \
    ENFORCE_STACK_ALIGN_1D (uint8_t, cur_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, ref_data_a, BUFFER_SIZE, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psad8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psd8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (uint8_t, pmad8x8_a, BUFFER_SIZE/64, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psum16x16_a, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqsum16x16_a, BUFFER_SIZE/256, 16); \
    ENFORCE_STACK_ALIGN_1D (int32_t, psqdiff16x16_a, BUFFER_SIZE/256, 16); \
    int32_t pic_width_a; \
    int32_t pic_height_a; \
    int32_t pic_stride_a; \
    int32_t psadframe_a; \
    for (int i=0; i<4; i++) { \
        pic_width_c  = pic_width_a = 320-16*i; \
        pic_height_c = pic_height_a = 320; \
        pic_stride_c = pic_stride_a = 320; \
        psadframe_c = psadframe_a = 0; \
        for (int j=0; j<BUFFER_SIZE; j++) { \
            cur_data_c[j] = cur_data_a[j] = (rand()%256); \
            ref_data_c[j] = ref_data_a[j] = (rand()%256); \
            psad8x8_c[j%(BUFFER_SIZE/64)] = psad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
            psd8x8_c[j%(BUFFER_SIZE/64)]  = psd8x8_a[j%(BUFFER_SIZE/64)]  = (rand()%256); \
            pmad8x8_c[j%(BUFFER_SIZE/64)] = pmad8x8_a[j%(BUFFER_SIZE/64)] = (rand()%256); \
            psum16x16_c[j%(BUFFER_SIZE/256)]    = psum16x16_a[j%(BUFFER_SIZE/256)] = (rand()%256); \
            psqsum16x16_c[j%(BUFFER_SIZE/256)]  = psqsum16x16_a[j%(BUFFER_SIZE/256)]  = (rand()%256); \
            psqdiff16x16_c[j%(BUFFER_SIZE/256)] = psqdiff16x16_a[j%(BUFFER_SIZE/256)] = (rand()%256); \
        } \
        VAACalcSadSsdBgd_ref (cur_data_c, ref_data_c, pic_width_c, pic_height_c, pic_stride_c, &psadframe_c, psad8x8_c, psum16x16_c, psqsum16x16_c, psqdiff16x16_c, psd8x8_c, pmad8x8_c); \
        func (cur_data_a, ref_data_a, pic_width_a, pic_height_a, pic_stride_a, &psadframe_a, psad8x8_a, psum16x16_a, psqsum16x16_a, psqdiff16x16_a, psd8x8_a, pmad8x8_a); \
        ASSERT_EQ (psadframe_a, psadframe_c); \
        for (int j=0; j<(BUFFER_SIZE/64); j++) {\
            ASSERT_EQ (psad8x8_a[j], psad8x8_c[j]); \
            ASSERT_EQ (psd8x8_a[j], psd8x8_c[j]); \
            ASSERT_EQ (pmad8x8_a[j], pmad8x8_c[j]); \
        } \
        for (int j=0; j<(BUFFER_SIZE/256); j++) {\
            ASSERT_EQ (psum16x16_a[j], psum16x16_c[j]); \
            ASSERT_EQ (psqsum16x16_a[j], psqsum16x16_c[j]); \
            ASSERT_EQ (psqdiff16x16_a[j], psqdiff16x16_c[j]); \
        } \
    } \
}

GENERATE_VAACalcSad_UT (VAACalcSad_c, 0, 0)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_c, 0, 0)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_c, 0, 0)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_c, 0, 0)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_c, 0, 0)
#if defined(X86_ASM)
GENERATE_VAACalcSad_UT (VAACalcSad_sse2, 1, WELS_CPU_SSE2)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_sse2, 1, WELS_CPU_SSE2)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_sse2, 1, WELS_CPU_SSE2)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_sse2, 1, WELS_CPU_SSE2)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_sse2, 1, WELS_CPU_SSE2)

#if defined(HAVE_AVX2)
GENERATE_VAACalcSad_UT (VAACalcSad_avx2, 1, WELS_CPU_AVX2)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_avx2, 1, WELS_CPU_AVX2)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_avx2, 1, WELS_CPU_AVX2)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_avx2, 1, WELS_CPU_AVX2)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_avx2, 1, WELS_CPU_AVX2)
#endif //HAVE_AVX2
#endif

#if defined(HAVE_NEON)
GENERATE_VAACalcSad_UT (VAACalcSad_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_neon, 1, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_VAACalcSad_UT (VAACalcSad_AArch64_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_AArch64_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_AArch64_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_AArch64_neon, 1, WELS_CPU_NEON)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_AArch64_neon, 1, WELS_CPU_NEON)
#endif

#if defined(HAVE_MMI)
GENERATE_VAACalcSad_UT (VAACalcSad_mmi, 1, WELS_CPU_MMI)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_mmi, 1, WELS_CPU_MMI)
GENERATE_VAACalcSadSsdBgd_UT (VAACalcSadSsdBgd_mmi, 1, WELS_CPU_MMI)
GENERATE_VAACalcSadSsd_UT (VAACalcSadSsd_mmi, 1, WELS_CPU_MMI)
GENERATE_VAACalcSadVar_UT (VAACalcSadVar_mmi, 1, WELS_CPU_MMI)
#endif

#if defined(HAVE_LSX)
GENERATE_VAACalcSadBgd_UT (VAACalcSadBgd_lsx, 1, WELS_CPU_LSX)
#endif
