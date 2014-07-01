#include <gtest/gtest.h>
#include <stdlib.h>
#include <time.h>
#if defined (ANDROID_NDK)
#include <stdio.h>
#endif


#if (defined(ANDROID_NDK)||defined(APPLE_IOS))
int CodecUtMain (int argc , char** argv) {
#else
int main (int argc, char** argv) {
#endif

#if (defined(ANDROID_NDK)||defined(APPLE_IOS))
  char xmlPath[1024] = "";
  sprintf (xmlPath, "xml:%s", argv[1]);
  ::testing::GTEST_FLAG (output) = xmlPath;
#endif
  srand ((unsigned int)time (NULL));
  ::testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS();
}
