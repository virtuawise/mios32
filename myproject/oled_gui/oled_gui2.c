#include "oled_gui2.h"
#include <glcd_font.h>
#include <string.h>

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

static void frame_box(u8 (*d)[CW], int x, int y, int w, int h, int g);
// ---- tegel: mixerkanaal in hokjes (zoals het stapscherm) ----
// groot vak = waarde van de gekozen mixerpagina; 3 kleine vakjes eronder = poort, MIDI-kanaal, programma
static void slot_text(u8 (*d)[CW], int x, int y, const char *t, int on)
{
  frame_box(d, x, y, 18, 13, on ? 9 : 3);
  int w = tw(GLCD_FONT_SMALL, t, 1);
  text(d, GLCD_FONT_SMALL, x + 9 - w/2, y+3, t, 1, on ? 15 : 5);
}

void OG2_RenderMixerTile(u8 *out, int num, const og2_tile_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  char b[8];
  int sel = t->flags & 1;
  const int bx = 4, by = 3, bw = 56, bh = 43;
  if( sel ) rect(d, bx, by, bw, bh, 2);
  frame_box(d, bx, by, bw, bh, sel ? 15 : 9);
  if( sel ) frame_box(d, bx+1, by+1, bw-2, bh-2, 15);
  // kanaalnummer klein linksboven, paginanaam rechtsboven
  b[0] = '0' + (num+1)/10; b[1] = '0' + (num+1)%10; b[2] = 0;
  text(d, GLCD_FONT_SMALL, bx+3, by+3, b, 1, sel ? 15 : 9);
  text(d, GLCD_FONT_SMALL, bx+bw-3-tw(GLCD_FONT_SMALL, t->pname, 1), by+3, t->pname, 1, sel ? 12 : 8);
  // waarde groot
  { int sc = (tw(GLCD_FONT_NORMAL, t->txt, 2) > 48) ? 1 : 2, w = tw(GLCD_FONT_NORMAL, t->txt, sc), ty = sc == 2 ? by+14 : by+17;
    text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2 - 1, ty, t->txt, sc, 15); text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2, ty, t->txt, sc, 15); }
  // balkje onderin (pan: schuif met middenstreep)
  if( t->vmax ){
    for(int x=0; x<46; ++x) px(d, bx+5+x, by+bh-7, 3);
    if( t->pan ){
      rect(d, bx+5+23, by+bh-11, 1, 7, 5);
      rect(d, bx+5 + (t->v1 * 42) / t->vmax, by+bh-10, 4, 6, 15);
    } else {
      int w = (t->v1 * 46) / t->vmax; if( w > 46 ) w = 46;
      rect(d, bx+5, by+bh-8, w, 3, 13);
    }
  }
  // 3 kleine vakjes: poort, MIDI-kanaal, programma
  char c2[6]; int k = 0; if( t->chn+1 >= 10 ) c2[k++] = '0' + (t->chn+1)/10; c2[k++] = '0' + (t->chn+1)%10; c2[k] = 0;
  char c3[6]; if( t->v3 ) { k = 0; if( t->v3 >= 100 ) c3[k++] = '0' + t->v3/100; if( t->v3 >= 10 ) c3[k++] = '0' + (t->v3/10)%10; c3[k++] = '0' + t->v3%10; c3[k] = 0; } else { c3[0] = '-'; c3[1] = '-'; c3[2] = 0; }
  slot_text(d, 2, 50, t->port, 1);
  slot_text(d, 23, 50, c2, 1);
  slot_text(d, 44, 50, c3, t->v3 != 0);
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

// ---- platte tekst (pagina's zonder eigen ontwerp): 5 tekens per GP-kolom, 2x vergroot ----
static void plain_line(u8 (*d)[CW], int y, const char *s)
{
  for(int i=0; i<5; ++i){
    unsigned char c = (unsigned char)s[i];
    int x = 2 + i*12;
    if( c == ' ' || c == 0 ) continue;
    if( c < 32 ){ for(int yy=0; yy<12; ++yy) for(int xx=0; xx<8; ++xx) if( ((xx+yy)&1) == 0 ) px(d, x+xx+1, y+2+yy, 9); continue; }
    if( c == 0x7e ) c = '>';
    if( c == 0x7f ) c = '<';
    if( c > 0x7e ) c = '?';
    chr(d, GLCD_FONT_NORMAL, x, y, c, 2, 15);
    chr(d, GLCD_FONT_NORMAL, x+1, y, c, 2, 15);
  }
}

void OG2_RenderPlainCell(u8 *out, int cell, const og2_text_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  if( (cell & 3) != 0 ) for(int y=0; y<64; y+=4) px(d, 0, y, 2);
  plain_line(d, 6, t->l1);
  hline(d, 4, 31, 56, 2, 3);
  plain_line(d, 38, t->l2);
  tile_pack(out, tmp);
}

// ---- gegroepeerde hokjes-weergave van de LCD-tekst ----
static char tg_char(u8 c){ return (c < 32 || c > 126) ? '#' : (char)c; }

static void tg_text(char *o, const u8 *row, int c0, int n, int *sel)
{
  int a = c0*5, b = (c0+n)*5, i, j = 0;
  while( b > a && (row[b-1] == ' ' || row[b-1] == '<' || row[b-1] == '>') ){ if( row[b-1] == '<' ) *sel = 1; --b; }
  while( a < b && (row[a] == ' ' || row[a] == '>' || row[a] == '<') ){ if( row[a] == '>' ) *sel = 1; ++a; }
  for(i=a; i<b && j<27; ++i) o[j++] = tg_char(row[i]);
  o[j] = 0;
}

void OG2_TextGroups(const u8 *row0, const u8 *row1, og2_tgroup_t g[16])
{
  int occ[16];
  for(int c=0; c<16; ++c){ occ[c] = 0; for(int i=0; i<5; ++i) if( (row0[c*5+i] != ' ') || (row1[c*5+i] != ' ') ) occ[c] = 1; }
  for(int c=0; c<16; ++c){ g[c].n = 0; g[c].start = c; g[c].sel = 0; g[c].label[0] = 0; g[c].value[0] = 0; }
  int c = 0;
  while( c < 16 ){
    if( !occ[c] ){ ++c; continue; }
    int c1 = c;
    // een woord dat over de grens naar de volgende cel loopt (in regel 1 of 2) voegt de cellen samen
    while( c1 + 1 < 16 && occ[c1+1] &&
           ((row0[c1*5+4] != ' ' && row0[(c1+1)*5] != ' ') || (row1[c1*5+4] != ' ' && row1[(c1+1)*5] != ' ')) ) ++c1;
    if( c1 - c + 1 > 3 ) c1 = c;          // zeer lange ketens (twee rijen die door elkaar lopen, bv. Direction): niet samenvoegen
    int n = c1 - c + 1, sel = 0;
    char lab[28], val[28];
    tg_text(lab, row0, c, n, &sel);
    tg_text(val, row1, c, n, &sel);
    for(int k=c; k<=c1; ++k){
      g[k].start = c; g[k].n = n; g[k].sel = sel;
      for(int q=0; q<28; ++q){ g[k].label[q] = lab[q]; g[k].value[q] = val[q]; }
    }
    c = c1 + 1;
  }
}

void OG2_RenderTextGroupCell(u8 *out, int cell, const og2_tgroup_t *g)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  if( g->n == 0 ){ tile_pack(out, tmp); return; }     // empty column: nothing
  ox_shift = (cell - g->start) * 64;
  // same grid and rules as the column pages (DESIGN.md): box y 11..60, label top-left, value 1x bold upper case at y 34
  const int bx = 3, by = 11, bw = g->n * 64 - 6, bh = 50;
  char lu[28], vu[28]; int na = 0, nb = 0;
  for(; g->label[na] && na < 27; ++na) lu[na] = g->label[na];
  lu[na] = 0;
  for(; g->value[nb] && nb < 27; ++nb) vu[nb] = (g->value[nb] >= 'a' && g->value[nb] <= 'z') ? g->value[nb] - 32 : g->value[nb];
  vu[nb] = 0;
  if( g->sel ) rect(d, bx, by, bw, bh, 2);
  frame_box(d, bx, by, bw, bh, g->sel ? 15 : 9);
  if( g->sel ) frame_box(d, bx+1, by+1, bw-2, bh-2, 15);
  int cx = bx + bw/2;
  if( na > 0 ) text(d, GLCD_FONT_NORMAL, bx+5, by+4, lu, 1, g->sel ? 12 : 9);
  if( nb > 0 ){ int w = tw(GLCD_FONT_NORMAL, vu, 1); text(d, GLCD_FONT_NORMAL, cx - w/2, 34, vu, 1, 15); text(d, GLCD_FONT_NORMAL, cx - w/2 + 1, 34, vu, 1, 15); }
  ox_shift = 0;
  tile_pack(out, tmp);
}

// 'hokjes'-weergave van de LCD-tekst: elk onderdeel (5 tekens per GP-kolom) krijgt een vak; regel 1 = label klein, regel 2 = waarde groot.
// Gekozen onderdelen op het LCD staan tussen '>' en '<' -> felle dubbele rand.
static int trim_copy(char *o, const char *s, int *sel)
{
  int n = 5, i = 0, j;
  while( n > 0 && (s[n-1] == ' ' || s[n-1] == 0 || s[n-1] == '<' || s[n-1] == '>') ){ if( s[n-1] == '<' ) *sel = 1; --n; }
  while( i < n && (s[i] == ' ' || s[i] == '>' || s[i] == '<') ){ if( s[i] == '>' ) *sel = 1; ++i; }
  for(j=0; i<n; ++i){ unsigned char c = (unsigned char)s[i]; o[j++] = (c < 32 || c > 126) ? '#' : (char)c; }
  o[j] = 0; return j;
}

void OG2_RenderTextCell(u8 *out, int cell, const og2_text_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  char a[8], b[8]; int sel = 0;
  int na = trim_copy(a, t->l1, &sel), nb = trim_copy(b, t->l2, &sel);
  const int bx = 4, by = 3, bw = 56, bh = 56;
  if( na == 0 && nb == 0 ){ for(int x=0; x<bw; x+=4) px(d, bx+x, by+bh/2, 2); tile_pack(out, tmp); return; }   // leeg: stipjes
  if( sel ) rect(d, bx, by, bw, bh, 2);
  frame_box(d, bx, by, bw, bh, sel ? 15 : 9);
  if( sel ) frame_box(d, bx+1, by+1, bw-2, bh-2, 15);
  if( nb > 0 && na > 0 ){                                  // label klein boven, waarde groot
    text(d, GLCD_FONT_NORMAL, bx+4, by+4, a, 1, sel ? 12 : 9);
    int sc = (tw(GLCD_FONT_NORMAL, b, 2) > 50) ? 1 : 2, w = tw(GLCD_FONT_NORMAL, b, sc);
    int ty = sc == 2 ? by+24 : by+28;
    text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2 - 1, ty, b, sc, 15); text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2, ty, b, sc, 15);
  } else {                                                 // maar 1 regel: die is de inhoud
    const char *m = na > 0 ? a : b;
    int sc = (tw(GLCD_FONT_NORMAL, m, 2) > 50) ? 1 : 2, w = tw(GLCD_FONT_NORMAL, m, sc);
    int ty = by + bh/2 - (sc == 2 ? 8 : 4);
    text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2 - 1, ty, m, sc, 15); text(d, GLCD_FONT_NORMAL, bx + bw/2 - w/2, ty, m, sc, 15);
  }
  tile_pack(out, tmp);
}

// ---- Event-pagina: eigen indeling (raster: blok y 11..60, marge 3, label links-boven, waarde gecentreerd, balkje onderin) ----
#define EBY 11
#define EBH 50
static int ebx(int k){ return k*64+3; }
static int ebw(int n){ return n*64-6; }
static void ecbtext(u8 (*d)[CW], int cx, int y, const char *s, int sc, int g){ int w = tw(GLCD_FONT_NORMAL, s, sc); text(d, GLCD_FONT_NORMAL, cx-w/2, y, s, sc, g); text(d, GLCD_FONT_NORMAL, cx-w/2+1, y, s, sc, g); }
// labels and small texts use the normal 6x8 font (the 4x5 small font looked squashed)
#define LFONT GLCD_FONT_NORMAL
static void ecsmall(u8 (*d)[CW], int cx, int y, const char *s, int g){ int w = tw(LFONT, s, 1); text(d, LFONT, cx-w/2, y-1, s, 1, g); }
static void ebox(u8 (*d)[CW], int k, int n, int sel){ int x = ebx(k), w = ebw(n); if( sel ) rect(d, x, EBY, w, EBH, 2); frame_box(d, x, EBY, w, EBH, sel ? 15 : 9); if( sel ) frame_box(d, x+1, EBY+1, w-2, EBH-2, 15); }
static void elabel(u8 (*d)[CW], int k, const char *l, int sel){ text(d, LFONT, ebx(k)+5, EBY+4, l, 1, sel ? 12 : 9); }
// label that is clipped to a box of n columns
static void elabel_n(u8 (*d)[CW], int k, int n, const char *l, int sel){ char b[44]; int i, m = (ebw(n) - 8) / 6; if( m > 43 ) m = 43; for(i=0; l[i] && i<m; ++i) b[i] = l[i]; b[i] = 0; elabel(d, k, b, sel); }
// UI rules (boxed pages): label top-left in LFONT; value always 1x bold upper case at VALY; no bars under values;
// on / active = filled box; last touched = double frame; lists = chips
#define VALY 34
static void evalue(u8 (*d)[CW], int k, int n, const char *v, int g)
{
  char u[32]; int i = 0, maxc = (ebw(n) - 8) / 6;      // clip to the box (6 px per character)
  if( maxc > 31 ) maxc = 31;
  for(; v[i] && i < maxc; ++i) u[i] = (v[i] >= 'a' && v[i] <= 'z' && !(v[i] == 'x' && v[i+1] >= '0' && v[i+1] <= '9')) ? v[i] - 32 : v[i];   // keep 'x1' (replay)
  u[i] = 0;
  ecbtext(d, ebx(k) + ebw(n)/2, VALY, u, 1, g);
}
static void evbox(u8 (*d)[CW], int k, const char *l, const char *v, int frac, int sel, int dim){ ebox(d, k, 1, sel); elabel(d, k, l, sel); evalue(d, k, 1, v, dim ? 5 : 15); (void)frac; }
static void epicker(u8 (*d)[CW], int k, int n, const char *l, const char **it, int cnt, int cols, int sel, int sel_box, int active_cnt)
{
  ebox(d, k, n, sel_box); elabel(d, k, l, sel_box);
  int rows = (cnt + cols - 1) / cols, ix = ebx(k)+5, iw = ebw(n)-10, iy = EBY+16, ih = EBH-24, gap = 2;     // >= 6 px free below the chips
  int cw = (iw - (cols-1)*gap) / cols, ch = (ih - (rows-1)*gap) / rows;
  for(int i=0; i<cnt; ++i){
    int cx = ix + (i % cols)*(cw+gap), cy = iy + (i / cols)*(ch+gap), s = (i == sel), act = i < active_cnt;
    if( s ) rect(d, cx, cy, cw, ch, 13); else frame_box(d, cx, cy, cw, ch, act ? 6 : 3);
    ecsmall(d, cx + cw/2, cy + ch/2 - 3, it[i], s ? 0 : (act ? 10 : 4));
  }
}
static void eheader(u8 (*d)[CW], int scr, const char *t){ text(d, LFONT, scr*256+5, 1, t, 1, 9); for(int x = scr*256+5+tw(LFONT, t, 1)+6; x < scr*256+251; x += 2) px(d, x, 5, 3); }

static void num2str(char *o, int v){ char t[6]; int n = 0; if( v == 0 ) t[n++] = '0'; while( v > 0 && n < 5 ){ t[n++] = '0' + v%10; v /= 10; } int k = 0; while( n > 0 ) o[k++] = t[--n]; o[k] = 0; }

void OG2_RenderEventCell(u8 *out, int cell, const og2_event_t *e)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int scr = cell / 4;
  ox_shift = cell * 64;
  char b[8];
  #define SELK(k) (e->sel == (k))
  switch( scr ){
  case 0: {
    eheader(d, 0, "EVENT - TRACK");
    evbox(d, 0, "TRACK", e->track, -1, SELK(0), 0);
    static const char *ty[4] = {"NOTE","CHORD","CC","DRUM"};
    epicker(d, 1, 2, "TYPE", ty, 4, 2, e->mode, SELK(1) || SELK(2), 4);
    num2str(b, e->steps);
    evbox(d, 3, "STEPS", b, e->size_n > 1 ? (e->size_idx * 100) / (e->size_n - 1) : 0, SELK(3), 0);
  } break;
  case 1: {
    eheader(d, 1, "OUTPUT");
    evbox(d, 4, "PORT", e->port, -1, SELK(4), 0);
    num2str(b, e->chn + 1);
    evbox(d, 5, "CHANNEL", b, (e->chn * 100) / 15, SELK(5), 0);
    ebox(d, 6, 1, SELK(6)); elabel(d, 6, "INSTR.", SELK(6));
    if( e->instruments > 1 ){ num2str(b, e->instruments); evalue(d, 6, 1, b, 15); } else { evalue(d, 6, 1, "--", 5); ecsmall(d, ebx(6)+ebw(1)/2, 48, "(DRUM)", 4); }
    ebox(d, 7, 1, 0); elabel(d, 7, "LAYERS", 0);
    text(d, LFONT, ebx(7)+5, 29, "PAR", 1, 8); num2str(b, e->par_layers); ecbtext(d, ebx(7)+ebw(1)-8-tw(GLCD_FONT_NORMAL, b, 1)/2, 29, b, 1, 15);
    text(d, LFONT, ebx(7)+5, 42, "TRG", 1, 8); num2str(b, e->trg_layers); ecbtext(d, ebx(7)+ebw(1)-8-tw(GLCD_FONT_NORMAL, b, 1)/2, 42, b, 1, 15);
  } break;
  case 2: if( e->mode == 3 ) {
    eheader(d, 2, "DRUM");
    evbox(d, 8, "LAYER A", e->lay_a, -1, SELK(8), 0);
    evbox(d, 9, "LAYER B", e->lay_b, -1, SELK(9), 0);
    num2str(b, e->drum_inst + 1);
    evbox(d, 10, "DRUM", b, e->instruments > 1 ? (e->drum_inst * 100) / (e->instruments - 1) : 0, SELK(10), 0);
    { char nn[8]; note_name(e->drum_note, nn); evbox(d, 11, "NOTE", nn, (e->drum_note * 100) / 127, SELK(11), 0); }
    break;
  } else {
    eheader(d, 2, "PARAMETER LAYERS");
    static const char *ly[16] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P"};
    epicker(d, 8, 2, "LAYER", ly, 16, 8, e->layer_sel, SELK(8) || SELK(9), e->par_layers);
    evbox(d, 10, "TYPE", e->ltype, -1, SELK(10) || e->ltype_pending, 0);
    ebox(d, 11, 1, SELK(11)); elabel(d, 11, "PARAM", SELK(11)); evalue(d, 11, 1, e->param, e->param[0] == '#' ? 15 : 5);
  } break;
  default: if( e->mode == 3 ) {
    eheader(d, 3, "ACTIONS");
    num2str(b, e->vel_n); evbox(d, 12, "VEL N", b, (e->vel_n * 100) / 127, SELK(12), 0);
    num2str(b, e->vel_a); evbox(d, 13, "VEL A", b, (e->vel_a * 100) / 127, SELK(13), 0);
    ebox(d, 14, 1, SELK(14)); elabel(d, 14, "PRESETS", SELK(14));
    { int cx = ebx(14)+ebw(1)/2, w = tw(LFONT, "SELECT", 1) + 6; frame_box(d, cx-w/2, 31, w, 15, 9); ecsmall(d, cx, 35, "SELECT", 15); }
    { int hot = e->init_needed || SELK(15);
      ebox(d, 15, 1, hot); elabel(d, 15, "INIT", hot);
      ecsmall(d, ebx(15)+ebw(1)/2, 29, "HOLD", e->init_needed ? 15 : 8); ecsmall(d, ebx(15)+ebw(1)/2, 38, "2 SEC", e->init_needed ? 15 : 8);
      int x = ebx(15)+5, w = ebw(1)-10; for(int i=0; i<w; ++i) px(d, x+i, 52, 3); if( e->init_pct ) rect(d, x, 50, (w * e->init_pct) / 100, 5, 15); }
    break;
  } else {
    eheader(d, 3, "ACTIONS");
    ebox(d, 12, 2, SELK(12) || SELK(13)); elabel(d, 12, "PRESETS", SELK(12) || SELK(13));
    { int cx = ebx(12)+ebw(2)/2, w = tw(GLCD_FONT_NORMAL, "SELECT", 1) + 14; frame_box(d, cx-w/2, VALY-4, w, 15, 9); ecbtext(d, cx, VALY, "SELECT", 1, 15); }
    int hot = e->init_needed || SELK(14) || SELK(15);
    ebox(d, 14, 2, hot); elabel(d, 14, "INIT", hot);
    { int cx = ebx(14)+ebw(2)/2; ecbtext(d, cx, VALY, e->init_needed ? "HOLD 2 SEC!" : "HOLD 2 SEC", 1, e->init_needed ? 15 : 8); }
    { int x = ebx(14)+5, w = ebw(2)-10; for(int i=0; i<w; ++i) px(d, x+i, 52, 3); if( e->init_pct ) rect(d, x, 50, (w * e->init_pct) / 100, 5, 15); }
  } break;
  }
  ox_shift = 0;
  tile_pack(out, tmp);
}

// ---- Timing page: same grid as the Event page ----
// on/off box: filled background when on, ON/OFF in the middle, full bar when on
static void etoggle(u8 (*d)[CW], int k, const char *l, int on, int sel, int dim)
{
  ebox(d, k, 1, sel);
  if( on && !sel ) rect(d, ebx(k)+1, EBY+1, ebw(1)-2, EBH-2, 3);
  elabel(d, k, l, sel || on);
  ecbtext(d, ebx(k)+ebw(1)/2, VALY, on ? "ON" : "OFF", 1, dim ? 4 : (on ? 15 : 7));
}
// small text right-aligned in the label row of a box spanning n columns
static void elabel_right(u8 (*d)[CW], int k, int n, const char *s, int g){ text(d, LFONT, ebx(k)+ebw(n)-5-tw(LFONT, s, 1), EBY+4, s, 1, g); }

// box for one option of a choice that has one column per option: the active option is filled
static void echoice(u8 (*d)[CW], int k, const char *l, const char *v, int on, int sel, int dim)
{
  ebox(d, k, 1, sel);
  if( on && !sel ) rect(d, ebx(k)+1, EBY+1, ebw(1)-2, EBH-2, 3);
  elabel(d, k, l, sel || on);
  evalue(d, k, 1, v, dim ? 4 : (on ? 15 : 6));
}
static void theader(u8 (*d)[CW], int scr, const char *t, const char *track)
{
  char h[28]; int n = 0;
  for(; t[n] && n < 18; ++n) h[n] = t[n];
  if( track ){ h[n++] = ' '; for(int i=0; track[i] && n < 26; ++i) h[n++] = track[i]; }
  h[n] = 0;
  eheader(d, scr, h);
}

void OG2_RenderTimingCell(u8 *out, int cell, const og2_timing_t *t)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int scr = cell / 4;
  ox_shift = cell * 64;
  char b[12];
  #define SELT(k) (t->sel == (k))
  if( t->page == 0 ) {                     // ---- DIVIDER ----
    int man = t->manual;
    switch( scr ){
    case 0:
      theader(d, 0, "DIVIDER", t->track);
      evbox(d, 0, "TRACK", t->track, -1, SELT(0), 0);
      if( man ) strcpy(b, "---"); else num2str(b, t->divider);
      evbox(d, 1, "DIVIDER", b, -1, SELT(1), man);
      etoggle(d, 2, "TRIPLET", t->triplet, SELT(2), man);
      etoggle(d, 3, "SYNC BAR", t->sync, SELT(3), man);
      break;
    case 1:
      theader(d, 1, "CLOCK", 0);
      etoggle(d, 4, "MANUAL", man, SELT(4), 0);
      ebox(d, 5, 3, 0); elabel(d, 5, "STEPS PER BAR", 0);
      if( man ) evalue(d, 5, 3, "MANUAL CLOCK", 6);
      else {   // a step lasts divider/16 of a quarter note... 256/divider steps per bar, x1.5 for triplets
        int x10 = (2560 * (t->triplet ? 3 : 2)) / (2 * t->divider);
        num2str(b, x10 / 10);
        if( x10 % 10 ){ int n = 0; while( b[n] ) ++n; b[n] = '.'; b[n+1] = '0' + x10 % 10; b[n+2] = 0; }
        evalue(d, 5, 3, b, 15);
      }
      break;
    default: {
      theader(d, scr, scr == 2 ? "TIMEBASE" : "", 0);
      static const char *tb[8] = {"1","2","4","8","8T","16","16T","32"};
      for(int i=0; i<4; ++i){ int k = scr*4 + i, n = k - 8;
        echoice(d, k, "TIMEBASE", tb[n], !man && t->tb_preset == n, SELT(k), man); }
    } break;
    }
  } else if( t->page == 1 ) {              // ---- LENGTH ----
    switch( scr ){
    case 0: {
      theader(d, 0, "LENGTH", t->track);
      evbox(d, 0, "TRACK", t->track, -1, SELT(0), 0);
      int over = t->length > t->num_steps;
      if( over ) strcpy(b, "!!!"); else num2str(b, t->length);
      evbox(d, 1, "LENGTH", b, -1, SELT(1), 0);
      num2str(b, t->loop);
      evbox(d, 2, "LOOP", b, -1, SELT(2), t->loop > t->length);
      evbox(d, 3, "PRESETS", t->qsel_loops ? "LOOPS" : "LENGTH", -1, SELT(3), 0);
    } break;
    case 1: {
      theader(d, 1, "OVERVIEW", 0);
      ebox(d, 4, 4, 0); elabel(d, 4, "STEPS", 0);
      num2str(b, t->num_steps); { char m[16] = "OF "; strcat(m, b); elabel_right(d, 4, 4, m, 9); }
      // ruler: the steps the track is prepared for, played range bright, loop part brightest
      int x0 = ebx(4) + 6, w = ebw(4) - 12, y = 33, h = 10, n = t->num_steps ? t->num_steps : 1;
      int len = t->length > n ? n : t->length, lp = t->loop > len ? len : t->loop;
      frame_box(d, x0-1, y-1, w+2, h+2, 4);
      rect(d, x0, y, (len * w) / n, h, 7);
      if( lp > 1 ) rect(d, x0, y, ((lp - 1) * w) / n, h, 4);
      for(int s=16; s<n; s+=16) for(int yy=y+h+2; yy<y+h+5; ++yy) px(d, x0 + (s * w) / n, yy, 6);
      num2str(b, t->length); { char m[24] = "PLAYS "; if( t->loop > 1 ){ char c[8]; num2str(c, t->loop); strcat(m, c); strcat(m, "-"); } else strcat(m, "1-"); strcat(m, b); ecsmall(d, ebx(4)+ebw(4)/2, 52, m, 9); }
    } break;
    default:
      theader(d, scr, scr == 2 ? "PRESETS" : "", 0);
      for(int i=0; i<4; ++i){ int k = scr*4 + i, n = k - 8;
        if( t->qsel_loops ){ char c[8]; num2str(b, t->loop_presets[n]); strcat(b, "-"); num2str(c, t->presets[n]); strcat(b, c); }
        else num2str(b, t->presets[n]);
        echoice(d, k, t->qsel_loops ? "LOOP" : "LENGTH", b, t->len_preset == n, SELT(k), t->presets[n] > t->num_steps); }
      break;
    }
  } else {                                 // ---- DIRECTION ----
    static const char *dn[7] = {"FORWARD","BACKWARD","PINGPONG","PENDULUM","RAND.DIR","RAND.STP","RAND.D+S"};
    switch( scr ){
    case 0:
      theader(d, 0, "DIRECTION", t->track);
      evbox(d, 0, "TRACK", t->track, -1, SELT(0), 0);
      for(int k=1; k<4; ++k) echoice(d, k, "DIR", dn[k-1], t->dir == k-1, SELT(k), 0);
      break;
    case 1:
      theader(d, 1, "", 0);
      for(int k=4; k<8; ++k) echoice(d, k, "DIR", dn[k-1], t->dir == k-1, SELT(k), 0);
      break;
    case 2:
      theader(d, 2, "STEPS", 0);
      num2str(b, t->fwd);  evbox(d, 8, "FORWARD", b, -1, SELT(8), 0);
      num2str(b, t->back); evbox(d, 9, "JMP BACK", b, -1, SELT(9), 0);
      ebox(d, 10, 2, SELT(10) || SELT(11)); elabel(d, 10, "REPLAY", SELT(10) || SELT(11));
      b[0] = 'x'; num2str(b+1, t->replay); evalue(d, 10, 2, b, 15);
      break;
    default:
      theader(d, 3, "REPEAT / SKIP", 0);
      num2str(b, t->repeat); evbox(d, 12, "REPEAT", b, -1, SELT(12), t->repeat == 0);
      num2str(b, t->skip);   evbox(d, 13, "SKIP", b, -1, SELT(13), t->skip == 0);
      num2str(b, t->itv);    evbox(d, 14, "INTERVAL", b, -1, SELT(14), t->repeat == 0 && t->skip == 0);
      etoggle(d, 15, "SYNC BAR", t->sync, SELT(15), 0);
      break;
    }
  }
  #undef SELT
  ox_shift = 0;
  tile_pack(out, tmp);
}

// ---- Mode page ----
static const char *const mode_help[16][4] = {
  {"OFF","The track plays nothing.","",""},
  {"NORMAL","Steps play their own notes.","",""},
  {"TRANSPOSE","Step notes are transposed by","the note held on the bus","(keyboard or another track)."},
  {"ARPEGGIATOR","Steps pick notes from the","notes held on the bus","(arp codes in the note layer)."},
  {"BUS","Which of the 4 busses feeds","this track in Transpose/Arp.",""},
  {"NOTE","FIRST: the first held note","is used for transposing,","LAST: the last played note."},
  {"HOLD","Held notes stay on the bus","after the keys are released.",""},
  {"SORT","ON: notes sorted by pitch,","OFF: in the order played.",""},
  {"RESTART","The track restarts when a","new note arrives after all","keys were released."},
  {"STEP TRIGGER","The track advances one step","with every incoming note.",""},
  {"FORCE TO SCALE","Notes are forced to the","global scale.",""},
  {"SUSTAIN","Notes are held until the","next note is played.",""},
  {"","","",""},{"","","",""},{"","","",""},{"","","",""}
};
void OG2_RenderModeCell(u8 *out, int cell, const og2_mode_t *m)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int scr = cell / 4, na = (m->mode < 2);      // note stack options only matter for Transpose / Arpeggiator
  ox_shift = cell * 64;
  char b[8];
  #define SELM(k) (m->sel == (k))
  switch( scr ){
  case 0: {
    char h[24] = "MODE ";
    strcat(h, m->track);
    eheader(d, 0, h);
    static const char *shrt[4] = {"OFF","NORMAL","TRANSP.","ARPEGGIO"};
    for(int i=0; i<4; ++i) echoice(d, i, "MODE", shrt[i], m->mode == i, SELM(i), 0);
  } break;
  case 1: {
    eheader(d, 1, "BUS / NOTE STACK");
    b[0] = '0' + 1 + (m->bus & 3); b[1] = 0;
    evbox(d, 4, "BUS", b, ((m->bus & 3) * 100) / 3, SELM(4), na);
    evbox(d, 5, "NOTE", m->first ? "FIRST" : "LAST", -1, SELM(5), na);
    etoggle(d, 6, "HOLD", m->hold, SELM(6), na);
    etoggle(d, 7, "SORT", m->sort, SELM(7), na);
  } break;
  case 2: {
    eheader(d, 2, "OPTIONS");
    etoggle(d, 8, "RESTART", m->restart, SELM(8), na);
    etoggle(d, 9, "STEP TRG", m->steptrg, SELM(9), na);
    etoggle(d, 10, "FORCE SC", m->fts, SELM(10), 0);
    etoggle(d, 11, "SUSTAIN", m->sustain, SELM(11), 0);
  } break;
  default: {
    eheader(d, 3, "INFO");
    int i = (m->sel < 4) ? m->mode & 3 : m->sel;   // mode columns: explain the active mode
    if( i > 11 ) i = (m->mode & 3);
    ebox(d, 12, 4, 0);
    text(d, LFONT, ebx(12)+5, EBY+4, mode_help[i][0], 1, 12);
    for(int l=1; l<4; ++l) text(d, LFONT, ebx(12)+5, EBY+5+l*11, mode_help[i][l], 1, 9);
  } break;
  }
  #undef SELM
  ox_shift = 0;
  tile_pack(out, tmp);
}

// ---- generic column page (all menu pages without a hand-made layout) ----
void OG2_PageInit(og2_page_t *p){ memset(p, 0, sizeof(*p)); p->graph_mark = -1; }
og2_col_t *OG2_PageCol(og2_page_t *p, int k, u8 kind, int span, const char *label)
{
  if( k < 0 || k > 15 ) k = 0;
  if( span < 1 ) span = 1;
  if( k + span > 16 ) span = 16 - k;
  og2_col_t *c = &p->c[k];
  memset(c, 0, sizeof(*c));
  c->kind = kind; c->span = span;
  if( label ){ int i = 0; for(; label[i] && i < 23; ++i) c->label[i] = label[i]; c->label[i] = 0; }
  for(int i=1; i<span; ++i){ memset(&p->c[k+i], 0, sizeof(og2_col_t)); p->c[k+i].kind = OG2_C_SPAN; }
  return c;
}
static void pg_box(u8 (*d)[CW], const og2_page_t *p, int j)
{
  const og2_col_t *c = &p->c[j];
  int n = c->span ? c->span : 1, sel = ((p->sel >> j) & ((1 << n) - 1)) != 0;
  int fill = (c->kind == OG2_C_TOGGLE || c->kind == OG2_C_CHOICE) && c->on && !c->dim;
  ebox(d, j, n, sel);
  if( fill && !sel ) rect(d, ebx(j)+1, EBY+1, ebw(n)-2, EBH-2, 3);
  elabel_n(d, j, n, c->label, sel || fill);
  int cx = ebx(j) + ebw(n)/2;
  switch( c->kind ){
  case OG2_C_VALUE:
    evalue(d, j, n, c->value, c->dim ? 5 : 15); break;
  case OG2_C_TOGGLE:
    evalue(d, j, n, c->on ? "ON" : "OFF", c->dim ? 4 : (c->on ? 15 : 7)); break;
  case OG2_C_CHOICE:
    evalue(d, j, n, c->value, c->dim ? 4 : (c->on ? 15 : 6)); break;
  case OG2_C_ACTION: {
    int w = tw(LFONT, c->value, 1) + 10; if( w > ebw(n) - 8 ) w = ebw(n) - 8;
    frame_box(d, cx - w/2, VALY-4, w, 15, c->dim ? 4 : 9); evalue(d, j, n, c->value, c->dim ? 5 : 15);
  } break;
  case OG2_C_TEXT: {
    char line[84]; int l = 0, o = 0;
    for(int i=0; ; ++i){
      char ch = p->text[i];
      if( ch == '|' || ch == 0 ){ line[o] = 0; text(d, LFONT, ebx(j)+5, EBY+15+l*11, line, 1, c->dim ? 5 : 10); ++l; o = 0; if( !ch || l >= 3 ) break; }
      else if( o < 83 && i < (int)sizeof(p->text) ) line[o++] = ch;
    }
  } break;
  case OG2_C_ROW: {
    const char *q = c->value;
    for(int i=0; i<n && *q; ++i){
      char t[12]; int o = 0, mark = 0;
      if( *q == '>' ){ mark = 1; ++q; }
      while( *q && *q != '|' ){ if( o < 9 ) t[o++] = *q; ++q; }
      t[o] = 0;
      if( *q == '|' ) ++q;
      if( !t[0] ) continue;
      int ccx = ebx(j+i) + ebw(1)/2, w = tw(LFONT, t, 1) + 6;
      if( w > ebw(1) - 4 ) w = ebw(1) - 4;
      if( mark ){ rect(d, ccx - w/2, VALY-3, w, 13, c->dim ? 6 : 13); ecbtext(d, ccx, VALY, t, 1, 0); }
      else ecbtext(d, ccx, VALY, t, 1, c->dim ? 5 : 12);
      if( i > 0 ) for(int y=EBY+16; y<EBY+EBH-6; y+=2) px(d, ebx(j+i)-3, y, 3);     // faint divider: one GP button per text
    }
  } break;
  case OG2_C_GRAPH: {
    if( c->value[0] ) elabel_right(d, j, n, c->value, 9);
    int x0 = ebx(j)+6, w = ebw(n)-12, yc = 41, cnt = p->graph_n ? p->graph_n : 1, bw = w / cnt;
    for(int x=0; x<w; x+=2) px(d, x0+x, yc, 3);
    for(int i=0; i<p->graph_n; ++i){
      int v = p->graph[i], h = (v * 14) / 100, x = x0 + i*bw, g = (i == p->graph_mark) ? 15 : (c->dim ? 4 : 9);
      if( h > 0 ) rect(d, x, yc - h, bw > 2 ? bw - 1 : 1, h, g);
      else if( h < 0 ) rect(d, x, yc + 1, bw > 2 ? bw - 1 : 1, -h, g);
      else rect(d, x, yc - 1, bw > 2 ? bw - 1 : 1, 3, g);
    }
  } break;
  case OG2_C_METER: {
    if( c->value[0] ) elabel_right(d, j, n, c->value, 12);
    int x0 = ebx(j)+6, w = ebw(n)-12, y = 30, h = 12, f = (c->meter > 100 ? 100 : c->meter) * w / 100;
    frame_box(d, x0-1, y-1, w+2, h+2, 4); rect(d, x0, y, f, h, c->dim ? 5 : 12);
    for(int i=1; i<n; ++i) for(int yy=y+h+3; yy<y+h+6; ++yy) px(d, ebx(j+i)-3, yy, 6);   // column borders: the button below sets that position
  } break;
  case OG2_C_KEYS: {
    if( c->value[0] ) elabel_right(d, j, n, c->value, 12);
    static const u8 white[7] = {0,2,4,5,7,9,11}, black[5] = {1,3,6,8,10}, bpos[5] = {1,2,4,5,6};
    int x0 = ebx(j)+8, w = ebw(n)-16, kw = w / 7, y = 26, hh = 28;
    for(int i=0; i<7; ++i){ int lit = (c->keys >> white[i]) & 1; if( lit ) rect(d, x0+i*kw+1, y+1, kw-1, hh-1, 13); frame_box(d, x0+i*kw, y, kw+1, hh+1, 7); }
    for(int i=0; i<5; ++i){ int lit = (c->keys >> black[i]) & 1, bx = x0 + bpos[i]*kw - kw/3; rect(d, bx, y, (2*kw)/3, hh*6/10, lit ? 15 : 1); frame_box(d, bx, y, (2*kw)/3, hh*6/10, 7); }
  } break;
  default: break;
  }
}
void OG2_RenderPageCell(u8 *out, int cell, const og2_page_t *p)
{
  static u8 tmp[OG2_H][CW];
  for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x) tmp[y][x] = 0;
  u8 (*d)[CW] = tmp;
  int scr = cell / 4;
  ox_shift = cell * 64;
  eheader(d, scr, p->head[scr]);
  for(int j=0; j<16; ++j){
    const og2_col_t *c = &p->c[j];
    if( c->kind == OG2_C_NONE || c->kind == OG2_C_SPAN ) continue;
    int n = c->span ? c->span : 1;
    if( cell >= j && cell < j + n ) pg_box(d, p, j);
  }
  ox_shift = 0;
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
// vakje van 18x13 met een getal (SMALL font); bij accent: omgekeerd (vol, donker getal)
static void slot_num(u8 (*d)[CW], int x, int y, int value, int has, int accent)
{
  char b[4];
  b[0] = value >= 100 ? '0'+value/100 : ' '; b[1] = value >= 10 ? '0'+(value/10)%10 : ' '; b[2] = '0'+value%10; b[3] = 0;
  if( !has ){ frame_box(d, x, y, 18, 13, 3); return; }
  if( accent ){ rect(d, x, y, 18, 13, 15); text(d, GLCD_FONT_SMALL, x+3, y+3, b, 1, 0); }
  else { frame_box(d, x, y, 18, 13, 9); text(d, GLCD_FONT_SMALL, x+3, y+3, b, 1, 15); }
}

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
      // onderin het vak: lengte als horizontaal balkje (glide loopt door tot de rand)
      int len = 2 + (s->len > 100 ? 100 : s->len) * 44 / 100;
      for(int x=0; x<46; ++x) px(d, bx+5+x, by+bh-7, 3);                       // spoor
      rect(d, bx+5, by+bh-8, len, 3, 13);
      if( s->sld ) rect(d, bx+5+len, by+bh-8, 46-len, 3, 15);
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
  slot_num(d, 2, 50, s->vel, on && s->note != 0xff, s->acc);      // vakje 1: velocity (omgekeerd bij accent)
  slot_icon(d, 23, 50, 1, s->sld);                                   // vakje 2: glide
  int k3 = s->roll ? 2 : (s->skip ? 3 : 4);
  slot_icon(d, 44, 50, k3, s->roll || s->skip || (s->prob < 100));
  if( s->muted ){                                                          // gemute: alles dimmen, arcering, M-badge
    for(int y=0; y<OG2_H; ++y) for(int x=0; x<CW; ++x){
      int v = tmp[y][x];
      v = (v * 5) / 10;                                                    // ~50% helderheid
      if( ((x + y) & 3) == 0 && y < 48 ) v = (v < 4) ? 4 : v;              // diagonale arcering (alleen in het hoofdvak)
      tmp[y][x] = v;
    }
    rect(d, bx+bw-12, by+2, 10, 10, 15);                                   // M-badge rechtsboven in het vak
    text(d, GLCD_FONT_NORMAL, bx+bw-10, by+2, "M", 1, 0);
  }
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
