#include <gtest/gtest.h>
#include "utils/FileInputStream.h"
#include "svc_encode_slice.h"

#define MAX_WIDTH  (4096)
#define MAX_HEIGH  (2304)

class CSliceBufferReallocatTest : public ::testing::Test { //WithParamInterface<EncodeFileParam>{
public:
    virtual void SetUp() {
        m_pEncoder   = NULL;
        int32_t iRet = WelsCreateSVCEncoder(&m_pEncoder);
        ASSERT_EQ(0, iRet);
        ASSERT_TRUE(m_pEncoder != NULL);

        int32_t iCacheLineSize = 16;
        m_EncContext.pMemAlign = new CMemoryAlign(iCacheLineSize);
        ASSERT_TRUE(NULL != m_EncContext.pMemAlign);

        SWelsSvcCodingParam* pCodingParam = (SWelsSvcCodingParam*)m_EncContext.pMemAlign->WelsMalloc(sizeof(SWelsSvcCodingParam),
                                            "SWelsSvcCodingParam");
        ASSERT_TRUE(NULL != pCodingParam);
        m_EncContext.pSvcParam = pCodingParam;

        iRet = m_pEncoder->GetDefaultParams(m_EncContext.pSvcParam);
        ASSERT_EQ(0, iRet);
    }

    virtual void TearDown() {
        WelsDestroySVCEncoder(m_pEncoder);
        m_pEncoder = NULL;
        if (NULL != m_EncContext.pSvcParam) {
            m_EncContext.pMemAlign->WelsFree(m_EncContext.pSvcParam, "SWelsSvcCodingParam");
            m_EncContext.pSvcParam = NULL;
        }

        if (m_EncContext.pMemAlign != NULL)  {
            delete m_EncContext.pMemAlign;
            m_EncContext.pMemAlign = NULL;
        }
    }

    void SimulateEncodedOneSlice(const int32_t kiSlcIdx, const int32_t kiThreadIdx);
    void SimulateSliceInOnePartition(const int32_t kiPartNum,  const int32_t kiPartIdx, const int32_t kiSlcNumInPart);
    void SimulateSliceInOneLayer();

    void InitParamForTestCase(int32_t iLayerIdx);
    void InitParamForSizeLimitSlcModeCase(int32_t iLayerIdx);
    void UnInitParamForTestCase(int32_t iLayerIdx);

    void InitParam();
    void UnInitParam();

    void InitFrameBsBuffer();
    void UnInitFrameBsBuffer();

    void InitLayerSliceBuffer(const int32_t iLayerIdx);
    void UnInitLayerSliceBuffer(const int32_t iLayerIdx);

    ISVCEncoder*  m_pEncoder;
    sWelsEncCtx   m_EncContext;

private:

};
