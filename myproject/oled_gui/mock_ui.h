// Gedeelde tekenhulp voor de Mac-voorbeelden van de nieuwe pagina's (raster: blok y 11..61, marge 3, label links-boven, waarde gecentreerd, balk onderin)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mios32.h>
#include <glcd_font.h>
#define W 1024
#define H 96
static unsigned char F[H][W];
static void px(int x,int y,int g){ if(x<0||y<0||x>=W||y>=H) return; if(g>15) g=15; F[y][x]=g; }
static void rect(int x,int y,int w,int h,int g){ for(int j=0;j<h;++j) for(int i=0;i<w;++i) px(x+i,y+j,g); }
static void frame(int x,int y,int w,int h,int g){ for(int i=0;i<w;++i){px(x+i,y,g);px(x+i,y+h-1,g);} for(int j=0;j<h;++j){px(x,y+j,g);px(x+w-1,y+j,g);} }
static int tw(const unsigned char*f,const char*s,int sc){ int n=0; while(*s++) ++n; return n*f[0]*sc; }
static void chr(const unsigned char*f,int x,int y,char c,int sc,int g){ int w=f[0],off=f[3]; const unsigned char*gl=&f[4]+f[2]+(size_t)off*(unsigned char)c; for(int cx=0;cx<w;++cx) for(int cy=0;cy<8;++cy) if((gl[cx]>>cy)&1) rect(x+cx*sc,y+cy*sc,sc,sc,g); }
static void text(const unsigned char*f,int x,int y,const char*s,int sc,int g){ while(*s){ chr(f,x,y,*s,sc,g); x+=f[0]*sc; ++s; } }
static void btext(int x,int y,const char*s,int sc,int g){ text(GLCD_FONT_NORMAL,x,y,s,sc,g); text(GLCD_FONT_NORMAL,x+1,y,s,sc,g); }
// gecentreerd rond cx (bold normaal font)
static void cbtext(int cx,int y,const char*s,int sc,int g){ int w=tw(GLCD_FONT_NORMAL,s,sc); btext(cx-w/2,y,s,sc,g); }
static void csmall(int cx,int y,const char*s,int g){ int w=tw(GLCD_FONT_SMALL,s,1); text(GLCD_FONT_SMALL,cx-w/2,y,s,1,g); }
// vak: kolom k, n kolommen breed; x-marge 3, y 11..60 (hoogte 50)
#define BY 11
#define BH 50
static int bx(int k){ return k*64+3; }
static int bw(int n){ return n*64-6; }
static void box(int k,int n,int sel){ int x=bx(k), w=bw(n); if(sel) rect(x,BY,w,BH,2); frame(x,BY,w,BH,sel?15:9); if(sel) frame(x+1,BY+1,w-2,BH-2,15); }
static void label(int k,const char*l,int sel){ text(GLCD_FONT_SMALL,bx(k)+5,BY+4,l,1,sel?12:9); }
// waarde: horizontaal en verticaal gecentreerd tussen label (y 26) en balk (y 52)
static void value(int k,int n,const char*v,int g){ int sc=(tw(GLCD_FONT_NORMAL,v,2)>bw(n)-8)?1:2; cbtext(bx(k)+bw(n)/2, sc==2?29:33, v, sc, g); }
static void bar(int k,int frac100){ int x=bx(k)+5, w=bw(1)-10; for(int i=0;i<w;++i) px(x+i,52,3); rect(x,51,frac100*w/100,3,13); }
static void vbox(int k,const char*l,const char*v,int frac,int sel){ box(k,1,sel); label(k,l,sel); value(k,1,v,15); if(frac>=0) bar(k,frac); }
static void tbox(int k,const char*l,int on){ box(k,1,0); if(on) rect(bx(k)+1,BY+1,bw(1)-2,BH-2,6); label(k,l,on); cbtext(bx(k)+bw(1)/2,31,on?"ON":"OFF",1,on?15:6); int x=bx(k)+5,w=bw(1)-10; rect(x,51,w,3,on?15:3); }
// kiezer over n kolommen: cols x rows raster binnen het kader (y 26..57)
static void picker(int k,int n,const char*l,const char**it,int cnt,int cols,int sel){
  box(k,n,0); label(k,l,0);
  int rows=(cnt+cols-1)/cols, ix=bx(k)+5, iw=bw(n)-10, iy=BY+17, ih=BH-22;
  int gap=2, cw=(iw-(cols-1)*gap)/cols, ch=(ih-(rows-1)*gap)/rows;
  for(int i=0;i<cnt;++i){ int cx=ix+(i%cols)*(cw+gap), cy=iy+(i/cols)*(ch+gap); int s=(i==sel);
    if(s) rect(cx,cy,cw,ch,13); else frame(cx,cy,cw,ch,5);
    csmall(cx+cw/2,cy+ch/2-3,it[i],s?0:10); } }
// knopblok over n kolommen met gecentreerde tekst
static void button(int k,int n,const char*l,const char*t,int hot){ box(k,n,hot); label(k,l,hot); int cx=bx(k)+bw(n)/2; int w=tw(GLCD_FONT_NORMAL,t,1)+14; frame(cx-w/2,32,w,15,hot?15:9); cbtext(cx,36,t,1,15); }
static void header(int scr,const char*t){ text(GLCD_FONT_SMALL,scr*256+5,1,t,1,9); for(int x=scr*256+5+tw(GLCD_FONT_SMALL,t,1)+6;x<scr*256+251;x+=2) px(x,4,3); }
// onder elke kolom: encoder (cirkel, 14 px) en knop (28x9) precies midden onder de kolom (zoals op het paneel)
static void circle(int cx,int cy,int r,int g,int fill){ for(int y=-r;y<=r;++y) for(int x=-r;x<=r;++x){ int d=x*x+y*y; if(fill? d<=r*r : (d<=r*r && d>=(r-1)*(r-1))) px(cx+x,cy+y,g); } }
static void controls(void){
  for(int k=0;k<16;++k){ int cx=bx(k)+bw(1)/2;
    circle(cx,73,7,6,1); circle(cx,73,7,10,0); rect(cx-1,67,2,5,15);          // encoder met streepje
    rect(cx-14,83,28,9,5); frame(cx-14,83,28,9,10);                          // knop
  } }
static void save_pgm(const char*name){ controls(); FILE*o=fopen(name,"wb"); fprintf(o,"P5\n%d %d\n15\n",W,H); for(int y=0;y<H;++y) fwrite(F[y],1,W,o); fclose(o); }
