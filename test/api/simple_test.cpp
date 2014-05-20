#include <gtest/gtest.h>
#if defined (ANDROID_NDK)
#include <stdio.h>
#endif



#if defined(ANDROID_NDK)
int CodecUtMain(int argc , char** argv ) {
#else
int main (int argc, char** argv) {
#endif

#if defined(ANDROID_NDK)   
   char xmlPath[1024] = "";
   sprintf(xmlPath,"xml:%s",argv[1]);
  ::testing::GTEST_FLAG(output) = xmlPath;
#endif
  ::testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS();
}
