#include <string.h>//memset/memcpy
#include "utils/DataGenerator.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"

using namespace std;


bool YUVPixelDataGenerator (uint8_t* pPointer, int32_t iWidth, int32_t iHeight, int32_t iStride) {
#define SRC_FRAME_WIDTH (160)
#define SRC_FRAME_HEIGHT (96)

  if (SRC_FRAME_WIDTH - iWidth <= 0 || SRC_FRAME_HEIGHT - iHeight <= 0) {
    return false;
  }

  const int32_t kiFrameSize = SRC_FRAME_WIDTH * SRC_FRAME_HEIGHT;
  BufferedData sBuf;
  sBuf.SetLength (kiFrameSize);
  if (sBuf.Length() != (size_t)kiFrameSize) { //include memory fail (-1) case
    return false;
  }

  FileInputStream fileStream;
#if defined(ANDROID_NDK)
  if (!fileStream.Open ("/sdcard/res/CiscoVT2people_160x96_6fps.yuv")) {
#else
  if (!fileStream.Open ("res/CiscoVT2people_160x96_6fps.yuv")) {
#endif
    return false;
  }
  if (fileStream.read (sBuf.data(), kiFrameSize) == kiFrameSize) {
    int32_t iStartPosX = rand() % (SRC_FRAME_WIDTH - iWidth);
    int32_t iStartPosY = rand() % (SRC_FRAME_HEIGHT - iHeight);
    uint8_t* pSrcPointer = sBuf.data() + iStartPosX + iStartPosY * SRC_FRAME_WIDTH;
    uint8_t* pLocalPointer = pPointer;

    for (int j = 0; j < iHeight; j++) {
      memcpy (pLocalPointer, pSrcPointer, iWidth * sizeof (uint8_t));
      pLocalPointer += iStride;
      pSrcPointer += SRC_FRAME_WIDTH;
    }
    return true;
  }
  return false;
}

void RandomPixelDataGenerator (uint8_t* pPointer, int32_t iWidth, int32_t iHeight, int32_t iStride) {
  uint8_t* pLocalPointer = pPointer;
  for (int32_t j = 0; j < iHeight; j++) {
    for (int32_t i = 0; i < iWidth; i++) {
      pLocalPointer[i] = rand() % 256;
    }
    pLocalPointer += iStride;
  }
}


void RandomResidueDataGenerator (uint16_t* pPointer, int32_t iWidth, int32_t iHeight, int32_t iStride) {
}

void RandomCoeffDataGenerator (uint16_t* pPointer, int32_t iWidth, int32_t iHeight, int32_t iStride) {
}

