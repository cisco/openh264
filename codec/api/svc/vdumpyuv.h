#ifndef VACINGFANG_DUMPYUV_H_20210618
#define VACINGFANG_DUMPYUV_H_20210618
#include <stdio.h>

/**
 * dump yuv buffer to yuv file
 */
void inline dumpyuv420(unsigned char* const* pixs, int w, int h, const int *ws,
                       const char *fpath) {
  FILE *sdfd = fopen(fpath, "wb");
  if (!sdfd) {
    printf("\033[32;1m %s open [%s] failed \033[0m\n", __FUNCTION__, fpath);
    return;
  }

  for (int plane = 0; plane < 3; plane++) {
    const unsigned char *p = pixs[plane];
    if (!p) break;

    int wsl = ws[plane];
    int wl = plane? w/2:w;
    int hl = plane? h/2:h;
    for (int i = 0; i < hl; i++) {
      fwrite(p, wl, 1, sdfd);
      p += wsl;   // skip stride length
    }
  }
  fclose(sdfd);
}

#endif

