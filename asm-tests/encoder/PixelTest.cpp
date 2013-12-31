#include<gtest/gtest.h>
#include<math.h>
#include<stdlib.h>

#include "cpu_core.h"
#include "sample.h"
using namespace WelsSVCEnc;
TEST(GetIntraPredictorTest, TestPixel_sad_4x4)
{
	const int32_t stride_pix_a = rand()%256+4;
	const int32_t stride_pix_b = rand()%256+4;

	uint8_t* src_a = new uint8_t[stride_pix_a<<2];
	uint8_t* src_b = new uint8_t[stride_pix_b<<2];

	for(int i=0;i<(stride_pix_a<<2);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<2);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t sum_sad = 0;
	for (int i = 0; i < 4; i++ )
	{
		for(int j=0;j<4;j++)
			sum_sad+=abs(pix_a[j]-pix_b[j]);

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	ASSERT_EQ(WelsSampleSad4x4_c(src_a, stride_pix_a, src_b, stride_pix_b ),sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_8x8)
{
	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t sum_sad = 0;
	for (int i = 0; i < 8; i++ )
	{
		for(int j=0;j<8;j++)
			sum_sad+=abs(pix_a[j]-pix_b[j]);

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	ASSERT_EQ(WelsSampleSad8x8_c(src_a, stride_pix_a, src_b, stride_pix_b ),sum_sad);

	delete []src_a;
	delete []src_b;
}


TEST(GetIntraPredictorTest, TestPixel_sad_16x8)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<16;j++)
			sum_sad+=abs(pix_a[j]-pix_b[j]);

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	ASSERT_EQ(WelsSampleSad16x8_c(src_a, stride_pix_a, src_b, stride_pix_b ),sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_8x16)
{
	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<8;j++)
			sum_sad+=abs(pix_a[j]-pix_b[j]);

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	ASSERT_EQ(WelsSampleSad8x16_c(src_a, stride_pix_a, src_b, stride_pix_b ),sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_16x16)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<16;j++)
			sum_sad+=abs(pix_a[j]-pix_b[j]);

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	ASSERT_EQ(WelsSampleSad16x16_c(src_a, stride_pix_a, src_b, stride_pix_b ),sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSad4x4_mmx)
{
	const int32_t stride_pix_a = rand()%256+4;
	const int32_t stride_pix_b = rand()%256+4;

	uint8_t* src_a = new uint8_t[stride_pix_a<<2];
	uint8_t* src_b = new uint8_t[stride_pix_b<<2];

	for(int i=0;i<(stride_pix_a<<2);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<2);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSad4x4_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSad4x4_mmx(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}


TEST(GetIntraPredictorTest, WelsSampleSad8x8_sse21)
{
	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	int32_t tmpa, tmpb;

	CMemoryAlign cMemoryAlign(0);

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<4,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_b<<4,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;

	tmpa = WelsSampleSad8x8_c(src_a, stride_pix_a, src_b, stride_pix_b);
	tmpb = WelsSampleSad8x8_sse21(src_a, stride_pix_a, src_b, stride_pix_b);

	ASSERT_EQ(tmpa, tmpb);

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
}

TEST(GetIntraPredictorTest, WelsSampleSad8x16_sse2)
{
	const int32_t stride_pix_a = 32;
	const int32_t stride_pix_b = 32;

	CMemoryAlign cMemoryAlign(0);

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<4,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_b<<4,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSad8x16_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSad8x16_sse2(src_a, stride_pix_a, src_b, stride_pix_b));

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
}

TEST(GetIntraPredictorTest, WelsSampleSad16x8_sse2)
{
	const int32_t stride_pix_a = 32;
	const int32_t stride_pix_b = 32;

	CMemoryAlign cMemoryAlign(0);

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<4,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_b<<4,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSad16x8_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSad16x8_sse2(src_a, stride_pix_a, src_b, stride_pix_b));

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
}

TEST(GetIntraPredictorTest, WelsSampleSad16x16_sse2)
{
	const int32_t stride_pix_a = 32;
	const int32_t stride_pix_b = 32;

	int32_t tmpa, tmpb;

	CMemoryAlign cMemoryAlign(0);

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<4,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_b<<4,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	tmpa = WelsSampleSad16x16_c(src_a, stride_pix_a, src_b, stride_pix_b);
	tmpb = WelsSampleSad16x16_sse2(src_a, stride_pix_a, src_b, stride_pix_b);
	ASSERT_EQ(tmpa, tmpb);

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
}


TEST(GetIntraPredictorTest, TestPixel_satd_4x4)
{
	const int32_t stride_pix_a = rand()%256+4;
	const int32_t stride_pix_b = rand()%256+4;

	uint8_t* src_a = new uint8_t[stride_pix_a<<2];
	uint8_t* src_b = new uint8_t[stride_pix_b<<2];

	for(int i=0;i<(stride_pix_a<<2);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<2);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b;

	int32_t W[16],T[16],Y[16],k=0;
	for(int i=0;i<4;i++)
	{	
		for(int j=0;j<4;j++)
			W[k++]=pix_a[j]-pix_b[j];
		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	T[0]=W[0]+W[4]+W[8]+W[12];
	T[1]=W[1]+W[5]+W[9]+W[13];
	T[2]=W[2]+W[6]+W[10]+W[14];
	T[3]=W[3]+W[7]+W[11]+W[15];

	T[4]=W[0]+W[4]-W[8]-W[12];
	T[5]=W[1]+W[5]-W[9]-W[13];
	T[6]=W[2]+W[6]-W[10]-W[14];
	T[7]=W[3]+W[7]-W[11]-W[15];

	T[8]=W[0]-W[4]-W[8]+W[12];
	T[9]=W[1]-W[5]-W[9]+W[13];
	T[10]=W[2]-W[6]-W[10]+W[14];
	T[11]=W[3]-W[7]-W[11]+W[15];

	T[12]=W[0]-W[4]+W[8]-W[12];
	T[13]=W[1]-W[5]+W[9]-W[13];
	T[14]=W[2]-W[6]+W[10]-W[14];
	T[15]=W[3]-W[7]+W[11]-W[15];

	Y[0]=T[0]+T[1]+T[2]+T[3];
	Y[1]=T[0]+T[1]-T[2]-T[3];
	Y[2]=T[0]-T[1]-T[2]+T[3];
	Y[3]=T[0]-T[1]+T[2]-T[3];

	Y[4]=T[4]+T[5]+T[6]+T[7];
	Y[5]=T[4]+T[5]-T[6]-T[7];
	Y[6]=T[4]-T[5]-T[6]+T[7];
	Y[7]=T[4]-T[5]+T[6]-T[7];

	Y[8]=T[8]+T[9]+T[10]+T[11];
	Y[9]=T[8]+T[9]-T[10]-T[11];
	Y[10]=T[8]-T[9]-T[10]+T[11];
	Y[11]=T[8]-T[9]+T[10]-T[11];

	Y[12]=T[12]+T[13]+T[14]+T[15];
	Y[13]=T[12]+T[13]-T[14]-T[15];
	Y[14]=T[12]-T[13]-T[14]+T[15];
	Y[15]=T[12]-T[13]+T[14]-T[15];

	int32_t sum_satd = 0;
	for(int i=0;i<16;i++)
		sum_satd+=abs(Y[i]);

	ASSERT_EQ(WelsSampleSatd4x4_c(src_a, stride_pix_a, src_b, stride_pix_b ),(sum_satd+1)>>1 );

	delete []src_a;
	delete []src_b;

}


TEST(GetIntraPredictorTest, WelsSampleSatd4x4_sse2)
{
	const int32_t stride_pix_a = rand()%256+4;
	const int32_t stride_pix_b = rand()%256+4;

	uint8_t* src_a = new uint8_t[stride_pix_a<<2];
	uint8_t* src_b = new uint8_t[stride_pix_b<<2];

	for(int i=0;i<(stride_pix_a<<2);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<2);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd4x4_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd4x4_sse2(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}


TEST(GetIntraPredictorTest, WelsSampleSatd8x8_sse2)
{
	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd8x8_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd8x8_sse2(src_a, stride_pix_a, src_b, stride_pix_b ));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd8x16_sse2)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	int32_t tmpa, tmpb;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	tmpa = WelsSampleSatd8x16_c(src_a, stride_pix_a, src_b, stride_pix_b);
	tmpb = WelsSampleSatd8x16_sse2(src_a, stride_pix_a, src_b, stride_pix_b);
	ASSERT_EQ(tmpa, tmpb);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd16x8_sse2)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd16x8_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd16x8_sse2(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd16x16_sse2)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd16x16_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd16x16_sse2(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd4x4_sse41)
{
	const int32_t stride_pix_a = rand()%256+4;
	const int32_t stride_pix_b = rand()%256+4;

	uint8_t* src_a = new uint8_t[stride_pix_a<<2];
	uint8_t* src_b = new uint8_t[stride_pix_b<<2];

	for(int i=0;i<(stride_pix_a<<2);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<2);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd4x4_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd4x4_sse41(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}


TEST(GetIntraPredictorTest, WelsSampleSatd8x8_sse41)
{
	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd8x8_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd8x8_sse41(src_a, stride_pix_a, src_b, stride_pix_b ));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd8x16_sse41)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	int32_t tmpa, tmpb;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	tmpa = WelsSampleSatd8x16_c(src_a, stride_pix_a, src_b, stride_pix_b);
	tmpb = WelsSampleSatd8x16_sse41(src_a, stride_pix_a, src_b, stride_pix_b);
	ASSERT_EQ(tmpa, tmpb);

	//ASSERT_EQ(WelsSampleSatd8x16_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd8x16_sse41(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd16x8_sse41)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd16x8_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd16x8_sse41(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSatd16x16_sse41)
{
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;

	ASSERT_EQ(WelsSampleSatd16x16_c(src_a, stride_pix_a, src_b, stride_pix_b), WelsSampleSatd16x16_sse41(src_a, stride_pix_a, src_b, stride_pix_b));

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_16x16)
{    
    SWelsFuncPtrList pListC;
    WelsInitSampleSadFunc( &pListC, 0 );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_16x8)
{    
    SWelsFuncPtrList pListC;
    WelsInitSampleSadFunc( &pListC, 0 );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_8x16)
{    
    SWelsFuncPtrList pListC;
    WelsInitSampleSadFunc( &pListC, 0 );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_8x8)
{
    SWelsFuncPtrList pListC;
    WelsInitSampleSadFunc( &pListC, 0 );

	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_4x4)
{
    SWelsFuncPtrList pListC;
    WelsInitSampleSadFunc( &pListC, 0 );

	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <4; i++ )
	{
		for(int j=0;j<4;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_16x16_mmxext)
{
    SWelsFuncPtrList pListMMX;
    WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_16x8_mmxext)
{    
    SWelsFuncPtrList pListMMX;
    WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_8x16_mmxext)
{  
    SWelsFuncPtrList pListMMX;
    WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, TestPixel_sad_4_8x8_mmxext)
{    
    SWelsFuncPtrList pListMMX;
    WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}


TEST(GetIntraPredictorTest, TestPixel_sad_4_4x4_mmxext)
{       
    SWelsFuncPtrList pListMMX;
    WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <4; i++ )
	{
		for(int j=0;j<4;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t sad[4];
	pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
}

TEST(GetIntraPredictorTest, WelsSampleSadFour16x16_sse2)
{
    SWelsFuncPtrList pListSSE2;
    WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

	CMemoryAlign cMemoryAlign(0);

	const int32_t stride_pix_a = 32;
	const int32_t stride_pix_b = 32;

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<5,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<5,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t *sad = (int32_t *)cMemoryAlign.WelsMalloc(4 * sizeof(int32_t) ,"Sad");
	pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
	cMemoryAlign.WelsFree(sad, "Sad");
}

TEST(GetIntraPredictorTest, WelsSampleSadFour16x8_sse2)
{    
    SWelsFuncPtrList pListSSE2;
    WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

	CMemoryAlign cMemoryAlign(0);

	const int32_t stride_pix_a = 32;
	const int32_t stride_pix_b = 32;

	uint8_t* src_a = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<5,"Sad_src_a");		
	uint8_t* src_b = (uint8_t *)cMemoryAlign.WelsMalloc(stride_pix_a<<5,"Sad_src_b");

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<16;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t *sad = (int32_t *)cMemoryAlign.WelsMalloc(4 * sizeof(int32_t),"Sad");
	pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	cMemoryAlign.WelsFree(src_a,"Sad_src_a");
	cMemoryAlign.WelsFree(src_b,"Sad_src_b");
	cMemoryAlign.WelsFree(sad, "Sad");
}

TEST(GetIntraPredictorTest, WelsSampleSadFour8x16_sse2)
{  
    SWelsFuncPtrList pListSSE2;
    WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

	CMemoryAlign cMemoryAlign(0);
	const int32_t stride_pix_a = rand()%256+32;
	const int32_t stride_pix_b = rand()%256+32;

	uint8_t* src_a = new uint8_t[stride_pix_a<<5];
	uint8_t* src_b = new uint8_t[stride_pix_b<<5];

	for(int i=0;i<(stride_pix_a<<5);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<5);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <16; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t *sad = (int32_t *)cMemoryAlign.WelsMalloc(4 * sizeof(int32_t),"Sad");
	pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
	cMemoryAlign.WelsFree(sad, "Sad");
}

TEST(GetIntraPredictorTest, WelsSampleSadFour8x8_sse2)
{    
    SWelsFuncPtrList pListSSE2;
    WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

	CMemoryAlign cMemoryAlign(0);
	const int32_t stride_pix_a = rand()%256+16;
	const int32_t stride_pix_b = rand()%256+16;

	uint8_t* src_a = new uint8_t[stride_pix_a<<4];
	uint8_t* src_b = new uint8_t[stride_pix_b<<4];

	for(int i=0;i<(stride_pix_a<<4);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<4);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <8; i++ )
	{
		for(int j=0;j<8;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t *sad = (int32_t *)cMemoryAlign.WelsMalloc(4 * sizeof(int32_t),"Sad");
	pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
	cMemoryAlign.WelsFree(sad, "Sad");
}


TEST(GetIntraPredictorTest, WelsSampleSadFour4x4_sse2)
{       
    SWelsFuncPtrList pListSSE2;
    WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

	CMemoryAlign cMemoryAlign(0);

	const int32_t stride_pix_a = rand()%256+8;
	const int32_t stride_pix_b = rand()%256+8;

	uint8_t* src_a = new uint8_t[stride_pix_a<<3];
	uint8_t* src_b = new uint8_t[stride_pix_b<<3];

	for(int i=0;i<(stride_pix_a<<3);i++)
		src_a[i]=rand()%256;
	for(int i=0;i<(stride_pix_b<<3);i++)
		src_b[i]=rand()%256;
	uint8_t *pix_a=src_a;
	uint8_t *pix_b=src_b+stride_pix_b;

	int32_t sum_sad = 0;
	for (int i = 0; i <4; i++ )
	{
		for(int j=0;j<4;j++)
		{
			sum_sad+=abs(pix_a[j]-pix_b[j-1]);
			sum_sad+=abs(pix_a[j]-pix_b[j+1]);
			sum_sad+=abs(pix_a[j]-pix_b[j-stride_pix_b]);
			sum_sad+=abs(pix_a[j]-pix_b[j+stride_pix_b]);
		}

		pix_a += stride_pix_a;
		pix_b += stride_pix_b;
	}

	int32_t *sad = (int32_t *)cMemoryAlign.WelsMalloc(4 * sizeof(int32_t),"Sad");
	pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](src_a, stride_pix_a, src_b+stride_pix_b, stride_pix_b, sad);
	ASSERT_EQ(sad[0]+sad[1]+sad[2]+sad[3],sum_sad);

	delete []src_a;
	delete []src_b;
	cMemoryAlign.WelsFree(sad, "Sad");
}

