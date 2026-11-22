#include "oled_gui2.h"
#include <glcd_font.h>

#define CW 64
static void px(u8 (*d)[CW], int x, int y, int g)
{
  if( x < 0 || y < 0 || x >= CW || y >= OG2_H ) return;
  if( g > 15 ) g = 15;
  if( g < 0 ) g = 0;
  d[y][x] = g;
}
static void rect(u8 (*d)[CW], int x, int y, int w, int h, int g)
{
  for(int j=0; j<h; ++j) for(int i=0; i<w; ++i) px(d, x+i, y+j, g);
}
static void hline(u8 (*d)[CW], int x, int y, int w, int g, int dotted)
{
  for(int i=0; i<w; ++i) if( !dotted || (i % dotted) == 0 ) px(d, x+i, y, g);
}
// glcd-font teken, alleen ingeschakelde pixels (fg) in grijs g; scale 1 of 2
static int chr(u8 (*d)[CW], const u8 *font, int x, int y, char c, int scale, int g)
{
  int w = font[0], off = font[3];
  const u8 *gl = &font[4] + font[2] + (size_t)off * (u8)c;
  for(int cx=0; cx<w; ++cx) for(int cy=0; cy<8; ++cy)
    if( (gl[cx] >> cy) & 1 ) rect(d, x+cx*scale, y+cy*scale, scale, scale, g);
  return w*scale;
}
static int tw(const u8 *font, const char *s, int scale){ int n=0; while(*s++) ++n; return n*font[0]*scale; }
static void text(u8 (*d)[CW], const u8 *font, int x, int y, const char *s, int scale, int g)
{ while( *s ) x += chr(d, font, x, y, *s++, scale, g); }

static void note_name(u8 note, char *out)
{
  static const char *n[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  int oct = (int)(note/12) - 2, k = 0;
  for(const char *p = n[note%12]; *p; ++p) out[k++] = *p;
  if( oct < 0 ){ out[k++] = '-'; oct = -oct; }
  out[k++] = '0' + oct; out[k] = 0;
}
static void num3(char *b, int v){ b[0] = v>=100 ? '0'+v/100 : ' '; b[1] = v>=10 ? '0'+(v/10)%10 : ' '; b[2] = '0'+v%10; b[3] = 0; }

#define ROLL_TOP 14
#define ROLL_BOT 47     // y van de laagste noot

static void cell(u8 (*d)[CW], int divider, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer)
{
  char b[8];
  const int x0 = 0;
  int bg = playing ? 3 : (trailing ? 1 : 0);
  if( bg ) rect(d, x0+1, 0, 62, 64, bg);
  if( divider ) for(int y=0; y<64; y+=4) px(d, x0, y, 2);         // scheidingslijn

  // kop: stapnummer + noot
  b[0] = '0'+(num+1)/10; b[1] = '0'+(num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, x0+3, 3, b, 1, s->on ? 9 : 4);
  if( s->on && s->note != 0xff ){
    note_name(s->note, b);
    int w = tw(GLCD_FONT_NORMAL, b, 1);
    text(d, GLCD_FONT_NORMAL, x0+61-w, 3, b, 1, 15);
    text(d, GLCD_FONT_NORMAL, x0+62-w, 3, b, 1, 15);          // vet
  } else {
    text(d, GLCD_FONT_NORMAL, x0+61-12, 3, "--", 1, 3);
  }

  // pianoroll-vlak: octaaflijnen (C) gestippeld
  hline(d, x0+2, ROLL_TOP-1, 60, 2, 3);
  int span = hi - lo; if( span < 11 ) span = 11;
  for(int n=lo; n<=lo+span; ++n)
    if( (n % 12) == 0 ){ int y = ROLL_BOT - ((n-lo)*(ROLL_BOT-ROLL_TOP-3))/span; hline(d, x0+2, y+1, 60, 2, 4); }

  if( s->on && s->note != 0xff ){
    int y = ROLL_BOT - ((s->note-lo)*(ROLL_BOT-ROLL_TOP-3))/span;      // bovenkant van de balk
    int gx = x0 + 4 + (s->delay > 0 ? (s->delay*50)/100 : 0);
    int len = 2 + (s->len > 100 ? 100 : s->len)*(56 - (gx-x0-4))/100; if( len < 3 ) len = 3;
    int g = 5 + (s->vel*10)/127;                                       // velocity = helderheid
    for(int i=0; i<len; ++i){
      int on = 1;
      if( s->prob < 100 ){                                              // probability = korrel
        if( s->prob < 35 ) on = ((i + (num&1)) % 4) == 0;
        else if( s->prob < 70 ) on = ((i + (num&1)) % 2) == 0;
        else on = (i % 5) != 4;
      }
      if( on ){ px(d, gx+i, y, g); px(d, gx+i, y+1, g); px(d, gx+i, y+2, g); px(d, gx+i, y+3, g-2); }
    }
    rect(d, gx, y-1, 3, 6, 15);                                         // kop van de noot (gate-start)
    if( s->acc ){ hline(d, gx-1, y-3, 5, 15, 0); hline(d, gx-1, y-4, 5, 15, 0); }                      // accentstreepje
    if( s->sld ){ for(int i=0; i<6; ++i){ px(d, gx+len+i, y+2-i/2, 13); px(d, gx+len+i, y+3-i/2, 13); } }// glide-staartje
  } else {
    hline(d, x0+22, (ROLL_TOP+ROLL_BOT)/2, 20, 3, 2);                    // lege stap: stippellijn
  }

  // onderrij: iconen links (A/S/R/K), actieve laagwaarde rechts
  int ix = x0+3;
  struct { u8 on; char c; } ic[4] = {{s->acc,'A'},{s->sld,'S'},{s->roll,'R'},{s->skip,'K'}};
  for(int i=0; i<4; ++i) if( ic[i].on ){
    rect(d, ix, 52, 8, 9, 10);
    char t[2] = {ic[i].c, 0};
    for(int cx=0; cx<4; ++cx) for(int cy=0; cy<8; ++cy)
      if( (GLCD_FONT_SMALL[4+GLCD_FONT_SMALL[2]+GLCD_FONT_SMALL[3]*(u8)t[0]+cx] >> cy) & 1 ) px(d, ix+2+cx, 52+cy, 0);
    ix += 10;
  }
  if( s->on ){
    num3(b, s->value);
    int w = tw(GLCD_FONT_NORMAL, b, 1);
    text(d, GLCD_FONT_NORMAL, x0+61-w, 52, b, 1, 15);
    text(d, GLCD_FONT_NORMAL, x0+62-w, 52, b, 1, 15);
  } else {
    text(d, GLCD_FONT_SMALL, x0+61-12, 52, layer, 1, 3);
  }

  // afspeelpositie: heldere balk bovenaan en onderaan
  if( playing ){ rect(d, x0+1, 0, 62, 2, 15); rect(d, x0+1, 62, 62, 2, 15); }
}

void OG2_StepRange(const og2_step_t st[16], int *lo, int *hi)
{
  int l = 127, h = 0;
  for(int i=0; i<16; ++i) if( st[i].on && st[i].note != 0xff ){ if( st[i].note < l ) l = st[i].note; if( st[i].note > h ) h = st[i].note; }
  if( l > h ){ l = 36; h = 48; }
  *lo = l; *hi = h;
}

// rendert 1 stap-kolom (64x64) naar 2048 bytes: 4 bit per pixel, 32 bytes per rij, eerste pixel = hoge nibble
void OG2_RenderCell(u8 *out, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  cell(tmp, (num & 3) != 0, num, s, lo, hi, playing, trailing, layer ? layer : "VEL");
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; x+=2) out[y*32 + x/2] = (tmp[y][x] << 4) | tmp[y][x+1];
}

// host-test: 16 cellen naar 4 schermen van 256x64 (1 byte per pixel)
void OG2_RenderSteps(og2_frame_t *f, const og2_step_t st[16], u8 play_step, u8 trail_step)
{
  static u8 c[2048];
  int lo, hi;
  OG2_StepRange(st, &lo, &hi);
  for(int n=0; n<16; ++n){
    int d = n/4, k = n%4;
    OG2_RenderCell(c, n, &st[n], lo, hi, play_step == n, trail_step == n, f->layer_name);
    for(int y=0; y<OG2_H; ++y) for(int x=0; x<64; ++x)
      f->gray[d][y][k*64+x] = (x & 1) ? (c[y*32+x/2] & 15) : (c[y*32+x/2] >> 4);
  }
}
