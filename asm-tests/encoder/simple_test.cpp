#include <gtest/gtest.h>
#if defined (WIN32)
#include <windows.h>
#include <tchar.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


int main (int argc, char** argv) {
  testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS();
}
