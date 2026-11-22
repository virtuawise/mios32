#include <stdio.h>
#include <string.h>
#include "oled_gui2.h"
static int rle_encode(const u8 *in, int n, u8 *out)
{
  int i = 0, o = 0;
  while( i < n ) {
    int run = 1;
    while( i+run < n && in[i+run] == in[i] && run < 129 ) ++run;
    if( run >= 2 ) {
      out[o++] = 126 + run;     // 128..255
      out[o++] = in[i];
      i += run;
    } else {
      int start = i, len = 0;
      while( i < n && len < 128 && !(i+1 < n && in[i] == in[i+1]) ) { ++i; ++len; }
      if( len == 0 ) { ++i; len = 1; }
      out[o++] = len - 1;
      memcpy(&out[o], &in[start], len);
      o += len;
    }
  }
  return o;
}


int main(void){
  og2_step_t s={0}; s.note=40; s.vel=100; s.len=60; s.prob=50; s.on=1; s.acc=1; s.value=100;
  static u8 raw[2048], enc[2112];
  OG2_RenderCell(raw, 2, &s, 36, 48, 1, 0, "VEL");
  int n=rle_encode(raw,2048,enc);
  FILE*f=fopen("rle_raw.bin","wb"); fwrite(raw,1,2048,f); fclose(f);
  f=fopen("rle_enc.bin","wb"); fwrite(enc,1,n,f); fclose(f);
  printf("raw 2048 -> rle %d\n", n); return 0; }
