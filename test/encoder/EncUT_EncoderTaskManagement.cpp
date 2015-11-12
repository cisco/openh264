#include <gtest/gtest.h>

#include "utils/DataGenerator.h"
#include "encoder_context.h"
#include "wels_task_management.h"

using namespace WelsEnc;


TEST (EncoderTaskManagement, CWelsTaskManageBase) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sCtx.iMaxSliceCount = 35;
  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 1, false);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}

TEST (EncoderTaskManagement, CWelsTaskManageParallel) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sCtx.iMaxSliceCount = 35;
  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 1, true);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}

TEST (EncoderTaskManagement, CWelsTaskManageMultiD) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sWelsSvcCodingParam.sSpatialLayers[0].sSliceArgument.uiSliceNum = 35;
  sCtx.iMaxSliceCount = 35;

  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 4, true);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}