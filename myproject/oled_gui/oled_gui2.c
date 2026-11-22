#include "oled_gui2.h"
#include <glcd_font.h>

#define CW 64
static int ox_shift = 0;      // dashboard: tekent een heel scherm (256 breed) en toont daarvan 1 cel van 64 px
static void px(u8 (*d)[CW], int x, int y, int g)
{
  x -= ox_shift;
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

#define ROLL_TOP 21
#define ROLL_BOT 48     // y van de laagste noot

static void cell_note(u8 (*d)[CW], int divider, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer);
static void cell_layer(u8 (*d)[CW], int divider, int num, const og2_step_t *s, int lo, int playing, int trailing, const char *layer, og2_kind_t kind);

static void cell_note(u8 (*d)[CW], int divider, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer)
{
  char b[8];
  const int x0 = 0;
  int bg = playing ? 3 : (trailing ? 1 : 0);
  if( bg ) rect(d, x0+1, 0, 62, 64, bg);
  if( divider ) for(int y=0; y<64; y+=4) px(d, x0, y, 2);         // scheidingslijn

  // kop: stapnummer klein links, noot GROOT (2x, vet) rechts
  b[0] = '0'+(num+1)/10; b[1] = '0'+(num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, x0+3, 3, b, 1, s->on ? 9 : 4);
  if( s->on && s->note != 0xff ){
    note_name(s->note, b);
    int w = tw(GLCD_FONT_NORMAL, b, 2);
    text(d, GLCD_FONT_NORMAL, x0+61-w, 3, b, 2, 15);
    text(d, GLCD_FONT_NORMAL, x0+62-w, 3, b, 2, 15);          // vet
  } else {
    text(d, GLCD_FONT_NORMAL, x0+61-24, 3, "--", 2, 3);
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
    int ramp = (s->sld ? 1 : 0);
    int ramp_x = 46;                       // hier begint de helling (glide)
    if( ramp && gx + len > ramp_x ) len = ramp_x - gx;
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
    if( ramp ){                                                        // glide: schuine lijn naar de volgende noot (loopt desnoods door lege stappen heen)
      int y2 = (s->gl_kind == 1) ? ROLL_BOT - (((int)s->gl_b - lo*16) * (ROLL_BOT-ROLL_TOP-3)) / (span*16)
             : (s->glide_to != 0xff) ? ROLL_BOT - ((s->glide_to-lo)*(ROLL_BOT-ROLL_TOP-3))/span : y - 4;
      int x1 = gx + len, x2 = 61;
      for(int x=x1; x<=x2; ++x){
        int yy = y + ((y2 - y) * (x - x1)) / (x2 - x1 > 0 ? x2 - x1 : 1);
        for(int t=0; t<4; ++t) px(d, x, yy+t, 14);
      }
    }
  } else if( s->gl_kind == 2 ){                                          // glide loopt door deze (lege) stap
    int ya = ROLL_BOT - (((int)s->gl_a - lo*16) * (ROLL_BOT-ROLL_TOP-3)) / (span*16);
    int yb = ROLL_BOT - (((int)s->gl_b - lo*16) * (ROLL_BOT-ROLL_TOP-3)) / (span*16);
    for(int x=1; x<=62; ++x){
      int yy = ya + ((yb - ya) * (x - 1)) / 61;
      for(int t=0; t<4; ++t) px(d, x, yy+t, 14);
    }
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


// ---- weergave voor een gekozen parameterlaag (lengte, probability, delay, roll, velocity, overig): de laagwaarde is groot,
//      de noot is bijzaak (klein, linksonder)
static void ring_gauge(u8 (*d)[CW], int cx, int cy, int r, int lit, int bright)
{
  static const s8 RC2[13] = {-71,-92,-100,-92,-71,-38,0,38,71,92,100,92,71};
  static const s8 RS2[13] = {71,38,0,-38,-71,-92,-100,-92,-71,-38,0,38,71};
  for(int k=0; k<13; ++k){
    int x = cx + (RC2[k]*r)/100, y = cy + (RS2[k]*r)/100;
    if( k < lit ) rect(d, x-1, y-1, 3, 3, bright ? 15 : 9); else px(d, x, y, bright ? 6 : 3);
  }
}

static void cell_layer(u8 (*d)[CW], int divider, int num, const og2_step_t *s, int lo, int playing, int trailing, const char *layer, og2_kind_t kind)
{
  char b[8];
  const int x0 = 0;
  int bg = playing ? 3 : (trailing ? 1 : 0);
  int on = s->on;
  int hot = on ? 15 : 7;                              // waarde ook tonen bij stappen zonder gate (gedimd)
  if( bg ) rect(d, x0+1, 0, 62, 64, bg);
  if( divider ) for(int y=0; y<64; y+=4) px(d, x0, y, 2);

  // kop: stapnummer klein, laagwaarde groot (2x, vet)
  b[0] = '0'+(num+1)/10; b[1] = '0'+(num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, x0+3, 3, b, 1, on ? 9 : 4);
  { int w = tw(GLCD_FONT_NORMAL, s->txt, 2);
    text(d, GLCD_FONT_NORMAL, x0+61-w, 3, s->txt, 2, hot);
    text(d, GLCD_FONT_NORMAL, x0+62-w, 3, s->txt, 2, hot); }

  // lichaam (y 21..48): visualisatie per laagsoort
  int cy = 35;
  switch( kind ){
  case OG2_K_LEN: {                                    // lengte als balk over de stap, Gld. = legato-pijl
    hline(d, x0+4, cy, 56, 3, 0);
    int glide = s->lval >= 96;
    int w = glide ? 42 : 2 + ((s->lval > 95 ? 95 : s->lval) * 54) / 95;
    rect(d, x0+4, cy-3, w, 7, on ? 12 : 6);
    rect(d, x0+4, cy-4, 2, 9, 15);
    if( glide ){ for(int i=0; i<9; ++i){ rect(d, x0+46+i, cy-4+i/2, 1, 9-i, 15); } }
  } break;
  case OG2_K_PROB: {                                   // probability als vulling van een vak
    int p = s->lval > 100 ? 100 : s->lval;
    for(int x=0; x<40; ++x) for(int y=0; y<18; ++y)
      if( x*100/40 < p ) px(d, x0+12+x, 26+y, ((x+y)&1) ? hot-3 : hot);
    for(int x=0; x<40; ++x){ px(d, x0+12+x, 25, 5); px(d, x0+12+x, 44, 5); }
  } break;
  case OG2_K_DELAY: {                                  // delay als verschuiving binnen de stap
    int dl = (s->lval > 95 ? 95 : s->lval);
    hline(d, x0+4, cy, 56, 3, 0);
    for(int i=0; i<5; ++i) px(d, x0+4+i*14, cy-3, 4), px(d, x0+4+i*14, cy+3, 4);
    rect(d, x0+4 + (dl*50)/95, cy-5, 4, 11, hot);
  } break;
  case OG2_K_ROLL: {                                   // roll: aantal slagen met oplopende/aflopende velocity
    int v = s->lval;
    if( v ){
      int n = ((v & 0x30) >> 4) + 2, up = (v & 0x40) != 0, vc = v & 0xf;
      for(int i=0; i<n; ++i){
        int g = up ? 6 + (i*8)/(n > 1 ? n-1 : 1) : 14 - (i*8)/(n > 1 ? n-1 : 1);
        if( vc == 0 ) g = 12;
        int bw = 8, gap = 2, tot = n*bw + (n-1)*gap, bx = x0 + 32 - tot/2 + i*(bw+gap);
        rect(d, bx, 26, bw, 18, g);
      }
    } else hline(d, x0+22, cy, 20, 3, 2);
  } break;
  case OG2_K_VEL:
  default: {                                           // ring met waarde
    int lit = (s->lval * 13 + 64) / 127;
    ring_gauge(d, x0+32, cy+1, 13, lit, on);
  } break;
  }

  // onderrij: noot klein, iconen, laagnaam
  int ix = x0+3;
  if( on && s->note != 0xff ){
    note_name(s->note, b);
    text(d, GLCD_FONT_NORMAL, ix, 53, b, 1, 8);
    ix += tw(GLCD_FONT_NORMAL, b, 1) + 3;
  }
  struct { u8 on; char c; } ic[4] = {{s->acc,'A'},{s->sld,'S'},{s->roll,'R'},{s->skip,'K'}};
  for(int i=0; i<4; ++i) if( ic[i].on && ix + 8 <= x0+46 ){
    rect(d, ix, 52, 8, 9, 10);
    char t[2] = {ic[i].c, 0};
    for(int cx=0; cx<4; ++cx) for(int cy2=0; cy2<8; ++cy2)
      if( (GLCD_FONT_SMALL[4+GLCD_FONT_SMALL[2]+GLCD_FONT_SMALL[3]*(u8)t[0]+cx] >> cy2) & 1 ) px(d, ix+2+cx, 52+cy2, 0);
    ix += 10;
  }
  text(d, GLCD_FONT_SMALL, x0+61-tw(GLCD_FONT_SMALL, layer, 1), 53, layer, 1, 6);

  if( playing ){ rect(d, x0+1, 0, 62, 2, 15); rect(d, x0+1, 62, 62, 2, 15); }
}


// ---- tegel: track (overzicht van alle tracks: gate-raster, afspeelpositie, mute/solo) ----
static void tile_pack(u8 *out, u8 (*tmp)[CW])
{
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; x+=2) out[y*32 + x/2] = (tmp[y][x] << 4) | tmp[y][x+1];
}

void OG2_RenderTrackTile(u8 *out, int num, const og2_tile_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int sel = t->flags & 1, mute = (t->flags >> 1) & 1, solo = (t->flags >> 2) & 1;
  int dim = mute ? 1 : 0;
  if( sel ) rect(d, 1, 0, 62, 64, 2);
  if( (num & 3) != 0 ) for(int y=0; y<64; y+=4) px(d, 0, y, 2);
  text(d, GLCD_FONT_NORMAL, 3, 3, t->label, 1, sel ? 15 : (dim ? 5 : 11));
  text(d, GLCD_FONT_NORMAL, 4, 3, t->label, 1, sel ? 15 : (dim ? 5 : 11));
  if( mute ){ rect(d, 46, 2, 15, 11, 12); text(d, GLCD_FONT_NORMAL, 49, 3, "M", 1, 0); text(d, GLCD_FONT_NORMAL, 50, 3, "M", 1, 0); }
  else if( solo ){ rect(d, 46, 2, 15, 11, 15); text(d, GLCD_FONT_NORMAL, 49, 3, "S", 1, 0); text(d, GLCD_FONT_NORMAL, 50, 3, "S", 1, 0); }
  else { char b[6]; int v = t->len; b[0] = v>=100 ? '0'+v/100 : ' '; b[1] = v>=10 ? '0'+(v/10)%10 : ' '; b[2] = '0'+v%10; b[3] = 0;
         text(d, GLCD_FONT_SMALL, 61 - tw(GLCD_FONT_SMALL, b, 1), 4, b, 1, 6); }

  // 16 stappen in 2 rijen van 8
  int cur = (t->pos != 0xff) ? (t->pos - t->win*16) : -1;
  for(int i=0; i<16; ++i){
    int bx = 4 + (i & 7) * 7, by = 18 + (i >> 3) * 14, step = t->win*16 + i;
    int active = step < t->len;
    int on = (t->gates >> i) & 1;
    if( !active ){ px(d, bx+2, by+5, 2); continue; }
    if( on ) rect(d, bx, by, 6, 11, dim ? 5 : (cur == i ? 15 : 12));
    else { for(int x=0; x<6; ++x){ px(d, bx+x, by, 3); px(d, bx+x, by+10, 3); } for(int y=0; y<11; ++y){ px(d, bx, by+y, 3); px(d, bx+5, by+y, 3); } }
    if( cur == i ){ rect(d, bx-1, by+12, 8, 2, 15); }
  }
  // voortgangsbalk over de hele tracklengte
  for(int x=0; x<56; ++x) px(d, 4+x, 52, 3);
  if( t->pos != 0xff ){ int w = (t->pos * 56) / (t->len ? t->len : 1); rect(d, 4, 51, w+1, 3, dim ? 5 : 13); }
  // vensternummer (welke 16 stappen)
  if( t->len > 16 ){ char b[4]; b[0]='p'; b[1]='0'+t->win+1; b[2]=0; text(d, GLCD_FONT_SMALL, 4, 56, b, 1, 6); }
  tile_pack(out, tmp);
}

// ---- tegel: mixerkanaal; toont de waarde van de gekozen mixerpagina (Port, Chn, Prg, Vol, Pan, ...) ----
void OG2_RenderMixerTile(u8 *out, int num, const og2_tile_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int sel = t->flags & 1;
  if( sel ) rect(d, 1, 0, 62, 64, 2);
  if( (num & 3) != 0 ) for(int y=0; y<64; y+=4) px(d, 0, y, 2);
  text(d, GLCD_FONT_SMALL, 3, 3, t->label, 1, sel ? 15 : 9);
  { int sc = (tw(GLCD_FONT_NORMAL, t->txt, 2) > 36) ? 1 : 2;       // lange teksten (poortnamen) kleiner
    int w = tw(GLCD_FONT_NORMAL, t->txt, sc), ty = sc == 2 ? 3 : 10;
    text(d, GLCD_FONT_NORMAL, 61-w, ty, t->txt, sc, 15); text(d, GLCD_FONT_NORMAL, 62-w, ty, t->txt, sc, 15); }
  if( t->vmax ){
    int w = (t->v1 * 56) / t->vmax; if( w > 56 ) w = 56;
    for(int x=0; x<56; ++x){ px(d, 4+x, 28, 3); px(d, 4+x, 38, 3); }
    if( t->pan ){
      rect(d, 4+28, 25, 1, 17, 5);
      rect(d, 4 + (t->v1 * 52) / t->vmax, 27, 4, 12, 15);
    } else {
      rect(d, 4, 29, w, 9, 12);
    }
  } else {
    hline(d, 20, 33, 24, 3, 2);
  }
  { int w = tw(GLCD_FONT_SMALL, t->pname, 1);
    text(d, GLCD_FONT_SMALL, 32 - w/2, 53, t->pname, 1, 8); }
  tile_pack(out, tmp);
}


// ---- tekstcel: 5 tekens per regel, 2x, voor alle pagina's die geen eigen weergave hebben ----
static void textcell_line(u8 (*d)[CW], int y, const char *s)
{
  for(int i=0; i<5; ++i){
    unsigned char c = (unsigned char)s[i];
    int x = 2 + i*12;
    if( c == ' ' || c == 0 ) continue;
    if( c < 32 ){ for(int yy=0; yy<12; ++yy) for(int xx=0; xx<8; ++xx) if( ((xx+yy)&1) == 0 ) px(d, x+xx+1, y+2+yy, 9); continue; }   // eigen tekens
    if( c == 0x7e ) c = '>';
    if( c == 0x7f ) c = '<';
    if( c > 0x7e ) c = '?';
    chr(d, GLCD_FONT_NORMAL, x, y, c, 2, 15);
    chr(d, GLCD_FONT_NORMAL, x+1, y, c, 2, 15);
  }
}

void OG2_RenderTextCell(u8 *out, int cell, const og2_text_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  if( (cell & 3) != 0 ) for(int y=0; y<64; y+=4) px(d, 0, y, 2);
  textcell_line(d, 6, t->l1);
  hline(d, 4, 31, 56, 2, 3);
  textcell_line(d, 38, t->l2);
  tile_pack(out, tmp);
}

// ---- dashboard ----
static void numtxt(char *o, int v, int width)       // rechts uitgelijnd getal
{
  char t[8]; int n = 0;
  if( v == 0 ) t[n++] = '0';
  while( v > 0 && n < 7 ){ t[n++] = '0' + v%10; v /= 10; }
  int pad = width - n; int k = 0;
  while( pad-- > 0 ) o[k++] = ' ';
  while( n > 0 ) o[k++] = t[--n];
  o[k] = 0;
}
static void bold(u8 (*d)[CW], int x, int y, const char *s, int sc, int g)
{ text(d, GLCD_FONT_NORMAL, x, y, s, sc, g); text(d, GLCD_FONT_NORMAL, x+1, y, s, sc, g); }

static void dash_screen(u8 (*d)[CW], int scr, const og2_dash_t *dh)
{
  char b[16];
  switch( scr ){
  case 0: {   // BPM + beat + status
    text(d, GLCD_FONT_SMALL, 8, 4, "BPM", 1, 8);
    numtxt(b, dh->bpm10 / 10, 3); b[3] = '.'; b[4] = '0' + dh->bpm10 % 10; b[5] = 0;
    bold(d, 8, 14, b, 3, 15);
    for(int i=0; i<4; ++i){
      int on = dh->running && dh->beat == i;
      rect(d, 140 + i*28, 8, 22, 22, on ? 15 : 2);
      if( !on ) for(int k=0; k<22; ++k){ px(d, 140+i*28+k, 8, 6); px(d, 140+i*28+k, 29, 6); px(d, 140+i*28, 8+k, 6); px(d, 140+i*28+21, 8+k, 6); }
    }
    bold(d, 140, 40, dh->running ? "PLAY" : "STOP", 2, dh->running ? 15 : 8);
    text(d, GLCD_FONT_NORMAL, 8, 50, dh->master ? "MASTER" : "SLAVE", 1, 7);
  } break;
  case 1: {   // maat/tel + 16 stappen
    text(d, GLCD_FONT_SMALL, 8, 4, "BAR", 1, 8);
    numtxt(b, dh->bar, 3); bold(d, 8, 14, b, 3, 15);
    text(d, GLCD_FONT_SMALL, 100, 4, "BEAT", 1, 8);
    b[0] = '1' + dh->beat; b[1] = 0; bold(d, 100, 14, b, 3, 15);
    for(int i=0; i<16; ++i){
      int x = 8 + i*15, on = dh->running && dh->sixt == i;
      rect(d, x, 46, 13, 10, on ? 15 : ((i & 3) == 0 ? 5 : 3));
    }
  } break;
  case 2: {   // patronen per groep
    for(int g=0; g<4; ++g){
      int x = g*64 + 5;
      b[0] = 'G'; b[1] = '1'+g; b[2] = 0;
      text(d, GLCD_FONT_SMALL, x, 4, b, 1, 8);
      b[0] = 'A' + dh->pat_group[g]; b[1] = '1' + dh->pat_num[g]; b[2] = 0;
      bold(d, x, 14, b, 3, dh->pat_req[g] ? 9 : 15);
      if( dh->pat_req[g] ) bold(d, x+40, 14, ">", 3, 15);
      char n8[10]; int k; for(k=0; k<9 && dh->pat_name[g][k]; ++k) n8[k] = dh->pat_name[g][k]; n8[k] = 0;
      text(d, GLCD_FONT_NORMAL, x, 46, n8, 1, 8);
    }
  } break;
  default: {  // sessie + geselecteerde track
    text(d, GLCD_FONT_SMALL, 8, 4, "SESSION", 1, 8);
    bold(d, 8, 14, dh->session, 2, 15);
    b[0] = 'G'; b[1] = '1' + dh->sel_track/4; b[2] = 'T'; b[3] = '1' + dh->sel_track%4; b[4] = 0;
    text(d, GLCD_FONT_SMALL, 8, 42, "TRACK", 1, 8);
    bold(d, 8, 50, b, 1, 15);
    text(d, GLCD_FONT_SMALL, 100, 42, "MUTED", 1, 8);
    numtxt(b, dh->muted_cnt, 2); bold(d, 100, 50, b, 1, 15);
    bold(d, 170, 40, dh->running ? "PLAY" : "STOP", 2, dh->running ? 15 : 8);
  } break;
  }
}

void OG2_RenderDashCell(u8 *out, int cell, const og2_dash_t *dh)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  int scr = cell / 4, col = cell % 4;
  ox_shift = col * 64;
  dash_screen(tmp, scr, dh);
  ox_shift = 0;
  if( col > 0 ) for(int y=0; y<64; y+=4) tmp[y][0] = 2;
  tile_pack(out, tmp);
}

// ---- voorstel: vierkante hokjes met 3 symboolvakjes ----
static void frame_box(u8 (*d)[CW], int x, int y, int w, int h, int g)
{
  for(int i=0; i<w; ++i){ px(d, x+i, y, g); px(d, x+i, y+h-1, g); }
  for(int j=0; j<h; ++j){ px(d, x, y+j, g); px(d, x+w-1, y+j, g); }
}
// symbool in een vakje van 18x13 (x,y = linkerbovenhoek)
static void slot_icon(u8 (*d)[CW], int x, int y, int kind, int on)
{
  int fg = on ? 0 : 3;                      // actief: donker symbool op felle vulling
  if( on ) rect(d, x, y, 18, 13, 13); else frame_box(d, x, y, 18, 13, 3);
  switch( kind ){
  case 0:   // accent: uitroepteken
    rect(d, x+8, y+2, 2, 6, fg ? fg : 0); rect(d, x+8, y+9, 2, 2, fg ? fg : 0); break;
  case 1:   // glide: schuine pijl omhoog
    for(int i=0; i<10; ++i){ px(d, x+4+i, y+9-i*6/10, fg); px(d, x+4+i, y+10-i*6/10, fg); }
    rect(d, x+12, y+2, 3, 2, fg); rect(d, x+13, y+2, 2, 4, fg); break;
  case 2:   // roll: drie streepjes die groeien
    rect(d, x+4, y+7, 2, 4, fg); rect(d, x+8, y+5, 2, 6, fg); rect(d, x+12, y+3, 2, 8, fg); break;
  case 3:   // skip: kruis
    for(int i=0; i<7; ++i){ px(d, x+5+i, y+3+i, fg); px(d, x+5+i, y+9-i, fg); } break;
  default:  // kans: korrelpatroon
    for(int yy=0; yy<7; ++yy) for(int xx=0; xx<10; ++xx) if( ((xx+yy)&1) == 0 ) px(d, x+4+xx, y+3+yy, fg);
  }
}

int og2_vel_style = 0;      // 0 verticaal balkje (huidig), 1 segmenten, 2 vulling, 3 boog, 4 balkdikte
void OG2_RenderCellBoxed(u8 *out, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer, og2_kind_t kind)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  char b[8];
  int on = s->on;
  const int bx = 4, by = 3, bw = 56, bh = 43;          // hoofdvak
  // hoofdvak: vulling bij een getriggerde stap, rand voor afspeelpositie
  if( on ) rect(d, bx, by, bw, bh, playing ? 6 : 2);
  frame_box(d, bx, by, bw, bh, playing ? 15 : (on ? 9 : 3));
  if( playing ) frame_box(d, bx+1, by+1, bw-2, bh-2, 15);
  if( trailing && !playing ) frame_box(d, bx+1, by+1, bw-2, bh-2, 5);
  if( og2_vel_style == 2 && on && kind == OG2_K_NOTE ){                    // velocity = vulniveau van het hele vak (van onder naar boven)
    int fh = (s->vel * (bh-2)) / 127;
    rect(d, bx+1, by+bh-1-fh, bw-2, fh, playing ? 8 : 5);
  }
  // stapnummer klein linksboven in het vak
  b[0] = '0'+(num+1)/10; b[1] = '0'+(num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, bx+3, by+3, b, 1, on ? 9 : 4);
  if( layer && layer[0] ) text(d, GLCD_FONT_SMALL, bx+bw-3-tw(GLCD_FONT_SMALL, layer, 1), by+3, layer, 1, on ? 8 : 4);   // parameterlabel
  if( kind == OG2_K_NOTE ){
    if( on && s->note != 0xff ){
      if( s->arp ) { int k; for(k=0; k<5 && s->txt[k]; ++k) b[k] = s->txt[k]; b[k] = 0; } else note_name(s->note, b);
      int w = tw(GLCD_FONT_NORMAL, b, 2), cx = bx + bw / 2;
      text(d, GLCD_FONT_NORMAL, cx - w/2 - 1, by+14, b, 2, 15); text(d, GLCD_FONT_NORMAL, cx - w/2, by+14, b, 2, 15);
      // onderin het vak: links lengte (horizontaal balkje; glide = loopt door), rechts velocity als blokje met getal (0-127)
      int len = 2 + (s->len > 100 ? 100 : s->len) * 26 / 100;
      for(int x=0; x<29; ++x) px(d, bx+5+x, by+bh-8, 3);                       // spoor
      rect(d, bx+5, by+bh-9, len, 3, 13);
      if( s->sld ) rect(d, bx+5+len, by+bh-9, 29-len, 3, 15);
      char vb[4]; int v = s->vel;
      vb[0] = v >= 100 ? '0'+v/100 : ' '; vb[1] = v >= 10 ? '0'+(v/10)%10 : ' '; vb[2] = '0'+v%10; vb[3] = 0;
      int vx = bx + bw - 26, vy = by + bh - 14;
      rect(d, vx, vy, 22, 11, 11);                                              // blokje
      text(d, GLCD_FONT_NORMAL, vx+2, vy+2, vb, 1, 0);                          // getal, donker op het blokje
    } else {
      hline(d, bx+16, by+bh/2+2, 24, 3, 2);
    }
  } else {
    int w = tw(GLCD_FONT_NORMAL, s->txt, 2);
    int g = on ? 15 : 7;
    text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2 - 1, by+14, s->txt, 2, g); text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2, by+14, s->txt, 2, g);
    // kleine visualisatie: balk met de laagwaarde
    int fill = (s->lval * 44) / 127; if( kind == OG2_K_LEN ) fill = (s->lval > 95 ? 44 : (s->lval * 44) / 95);
    for(int x=0; x<44; ++x) px(d, bx+6+x, by+bh-6, 3);
    rect(d, bx+6, by+bh-8, fill, 4, on ? 12 : 6);
  }
  // 3 symboolvakjes: accent / glide / roll-skip-kans
  slot_icon(d, 2, 50, 0, s->acc);
  slot_icon(d, 23, 50, 1, s->sld);
  int k3 = s->roll ? 2 : (s->skip ? 3 : 4);
  slot_icon(d, 44, 50, k3, s->roll || s->skip || (s->prob < 100));
  tile_pack(out, tmp);
}

void OG2_StepRange(const og2_step_t st[16], int *lo, int *hi)
{
  int l = 127, h = 0;
  for(int i=0; i<16; ++i) if( st[i].on && st[i].note != 0xff ){ if( st[i].note < l ) l = st[i].note; if( st[i].note > h ) h = st[i].note; }
  if( l > h ){ l = 36; h = 48; }
  *lo = l; *hi = h;
}

// rendert 1 stap-kolom (64x64) naar 2048 bytes: 4 bit per pixel, 32 bytes per rij, eerste pixel = hoge nibble
void OG2_RenderCell(u8 *out, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer, og2_kind_t kind)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  (void)cell_note; (void)cell_layer;     // oude weergaven (blijven beschikbaar), de standaard is nu de vierkante-hokjes-layout
  tile_pack(out, tmp);
  OG2_RenderCellBoxed(out, num, s, lo, hi, playing, trailing, layer ? layer : "", kind);
  return;
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
    OG2_RenderCell(c, n, &st[n], lo, hi, play_step == n, trail_step == n, f->layer_name, f->kind);
    for(int y=0; y<OG2_H; ++y) for(int x=0; x<64; ++x)
      f->gray[d][y][k*64+x] = (x & 1) ? (c[y*32+x/2] & 15) : (c[y*32+x/2] >> 4);
  }
}
