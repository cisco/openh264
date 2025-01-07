#include <gtest/gtest.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>


#if (defined(ANDROID_NDK)||defined(APPLE_IOS)||defined(WINDOWS_PHONE))
int CodecUtMain (int argc , char** argv) {
#else
int main (int argc, char** argv) {
#endif

#if (defined(ANDROID_NDK)||defined(APPLE_IOS)||defined(WINDOWS_PHONE))
  char xmlPath[1024] = "";
  snprintf (xmlPath, sizeof(xmlPath), "xml:%s", argv[1]);
  ::testing::GTEST_FLAG (output) = xmlPath;
#endif
  ::testing::InitGoogleTest (&argc, argv);
  unsigned int seed = (unsigned int) time (NULL);
  if (argc >= 2 && !strncmp (argv[1], "--seed=", 7))
    seed = atoi (argv[1] + 7);
  printf ("Random seed: %u\n", seed);
  srand (seed);

  return RUN_ALL_TESTS();
}
