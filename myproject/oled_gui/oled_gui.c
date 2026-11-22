// Tekenlaag voor 4 OLED-schermen - zie oled_gui.h
#include "oled_gui.h"
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// pixel-primitieven (per scherm 256x64, 1 bit)
/////////////////////////////////////////////////////////////////////////////
static void px(u8 *d, int x, int y, int on)
{
  if( x < 0 || y < 0 || x >= OLEDGUI_W || y >= OLEDGUI_H ) return;
  u8 *p = &d[y*OLEDGUI_ROW_BYTES + (x>>3)];
  if( on ) *p |= (1 << (x&7)); else *p &= ~(1 << (x&7));
}
static void rect(u8 *d, int x, int y, int w, int h, int on)
{
  for(int j=0; j<h; ++j) for(int i=0; i<w; ++i) px(d, x+i, y+j, on);
}
static void frame(u8 *d, int x, int y, int w, int h, int on)
{
  for(int i=0; i<w; ++i){ px(d, x+i, y, on); px(d, x+i, y+h-1, on); }
  for(int j=0; j<h; ++j){ px(d, x, y+j, on); px(d, x+w-1, y+j, on); }
}

// teken een teken uit een glcd-font met 1 byte-rij (hoogte 8): kolommen, bit0 = boven
static int chr(u8 *d, const u8 *font, int x, int y, char c, int scale, int inv)
{
  int w = font[0], off = font[3];
  const u8 *g = &font[4] + font[2] + (size_t)off * (u8)c;
  for(int cx=0; cx<w; ++cx)
    for(int cy=0; cy<8; ++cy){
      int on = (g[cx] >> cy) & 1;
      if( inv ) on = !on;
      if( scale == 1 ) px(d, x+cx, y+cy, on); else rect(d, x+cx*scale, y+cy*scale, scale, scale, on);
    }
  return w*scale;
}
static int text_w(const u8 *font, const char *s, int scale){ int n=0; while(*s++) ++n; return n*font[0]*scale; }
static void text(u8 *d, const u8 *font, int x, int y, const char *s, int scale, int inv)
{
  while( *s ) x += chr(d, font, x, y, *s++, scale, inv);
}

/////////////////////////////////////////////////////////////////////////////
// noten
/////////////////////////////////////////////////////////////////////////////
void OLEDGUI_NoteName(u8 note, char *out)
{
  static const char n[12][3] = {"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
  // korte vorm: "C1 ", "D#1": octaaf = note/12 - 2
  int oct = (int)(note/12) - 2;
  out[0] = n[note%12][0];
  out[1] = n[note%12][1] == '#' ? '#' : (oct < 0 ? '-' : ' ');
  out[2] = oct < 0 ? '0' + (-oct) : '0' + oct;
  if( n[note%12][1] != '#' && oct < 0 ){ out[1] = '-'; }
  out[3] = 0;
  if( n[note%12][1] != '#' && oct >= 0 ){ out[1] = out[2]; out[2] = 0; }
}

/////////////////////////////////////////////////////////////////////////////
// ring: 13 punten over 270 graden (-225..+45 gr), eerste n verlicht (2x2), rest 1 pixel
/////////////////////////////////////////////////////////////////////////////
// cos/sin * 100 voor -225 + k*22.5 graden
static const s8 RC[13] = {-71,-92,-100,-92,-71,-38,0,38,71,92,100,92,71};
static const s8 RS[13] = {71,38,0,-38,-71,-92,-100,-92,-71,-38,0,38,71};
// let op: hoek -225gr = linksonder; omhoog = negatieve y; kloksgewijs

static void ring(u8 *d, int cx, int cy, int r, int lit, int bright)
{
  for(int k=0; k<13; ++k){
    int x = cx + (RC[k]*r)/100, y = cy + (RS[k]*r)/100;
    if( k < lit ) rect(d, x-1, y-1, 3, 3, 1);
    else if( bright ) px(d, x, y, 1);
    else if( (k&1) == 0 ) px(d, x, y, 1);
  }
}

/////////////////////////////////////////////////////////////////////////////
// een stap (64 pixels breed) op positie x0 van een scherm
/////////////////////////////////////////////////////////////////////////////
static void cell(u8 *d, int x0, int num, const oledgui_step_t *s, int playing)
{
  char b[8];
  if( x0 > 0 ) for(int y=0; y<OLEDGUI_H; y+=4) px(d, x0, y, 1);       // scheidingslijn

  // kop (17 px): stapnummer links (klein), noot rechts groot (2x); gevuld als de stap getriggerd is
  if( s->on ) rect(d, x0+1, 0, 62, 17, 1);
  b[0] = '0' + (num+1)/10; b[1] = '0' + (num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, x0+3, 5, b, 1, s->on);
  if( s->note != 0xff ){ OLEDGUI_NoteName(s->note, b); } else { b[0]='-'; b[1]='-'; b[2]=0; }
  text(d, GLCD_FONT_NORMAL, x0+61-text_w(GLCD_FONT_NORMAL, b, 2), 1, b, 2, s->on);

  // ring + waarde
  int lit = s->on ? (s->value*13 + 64) / 127 : 0;
  ring(d, x0+32, 41, 14, lit, s->on);
  if( s->on ){
    b[0] = s->value >= 100 ? '0'+s->value/100 : ' ';
    b[1] = s->value >= 10 ? '0'+(s->value/10)%10 : ' ';
    b[2] = '0' + s->value%10; b[3] = 0;
    int tx = x0+32 - 9, ty = 37;
    text(d, GLCD_FONT_NORMAL, tx, ty, b, 1, 0);
    text(d, GLCD_FONT_NORMAL, tx+1, ty, b, 1, 0);
  } else {
    text(d, GLCD_FONT_NORMAL, x0+32-6, 37, "--", 1, 0);
  }

  // labels onderin: ACC gevuld, SLD tekst
  if( s->acc ){ rect(d, x0+2, 55, 19, 9, 1); text(d, GLCD_FONT_SMALL, x0+4, 56, "ACC", 1, 1); }
  if( s->sld ){ text(d, GLCD_FONT_SMALL, x0+38, 56, "SLD", 1, 0); }

  // looplicht: kader om de kolom
  if( playing ){ frame(d, x0+1, 0, 62, 64, 1); frame(d, x0+2, 1, 60, 62, 1); }
}

void OLEDGUI_RenderSteps(oledgui_frame_t *f, const oledgui_step_t steps[16], u8 play_step)
{
  for(int d=0; d<OLEDGUI_NUM_DISPLAYS; ++d){
    u8 *dp = f->fb[d];
    for(int i=0; i<OLEDGUI_DISPLAY_BYTES; ++i) dp[i] = 0;
    for(int k=0; k<4; ++k){
      int n = d*4 + k;
      cell(dp, k*64, n, &steps[n], play_step == n);
    }
  }
}
