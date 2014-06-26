#ifndef __HASHFUNCTIONS_H__
#define __HASHFUNCTIONS_H__

#include <stdio.h>
#include <string.h>
#include <gtest/gtest.h>
#include "../sha1.h"

static void CompareHash (const unsigned char* digest, const char* hashStr) {
  char hashStrCmp[SHA_DIGEST_LENGTH * 2 + 1];
  for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
    sprintf (&hashStrCmp[i * 2], "%.2x", digest[i]);
  }
  hashStrCmp[SHA_DIGEST_LENGTH * 2] = '\0';
  EXPECT_STREQ (hashStr, hashStrCmp);
}

#endif //__HASHFUNCTIONS_H__
