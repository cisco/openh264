#include <string.h>
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>

#define LOG_TAG "welsdec"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" int EncMain (int argc, char* argv[]);
extern "C"
JNIEXPORT void JNICALL Java_com_wels_enc_WelsEncTest_DoEncoderTest
(JNIEnv* env, jobject thiz, jstring jsFileNameIn) {
  /**************** Add the native codes/API *****************/
  char* argv[2];
  int  argc = 2;
  argv[0] = (char*) ("decConsole.exe");
  argv[1] = (char*) ((*env).GetStringUTFChars (jsFileNameIn, NULL));
  LOGI ("Start to run JNI module!+++");
  EncMain (argc, argv);
  LOGI ("End to run JNI module!+++");

}


