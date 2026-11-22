#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oled_gui2.h"
static void dump(const char *name, void (*fn)(u8*,int,const og2_tile_t*), og2_tile_t *tl)
{
  static og2_frame_t f; static u8 c[2048];
  for(int n=0;n<16;++n){ fn(c, n, &tl[n]);
    for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[n/4][y][(n%4)*64+x] = (x&1)?(c[y*32+x/2]&15):(c[y*32+x/2]>>4); }
  FILE *o = fopen(name,"wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
  for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
  fclose(o);
}
int main(int argc, char **argv)
{
  if( argc>1 && !strcmp(argv[1],"velstyles") ){
    og2_step_t s[4] = {0}; static const u8 notes[4]={36,36,39,41}, vel[4]={110,60,127,90}, len[4]={80,40,100,20};
    for(int i=0;i<4;++i){ s[i].note=notes[i]; s[i].vel=vel[i]; s[i].len=len[i]; s[i].prob=100; s[i].on=1; }
    static og2_frame_t f; static u8 cb[2048];
    static const int styles[4] = {1,2,3,4};
    for(int k=0;k<4;++k){ og2_vel_style = styles[k];
      for(int n=0;n<4;++n){ OG2_RenderCellBoxed(cb, n, &s[n], 36, 41, 0, 0, "NOTE", OG2_K_NOTE);
        for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[k][y][n*64+x] = (x&1)?(cb[y*32+x/2]&15):(cb[y*32+x/2]>>4); } }
    FILE *o = fopen("velstyles.pgm","wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
    for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
    fclose(o); return 0;
  }
  if( argc>1 && !strncmp(argv[1],"boxed",5) ){
    og2_step_t s[16] = {0};
    static const u8 notes[16] = {36,36,39,41, 0xff,34,36,48, 36,38,43,38, 36,0xff,34,46};
    static const u8 vel[16]   = {110,60,127,90, 0,100,70,127, 90,50,120,80, 110,0,100,127};
    static const u8 len[16]   = {80,40,100,20, 0,60,30,100, 50,90,100,25, 70,0,45,100};
    static const u8 prob[16]  = {100,100,100,50, 100,100,30,100, 100,100,100,100, 100,100,60,100};
    for(int i=0;i<16;++i){ s[i].note=notes[i]; s[i].vel=vel[i]; s[i].len=len[i]; s[i].prob=prob[i]; s[i].on=notes[i]!=0xff; s[i].value=vel[i]; s[i].lval=len[i]*96/100; sprintf(s[i].txt,"%d%%",len[i]); }
    s[0].acc=1; s[2].acc=1; s[2].sld=1; s[1].sld=1; s[7].roll=1; s[10].acc=1; s[12].acc=1; s[14].skip=1; s[3].roll=1; s[2].len=100;
    og2_kind_t kind = (argc>1 && !strcmp(argv[1],"boxedlen")) ? OG2_K_LEN : OG2_K_NOTE;
    if( argc>1 && !strcmp(argv[1],"boxedmute") ) for(int i=0;i<16;++i) s[i].muted = 1;
    static og2_frame_t f; static u8 cb[2048]; int lo, hi; OG2_StepRange(s,&lo,&hi);
    for(int n=0;n<16;++n){ OG2_RenderCellBoxed(cb, n, &s[n], lo, hi, n==5, n==4, "LEN", kind);
      for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[n/4][y][(n%4)*64+x] = (x&1)?(cb[y*32+x/2]&15):(cb[y*32+x/2]>>4); }
    FILE *o = fopen(kind==OG2_K_LEN?"boxedlen.pgm":(!strcmp(argv[1],"boxedmute")?"boxedmute.pgm":"boxed.pgm"),"wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
    for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
    fclose(o); return 0;
  }
  if( argc>1 && !strncmp(argv[1],"text",4) ){
    static const char *R0[3] = {"Trk. Type Steps/ParL/TrgL Port Chn. Trk.Layer  controls                         ",
      " Forward  >PingPong< Rand.Dir  Rand.D+S Fwd. Back Replay  Repeat Skip Itv. SyncM                    ",
      "Trk. Clock Divider  SyncToMeasure Manual       Quick Selection: Timebase                          "};
    static const char *R1[3] = {"G1T1 Note   128   8    8  USB2   1  Inst  A    Note                PRESETS  INIT",
      "G1T1  Backward  Pendulum  Rand.Step       1     0    x1       0    0    4   no                       ",
      "G1T1  16 (normal)         no        no     1    2    4    8    8T   16   16T  32                    "};
    int pg = argv[1][4] ? argv[1][4]-'0' : 0;
    og2_tgroup_t tg[16]; OG2_TextGroups((const u8*)R0[pg], (const u8*)R1[pg], tg);
    static og2_frame_t f; static u8 cb[2048];
    for(int n=0;n<16;++n){ OG2_RenderTextGroupCell(cb, n, &tg[n]);
      for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[n/4][y][(n%4)*64+x] = (x&1)?(cb[y*32+x/2]&15):(cb[y*32+x/2]>>4); }
    FILE *o = fopen("text.pgm","wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
    for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
    fclose(o); return 0;
  }
  if( argc>1 && !strncmp(argv[1],"timing",6) ){
    og2_timing_t t; memset(&t,0,sizeof(t)); strcpy(t.track,"G1T2"); t.length=16; t.loop=1; t.num_steps=64;
    static const u16 pr[8]={4,8,16,24,32,64,128,256}; for(int i=0;i<8;++i) t.presets[i]=pr[i]; t.len_preset=2;
    t.divider=16; t.tb_preset=5; t.sync=1; t.dir=2; t.fwd=1; t.back=0; t.replay=1; t.repeat=0; t.skip=2; t.itv=4;
    t.sel = argv[1][6] ? atoi(argv[1]+6) : 0;
    if( t.sel==7 ){ t.manual=1; }
    if( t.sel==5 ){ t.divider=12; t.tb_preset=0xff; t.triplet=1; }
    static og2_frame_t f; static u8 c[2048];
    for(int n=0;n<16;++n){ OG2_RenderTimingCell(c, n, &t);
      for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[n/4][y][(n%4)*64+x] = (x&1)?(c[y*32+x/2]&15):(c[y*32+x/2]>>4); }
    FILE *o = fopen("timing.pgm","wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
    for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
    fclose(o); return 0;
  }
  if( argc>1 && !strcmp(argv[1],"dash") ){
    og2_dash_t dh; memset(&dh,0,sizeof(dh)); dh.bpm10=1320; dh.running=1; dh.master=1; dh.bar=12; dh.beat=2; dh.sixt=9; strcpy(dh.session,"GEN1");
    for(int g=0;g<4;++g){ dh.pat_group[g]=g; dh.pat_num[g]=g==2?3:0; dh.pat_req[g]=(g==3); strcpy(dh.pat_name[g], g==0?"GEN1 lanes":(g==1?"GEN1 pad":"Init")); }
    dh.sel_track=1; dh.muted_cnt=2;
    static og2_frame_t f; static u8 c[2048];
    for(int n=0;n<16;++n){ OG2_RenderDashCell(c, n, &dh);
      for(int y=0;y<OG2_H;++y) for(int x=0;x<64;++x) f.gray[n/4][y][(n%4)*64+x] = (x&1)?(c[y*32+x/2]&15):(c[y*32+x/2]>>4); }
    FILE *o = fopen("dash.pgm","wb"); fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
    for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
    fclose(o); return 0;
  }
  if( argc>1 && !strcmp(argv[1],"tiles") ){
    og2_tile_t tl[16]; memset(tl,0,sizeof(tl));
    for(int i=0;i<16;++i){ sprintf(tl[i].label,"G%dT%d",i/4+1,i%4+1); tl[i].len = (i%4==0)?16:(i%4==1?15:(i%4==2?32:13)); tl[i].pos = (i%5==0)?0xff:(i*3)%tl[i].len; tl[i].win = tl[i].pos==0xff?0:tl[i].pos/16; tl[i].gates = (i*2731+977)&0xffff; tl[i].v1 = 40+i*5; tl[i].v2 = 20+i*6; tl[i].v3 = i*7; sprintf(tl[i].txt,"%d",40+i*5); strcpy(tl[i].pname,"VOLUME"); tl[i].vmax=127; }
    tl[0].flags=1; tl[3].flags=2; tl[6].flags=4; tl[7].flags=2;
    for(int i=0;i<16;++i){ sprintf(tl[i].txt,"%d",40+i*5); strcpy(tl[i].pname,"VOL"); tl[i].vmax=127; strcpy(tl[i].port, i%3?"USB1":"Def."); tl[i].chn=i; tl[i].v3=(i%2)?i*7:0; }
    tl[0].flags=1; tl[1].flags=0; sprintf(tl[3].txt,"L24"); strcpy(tl[3].pname,"PAN"); tl[3].pan=1; tl[3].v1=40; tl[5].txt[0]=0; strcpy(tl[5].txt,"---"); tl[5].vmax=0;
    dump("tiles_tracks.pgm", OG2_RenderTrackTile, tl); dump("tiles_mixer.pgm", OG2_RenderMixerTile, tl);
    for(int i=0;i<16;++i) sprintf(tl[i].label,"CH%02d",i+1);
    dump("tiles_mixer.pgm", OG2_RenderMixerTile, tl);
    return 0;
  }
  static og2_frame_t f; og2_step_t s[16] = {0};
  static const u8 notes[16] = {36,36,39,41, 0xff,34,36,48, 36,38,43,38, 36,0xff,34,46};
  static const u8 vel[16]   = {110,60,127,90, 0,100,70,127, 90,50,120,80, 110,0,100,127};
  static const u8 len[16]   = {80,40,100,20, 0,60,30,100, 50,90,100,25, 70,0,45,100};
  static const u8 prob[16]  = {100,100,100,50, 100,100,30,100, 100,100,100,100, 100,100,60,100};
  static const s8 dly[16]   = {0,0,0,40, 0,0,20,0, 0,0,0,0, 0,0,30,0};
  for(int i=0;i<16;++i){ s[i].note=notes[i]; s[i].vel=vel[i]; s[i].len=len[i]; s[i].prob=prob[i]; s[i].delay=dly[i]; s[i].on=notes[i]!=0xff; s[i].value=vel[i]; }
  s[0].acc=1; s[2].acc=1; s[2].sld=1; s[1].sld=1; s[7].roll=1; s[10].acc=1; s[12].acc=1; s[14].skip=1; s[3].roll=1;
  s[2].glide_to=notes[7]; s[7].sld=1; s[7].len=100; s[7].glide_to=36; s[1].glide_to=39; s[2].len=100;
  f.layer_name = "VEL"; f.kind = OG2_K_NOTE;
  if( argc>3 ){
    const char *k = argv[3];
    for(int i=0;i<16;++i){
      s[i].lval = s[i].len;                                         // laagwaarde per stap
      if( !strcmp(k,"len") ){ f.kind=OG2_K_LEN; f.layer_name="LEN"; s[i].lval = s[i].on ? (s[i].len>=100?96:s[i].len*96/100) : 72; if(s[i].lval>=96) strcpy(s[i].txt,"Gld."); else sprintf(s[i].txt,"%d%%", s[i].lval*100/96); }
      if( !strcmp(k,"vel") ){ f.kind=OG2_K_VEL; f.layer_name="VEL"; s[i].lval=s[i].vel; sprintf(s[i].txt,"%d",s[i].vel); }
      if( !strcmp(k,"prob") ){ f.kind=OG2_K_PROB; f.layer_name="PRB"; s[i].lval=s[i].prob; sprintf(s[i].txt,"%d%%",s[i].prob); }
      if( !strcmp(k,"dly") ){ f.kind=OG2_K_DELAY; f.layer_name="DLY"; s[i].lval=s[i].delay*96/100; sprintf(s[i].txt,"%d",s[i].lval); }
      if( !strcmp(k,"roll") ){ f.kind=OG2_K_ROLL; f.layer_name="RLL"; s[i].lval = (i%3==0)?0:(((i%4)<<4)|((i&1)?0x40:0)|3); if(s[i].lval) sprintf(s[i].txt,"%d%c%02d",((s[i].lval&0x30)>>4)+2,(s[i].lval&0x40)?'U':'D',s[i].lval&0xf); else strcpy(s[i].txt,"----"); }
    }
  }
  OG2_RenderSteps(&f, s, argc>1?atoi(argv[1]):5, argc>2?atoi(argv[2]):4);
  FILE *o = fopen("frame2.pgm","wb");
  fprintf(o,"P5\n%d %d\n15\n", OG2_W*OG2_DISPLAYS, OG2_H);
  for(int y=0;y<OG2_H;++y) for(int d=0;d<OG2_DISPLAYS;++d) for(int x=0;x<OG2_W;++x) fputc(f.gray[d][y][x], o);
  fclose(o); return 0;
}
