#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <android/log.h>

#define LOG_TAG "codec_unittest"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

int CodecUtMain(int argc, char** argv); 
extern "C"
JNIEXPORT void JNICALL Java_com_cisco_codec_unittest_MainActivity_DoUnittest 
(JNIEnv* env, jobject thiz,jstring jspath) {
  /**************** Add the native codes/API *****************/
  char* argv[2];
  int  argc = 2;
  argv[0] = (char*) ("codec_unittest.exe");
  
  argv[1] = (char*) ((*env).GetStringUTFChars (jspath,NULL));
  LOGI ("PATH:",+argv[1]);  
  LOGI ("Start to run JNI module!+++");
 CodecUtMain(argc,argv);
  LOGI ("End to run JNI module!+++");
}


