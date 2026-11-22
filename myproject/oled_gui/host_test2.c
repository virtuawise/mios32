#include <stdio.h>
#include <stdlib.h>
#include "oled_gui2.h"
int main(int argc, char **argv)
{
  static og2_frame_t f; og2_step_t s[16] = {0};
  static const u8 notes[16] = {36,36,39,41, 0xff,34,36,48, 36,38,43,38, 36,0xff,34,46};
  static const u8 vel[16]   = {110,60,127,90, 0,100,70,127, 90,50,120,80, 110,0,100,127};
  static const u8 len[16]   = {80,40,100,20, 0,60,30,100, 50,90,100,25, 70,0,45,100};
  static const u8 prob[16]  = {100,100,100,50, 100,100,30,100, 100,100,100,100, 100,100,60,100};
  static const s8 dly[16]   = {0,0,0,40, 0,0,20,0, 0,0,0,0, 0,0,30,0};
  for(int i=0;i<16;++i){ s[i].note=notes[i]; s[i].vel=vel[i]; s[i].len=len[i]; s[i].prob=prob[i]; s[i].delay=dly[i]; s[i].on=notes[i]!=0xff; s[i].value=vel[i]; }
  s[0].acc=1; s[2].acc=1; s[2].sld=1; s[1].sld=1; s[7].roll=1; s[10].acc=1; s[12].acc=1; s[14].skip=1; s[3].roll=1;
  s[2].glide_to=notes[7]; s[7].sld=1; s[7].len=100; s[7].glide_to=36; s[1].glide_to=39; s[2].len=100;
  f.layer_name = "VEL";
  OG2_RenderSteps(&f, s, argc>1?atoi(argv[1]):5, argc>2?atoi(argv[2]):4);
  FILE *o = fopen("frame2.pgm","wb");
  fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
  for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
  fclose(o); return 0;
}
