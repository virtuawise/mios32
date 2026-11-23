// $Id$
/*
 * 4x OLED (SSD1322, 256x64) stap-weergave, zie seq_oled.h
 * Tekenen gebeurt in myproject/oled_gui/oled_gui2 (platformonafhankelijk, op de Mac getest).
 * Elke stap-kolom (64x64 px, 4 bit grijs) wordt apart gerenderd, alleen als zijn invoer verandert, en run-length
 * gecodeerd naar de client gestuurd (remote servermodus, SysEx 09 07).
 */
#include <mios32.h>
#include <string.h>
#include <stdio.h>
#include "tasks.h"

#include "seq_oled.h"
#include "seq_ui.h"
#include "seq_core.h"
#include "seq_bpm.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_layer.h"
#include "seq_midi_sysex.h"
#include "seq_mixer.h"
#include "seq_cc.h"
#include "seq_pattern.h"
#include "seq_lcd.h"
#include "seq_midi_port.h"
#include "seq_file.h"

#include "oled_gui2.h"

#define OLED_PERIOD_MS      20     // invoer verzamelen/vergelijken: max 50x per seconde
#define OLED_RESEND_MS    3000     // alles opnieuw sturen (client kan later binnenkomen)
#define OLED_SEND_TICK_MS    4     // hooguit 1 cel per 4 ms versturen
#define OLED_NUM_CELLS      16
#define OLED_SHOW_PLAYHEAD   0     // 1 = afspeelstap (rand om het vak) op de OLED's; 0 = alleen de GP-knoppen lopen mee (gebruikerswens)

static og2_step_t steps[16];
static u8 play_step = 0xff, trail_step = 0xff;
static int range_lo, range_hi;
static og2_kind_t cur_kind = OG2_K_NOTE;
static u8 cur_mode = 0;                 // 0 step view, 6 Event, 7 Divider/Length/Direction, 8 Mode, 9 column page
static og2_event_t evt;
static og2_timing_t tim;
static og2_mode_t mod;
static og2_page_t pg;
extern s32 SEQ_UI_TRKTRAN_OledGet(og2_page_t *p);
extern s32 SEQ_UI_TRKGRV_OledGet(og2_page_t *p);
extern s32 SEQ_UI_TRGASG_OledGet(og2_page_t *p);
extern s32 SEQ_UI_TRKMORPH_OledGet(og2_page_t *p);
extern s32 SEQ_UI_MANUAL_OledGet(og2_page_t *p);
extern s32 SEQ_UI_FX_OledGet(og2_page_t *p);
extern s32 SEQ_UI_SYSEX_OledGet(og2_page_t *p);
extern s32 SEQ_UI_MUTE_OledGet(og2_page_t *p);
extern s32 SEQ_UI_PATTERN_OledGet(og2_page_t *p);
extern s32 SEQ_UI_SONG_OledGet(og2_page_t *p);
extern s32 SEQ_UI_UTIL_OledGet(og2_page_t *p);
extern s32 SEQ_UI_SAVE_OledGet(og2_page_t *p);
extern s32 SEQ_UI_MENU_OledGet(og2_page_t *p);
extern s32 SEQ_UI_BPM_OledGet(og2_page_t *p);
extern s32 SEQ_UI_OPT_OledGet(og2_page_t *p);
extern s32 SEQ_UI_INFO_OledGet(og2_page_t *p);
extern s32 SEQ_UI_FX_SCALE_OledGet(og2_page_t *p);

/////////////////////////////////////////////////////////////////////////////
// Stable copy of LCD row 2: the cursor blinks the selected value away for a moment; such characters are kept for
// LCD_KEEP periods (so grouping and values do not jump) and marked as blinking (= selected item).
/////////////////////////////////////////////////////////////////////////////
#define LCD_KEEP 75                      // x OLED_PERIOD_MS = 1.5 s
static char lcd_s1[80];
static u8 lcd_age[80];
static u8 lcd_blinkbits[10];
static seq_ui_page_t lcd_snap_page = SEQ_UI_PAGE_NONE;
static void lcd_snapshot(void)
{
  int x;
  if( lcd_snap_page != ui_page ) { memset(lcd_s1, ' ', sizeof(lcd_s1)); memset(lcd_age, LCD_KEEP, sizeof(lcd_age)); lcd_snap_page = ui_page; }
  memset(lcd_blinkbits, 0, sizeof(lcd_blinkbits));
  for(x=0; x<80; ++x) {
    char ch = SEQ_LCD_BufferCharGet(x, 1);
    if( ch < 32 || ch > 126 ) ch = '#';                   // bar / meter characters
    if( ch != ' ' ) { lcd_s1[x] = ch; lcd_age[x] = 0; }
    else if( lcd_s1[x] != ' ' && lcd_age[x] < LCD_KEEP ) { ++lcd_age[x]; lcd_blinkbits[x >> 3] |= 1 << (x & 7); }
    else lcd_s1[x] = ' ';
  }
}
static u8 lcd_blink(int x0, int x1)          // any blinking character in [x0, x1)
{
  int x;
  for(x=x0; x<x1 && x<80; ++x) if( lcd_blinkbits[x >> 3] & (1 << (x & 7)) ) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Column pages from a table: for pages whose LCD values sit in the 5-character GP columns (FX pages).
// Per item: the GP columns (same masks as the page's original LED handler) and an English label; the value is read
// from the LCD (row 2) in those columns. The selected value blinks on the LCD, so the last visible text is kept.
/////////////////////////////////////////////////////////////////////////////
typedef struct { u16 mask; const char *label; } lcd_item_t;
typedef struct { seq_ui_page_t page; const char *title; u8 track; const char *head[4]; u8 n; const lcd_item_t *it; } lcd_page_t;

static const lcd_item_t it_echo[] = { {0x0001,"TRACK"},{0x0002,"ECHO"},{0x000c,"REPEATS"},{0x0030,"DELAY"},{0x00c0,"VEL. LEVEL"},
				      {0x0300,"VELOCITY"},{0x0c00,"NOTE"},{0x3000,"GATE LENGTH"},{0xc000,"TICKS"} };
static const lcd_item_t it_hum[]  = { {0x0001,"TRACK"},{0x000e,"INTENSITY"},{0x0010,"NOTE"},{0x0020,"VEL/CC"},{0x00c0,"LENGTH"} };
static const lcd_item_t it_rob[]  = { {0x0001,"TRACK"},{0x0002,"ROBOTIZE"},{0x0004,"PROB."},{0x0008,"SKIP"},{0x0010,"OCTAVE"},{0x0020,"NOTE"},
				      {0x0040,"VEL/CC"},{0x0080,"LENGTH"},{0x0100,"SUSTAIN"},{0x0200,"NO FX"},{0x0400,"+ECHO"},{0x0800,"+DUPL."} };
static const lcd_item_t it_lim[]  = { {0x0001,"TRACK"},{0x0002,"LOWER"},{0x0004,"UPPER"} };
static const lcd_item_t it_lfo[]  = { {0x0001,"TRACK"},{0x0002,"WAVE"},{0x0004,"AMPL."},{0x0008,"PHASE"},{0x0010,"STEPS"},{0x0020,"RESET"},
				      {0x0040,"ONE SHOT"},{0x0080,"CLK DIV"},{0x0100,"NOTE"},{0x0200,"VELOCITY"},{0x0400,"LENGTH"},{0x0800,"CC"},
				      {0x2000,"EXTRA CC"},{0x1000,"CC #"},{0x4000,"OFFSET"},{0x8000,"PPQN"} };
static const lcd_item_t it_dup[]  = { {0x0001,"TRACK"},{0x0002,"ENABLE"},{0x001c,"CHANNELS"},{0x00e0,"FIRST CHANNEL"},{0x0100,"PORT"},
				      {0x0600,"NON-NOTES"},{0xf800,"MODE"} };
static const lcd_item_t it_loop[] = { {0x001f,"LOOP MODE"},{0x0020,"OFFSET"},{0x0040,"STEPS"},{0x0100,"LOOP"} };

#define LP(pg, t, trk, h0, h1, h2, h3, arr) { pg, t, trk, { h0, h1, h2, h3 }, sizeof(arr)/sizeof(lcd_item_t), arr }
static const lcd_page_t lcd_pages[] = {
  LP(SEQ_UI_PAGE_FX_ECHO,     "FX ECHO",      1, "", "", "FEEDBACK", "", it_echo),
  LP(SEQ_UI_PAGE_FX_HUMANIZE, "FX HUMANIZE",  1, "", "", "", "", it_hum),
  LP(SEQ_UI_PAGE_FX_ROBOTIZE, "FX ROBOTIZE",  1, "", "", "PROBABILITY", "", it_rob),
  LP(SEQ_UI_PAGE_FX_LIMIT,    "FX LIMIT",     1, "", "", "", "", it_lim),
  LP(SEQ_UI_PAGE_FX_LFO,      "FX LFO",       1, "", "", "TARGETS", "EXTRA CC", it_lfo),
  LP(SEQ_UI_PAGE_FX_DUPL,     "FX DUPLICATE", 1, "", "", "", "", it_dup),
  LP(SEQ_UI_PAGE_FX_LOOP,     "FX LOOP (GLOBAL)", 0, "", "", "", "", it_loop),
};


static s32 lcd_page_fill(void)
{
  const lcd_page_t *lp = 0;
  int i, k;
  for(i=0; i<(int)(sizeof(lcd_pages)/sizeof(lcd_page_t)); ++i)
    if( lcd_pages[i].page == ui_page ) lp = &lcd_pages[i];
  if( !lp ) return -1;

  u8 track = SEQ_UI_VisibleTrackGet();
  OG2_PageInit(&pg);
  for(k=0; k<4; ++k) strncpy(pg.head[k], lp->head[k], 23);
  if( lp->track ) sprintf(pg.head[0], "%s G%dT%d", lp->title, (track/4)+1, (track%4)+1);
  else strncpy(pg.head[0], lp->title, 23);

  for(i=0; i<lp->n; ++i) {
    u16 m = lp->it[i].mask;
    int first = 0, n = 0;
    while( first < 16 && !(m & (1 << first)) ) ++first;
    while( first + n < 16 && (m & (1 << (first + n))) ) ++n;
    if( first >= 16 ) continue;
    // value text from the LCD (row 2), trimmed; "label: value" -> value
    char v[28]; int o = 0, x;
    for(x = first*5; x < (first+n)*5 && o < 27; ++x) {
      char ch = lcd_s1[x];
      if( ch == '<' || ch == '>' || ch == '#' ) ch = ' ';
      v[o++] = (ch >= 32 && ch < 127) ? ch : ' ';
    }
    v[o] = 0;
    { char *cp = strrchr(v, ':'); if( cp && cp[1] ) memmove(v, cp+1, strlen(cp+1)+1); }
    { int a = 0; while( v[a] == ' ' ) ++a; if( a ) memmove(v, v+a, strlen(v+a)+1); }
    { int e = strlen(v); while( e > 0 && v[e-1] == ' ' ) v[--e] = 0; }

    u8 on = 0, tog = 0;
    if( !strcmp(v, "on") || !strcmp(v, "yes") ) { tog = 1; on = 1; }
    else if( !strcmp(v, "off") || !strcmp(v, "no") ) { tog = 1; }
    og2_col_t *c = OG2_PageCol(&pg, first, tog ? OG2_C_TOGGLE : OG2_C_VALUE, n, lp->it[i].label);
    c->on = on;
    if( !tog ) strncpy(c->value, v, sizeof(c->value)-1);
    if( ui_selected_item == i ) pg.sel = m;
  }
  return 0;
}

// MIDI page: sections on the left (GP1-8), the settings of the chosen section on the right (GP9-16).
// Section and selected item come from the page's own LED pattern; values from the LCD (right half).
extern u16 seq_ui_page_gp_leds;
typedef struct { u16 mask; const char *label; } midi_col_t;
static const midi_col_t midi_cols[5][8] = {
  { {0x0100,"BUS"},{0x0200,"PORT"},{0x0400,"CHANNEL"},{0x0800,"LOWER"},{0x1000,"UPPER"},{0x2000,"MODE"},{0xc000,"RESET"},{0,0} },
  { {0x0100,"PORT"},{0x0200,"CHANNEL"},{0x0400,"OCT. G1"},{0x0800,"OCT. G2"},{0x1000,"OCT. G3"},{0x2000,"OCT. G4"},{0x4000,"FORWARD"},{0x8000,"RESET"} },
  { {0x0100,"NODE"},{0x0200,"IN PORT"},{0x0400,"IN CHN"},{0x0800,"OUT PORT"},{0x1000,"OUT CHN"},{0xc000,"DEF. PORT"},{0,0},{0,0} },
  { {0x0100,"IN PORT"},{0x0200,"OUT PORT"},{0x0400,"CHANNEL"},{0x3800,"FUNCTION"},{0x4000,"CC #"},{0,0},{0,0},{0,0} },
  { {0x0300,"BLM SCALAR"},{0xc000,"MIDI"},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0} },
};
static void lcd_value(char *v, int first, int n, int strip_label)     // v: 32 bytes; strip_label: "Port: off" -> "off"
{
  int o = 0, x;
  for(x = first*5; x < (first+n)*5 && o < 31; ++x) {
    char ch = lcd_s1[x];
    if( ch == '<' || ch == '>' || ch == '|' || ch == '#' ) ch = ' ';
    v[o++] = (ch >= 32 && ch < 127) ? ch : ' ';
  }
  v[o] = 0;
  if( strip_label ) { char *cp = strrchr(v, ':'); if( cp && cp[1] ) memmove(v, cp+1, strlen(cp+1)+1); }
  { int a = 0; while( v[a] == ' ' ) ++a; if( a ) memmove(v, v+a, strlen(v+a)+1); }
  { int e = strlen(v); while( e > 0 && v[e-1] == ' ' ) v[--e] = 0; }
}
// one column (or a box over n columns) from the LCD: label from row 1 (NULL = take it from the LCD), value from row 2.
// The selected value blinks on the LCD: the last visible text is kept.
void SEQ_OLED_LcdCol(og2_page_t *p, int first, int n, const char *label, u8 sel)
{
  char v[32], l[32];
  int x, o;
  if( first < 0 || first > 15 ) return;
  lcd_value(v, first, n, 0);
  if( !label ) {
    for(o=0, x=first*5; x<(first+n)*5 && o<31; ++x) { char ch = SEQ_LCD_BufferCharGet(x, 0); l[o++] = (ch >= 32 && ch < 127) ? ch : ' '; }
    l[o] = 0;
    { int a = 0; while( l[a] == ' ' ) ++a; if( a ) memmove(l, l+a, strlen(l+a)+1); }
    { int e = strlen(l); while( e > 0 && l[e-1] == ' ' ) l[--e] = 0; }
    for(x=0; l[x]; ++x) if( l[x] >= 'a' && l[x] <= 'z' ) l[x] -= 32;
    label = l;
  }
  if( !v[0] && !label[0] ) return;
  og2_col_t *c = OG2_PageCol(p, first, OG2_C_VALUE, n, label);
  strncpy(c->value, v, sizeof(c->value)-1);
  if( sel ) p->sel |= ((1 << n) - 1) << first;
}

static s32 midi_page_fill(void)
{
  static const char *sec[5] = {"TRANSP./ARP","SECTIONS","ROUTER","EXT.CTRL","MISC"};
  static const u8 sec_first[5] = {0, 2, 4, 6, 7}, sec_n[5] = {2, 2, 2, 1, 1};
  static const char *sec_head[5] = {"TRANSPOSER/ARP INPUT","KEYBOARD SECTIONS","MIDI ROUTER","EXTERNAL CONTROL","MISC"};
  static u8 sub = 0;
  u16 leds = seq_ui_page_gp_leds;
  int i;
  // section = the low byte of the page's LED pattern (0x03, 0x0c, 0x30, 0x40, 0x80)
  for(i=0; i<5; ++i) { u8 m = ((1 << sec_n[i]) - 1) << sec_first[i]; if( (leds & 0xff) == m ) sub = i; }
  OG2_PageInit(&pg);
  strcpy(pg.head[0], "MIDI");
  strncpy(pg.head[2], sec_head[sub], 23);
  for(i=0; i<5; ++i) {
    og2_col_t *c = OG2_PageCol(&pg, sec_first[i], OG2_C_CHOICE, sec_n[i], "SECTION");
    strcpy(c->value, sec[i]);
    c->on = (i == sub);
  }
  for(i=0; i<8; ++i) {
    const midi_col_t *mc = &midi_cols[sub][i];
    int first = 0, n = 0;
    char v[32];
    if( !mc->mask ) continue;
    while( first < 16 && !(mc->mask & (1 << first)) ) ++first;
    while( first + n < 16 && (mc->mask & (1 << (first + n))) ) ++n;
    lcd_value(v, first, n, 1);
    u8 tog = (!strcmp(v, "on") || !strcmp(v, "off")) ? 1 : 0;
    og2_col_t *c = OG2_PageCol(&pg, first, tog ? OG2_C_TOGGLE : OG2_C_VALUE, n, mc->label);
    if( tog ) c->on = !strcmp(v, "on"); else strncpy(c->value, v, sizeof(c->value)-1);
  }
  pg.sel = leds & 0xff00;
  return 0;
}

// Mixer page as a column page: one box per mixer channel (GP1-16), value of the selected mixer parameter
extern u8 SEQ_UI_MIXER_ParGet(void);
extern u8 SEQ_UI_MIXER_UtilActive(void);
static s32 mixer_page_fill(void)
{
  static const char pname[16][8] = {"PORT","CHANNEL","PROGRAM","VOLUME","PANORAMA","REVERB","CHORUS","MOD","CC1","CC2","CC3","CC4","CC1 #","CC2 #","CC3 #","CC4 #"};
  u8 par = SEQ_UI_MIXER_ParGet();
  int t;
  if( SEQ_UI_MIXER_UtilActive() ) return -1;
  if( par > 15 ) par = 0;
  OG2_PageInit(&pg);
  sprintf(pg.head[0], "MIXER - %s", pname[par]);
  { char *mn = SEQ_MIXER_MapNameGet(); sprintf(pg.head[3], "MAP %d", (int)SEQ_MIXER_NumGet() + 1); if( mn && mn[0] && mn[0] != ' ' && strncmp(mn, "Unnamed", 7) ) { strcat(pg.head[3], " "); strncat(pg.head[3], mn, 12); } }
  for(t=0; t<16; ++t) {
    og2_col_t *c;
    char lbl[8];
    s32 v = SEQ_MIXER_Get(t, par);
    if( v < 0 ) v = 0;
    sprintf(lbl, "CH %d", t + 1);
    c = OG2_PageCol(&pg, t, OG2_C_VALUE, 1, lbl);
    if( par == SEQ_MIXER_PAR_PORT ) {
      const char *nm = SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet((mios32_midi_port_t)v));
      strncpy(c->value, nm ? nm : "---", 4);
    } else if( par == SEQ_MIXER_PAR_CHANNEL ) {
      sprintf(c->value, "%d", (int)(v & 0x0f) + 1);
    } else if( par == SEQ_MIXER_PAR_PANORAMA ) {
      int p = (int)(v & 0x7f) - 64;
      if( v == 0 ) { strcpy(c->value, "---"); c->dim = 1; }
      else if( p == 0 ) strcpy(c->value, "C");
      else sprintf(c->value, "%c%d", p < 0 ? 'L' : 'R', p < 0 ? -p : p);
    } else if( v == 0 && par >= SEQ_MIXER_PAR_PRG ) {
      strcpy(c->value, "---"); c->dim = 1;               // 0 = not used (as on the LCD)
    } else
      sprintf(c->value, "%d", (int)(v & 0x7f));
    if( ui_selected_item == t ) pg.sel = 1 << t;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Generic column page from the LCD text (pages without their own description).
// The LCD shows labels on row 1 and values on row 2, mostly in the 5-character GP columns. Neighbouring columns are
// joined only where a word (label or value) crosses the column border. A value that blinks (LCD cursor) is the
// selected item: it gets the double frame and keeps its last visible text.
/////////////////////////////////////////////////////////////////////////////

static void gen_trim_upper(char *s, u8 *sel)
{
  int i, o = 0;
  for(i=0; s[i]; ++i) {
    char ch = s[i];
    if( ch == '>' || ch == '<' ) { if( sel ) *sel = 1; ch = ' '; }
    if( ch < 32 || ch > 126 ) ch = ' ';
    if( ch >= 'a' && ch <= 'z' ) ch -= 32;
    s[o++] = ch;
  }
  s[o] = 0;
  { int a = 0; while( s[a] == ' ' ) ++a; if( a ) memmove(s, s+a, strlen(s+a)+1); }
  { int e = strlen(s); while( e > 0 && s[e-1] == ' ' ) s[--e] = 0; }
  // collapse runs of spaces
  for(i=0, o=0; s[i]; ++i) if( !(s[i] == ' ' && o > 0 && s[o-1] == ' ') ) s[o++] = s[i];
  s[o] = 0;
}

static s32 lcd_generic_fill(void)
{
  char r0[81], r1[81];
  int x, k;
  for(x=0; x<80; ++x) { r0[x] = SEQ_LCD_BufferCharGet(x, 0); r1[x] = lcd_s1[x]; }
  r0[80] = r1[80] = 0;
  for(x=0; x<80; ++x) if( r0[x] < 32 || r0[x] > 126 ) r0[x] = ' ';

  OG2_PageInit(&pg);
  {
    const char *pn = SEQ_UI_PAGES_PageNameGet(ui_page);
    char h[24]; int i;
    for(i=0; pn && pn[i] && i<23; ++i) h[i] = pn[i];
    h[i] = 0;
    gen_trim_upper(h, NULL);
    strcpy(pg.head[0], h);
  }
  k = 0;
  while( k < 16 ) {
    int n = 1;
    // join with the next column while a word crosses the border (row 1 or row 2)
    while( k + n < 16 ) {
      int b = 5*(k+n);
      #define GEN_SP(ch) ((ch) == ' ' || (ch) == '>' || (ch) == '<' || (ch) == '#')
      u8 cross0 = r0[b-1] != ' ' && r0[b] != ' ';
      u8 cross1 = !GEN_SP(r1[b-1]) && !GEN_SP(r1[b]);
      if( !(cross0 || cross1) ) break;
      ++n;
    }
    char l[84], v[84];
    u8 sel = 0;
    if( n > 1 ) {
      int i, words = 1, marked = -1;
      for(i=1; i<n; ++i) { int b = 5*(k+i); if( !GEN_SP(r1[b-1]) && !GEN_SP(r1[b]) ) words = 0; }
      for(i=0; i<n && words; ++i) { int x0 = 5*(k+i); for(x=x0; x<x0+5; ++x) if( r1[x] == '>' || r1[x] == '<' ) marked = i; }
      if( words ) {                                      // one text per column: a row of GP buttons under one title
	char t[84], w[8];
	og2_col_t *c;
	int o = 0;
	memcpy(t, &r0[5*k], 5*n); t[5*n] = 0;
	gen_trim_upper(t, NULL);
	c = OG2_PageCol(&pg, k, OG2_C_ROW, n, t);
	for(i=0; i<n; ++i) {
	  memcpy(w, &r1[5*(k+i)], 5); w[5] = 0;
	  for(x=0; x<5; ++x) if( w[x] == '#' ) w[x] = ' ';
	  gen_trim_upper(w, NULL);
	  if( i ) c->value[o++] = '|';
	  if( i == marked && o < 26 ) c->value[o++] = '>';
	  for(x=0; w[x] && o < 26; ++x) c->value[o++] = w[x];
	}
	c->value[o] = 0;
	if( marked >= 0 && lcd_blink(5*(k+marked), 5*(k+marked+1)) ) pg.sel |= 1 << (k+marked);
	k += n;
	continue;
      }
    }
    memcpy(l, &r0[5*k], 5*n); l[5*n] = 0;
    memcpy(v, &r1[5*k], 5*n); v[5*n] = 0;
    for(x=0; v[x]; ++x) if( v[x] == '#' ) v[x] = ' ';     // bar/meter characters
    gen_trim_upper(l, NULL);
    gen_trim_upper(v, &sel);
    if( lcd_blink(5*k, 5*(k+n)) ) sel = 1;             // blinking value = selected item
    if( l[0] || v[0] ) {
      u8 tog = !strcmp(v, "ON") || !strcmp(v, "OFF") || !strcmp(v, "YES") || !strcmp(v, "NO");
      if( !v[0] ) {                                      // only a label (title text): show it as the value
	og2_col_t *c = OG2_PageCol(&pg, k, OG2_C_VALUE, n, "");
	strncpy(c->value, l, sizeof(c->value)-1);
	c->dim = 1;
      } else {
	og2_col_t *c = OG2_PageCol(&pg, k, tog ? OG2_C_TOGGLE : OG2_C_VALUE, n, "");
	strncpy(c->label, l, sizeof(c->label)-1);
	if( tog ) c->on = !strcmp(v, "ON") || !strcmp(v, "YES");
	else strncpy(c->value, v, sizeof(c->value)-1);
	if( sel ) pg.sel |= ((1 << n) - 1) << k;
      }
    }
    k += n;
  }
  return 0;
}

// text of LCD row (0 = row 1, 1 = row 2 from the stable copy), columns [x0, x1), trimmed, upper case; out: 81 bytes
void SEQ_OLED_LcdText(char *out, int x0, int x1, int row)
{
  int x, o = 0;
  for(x=x0; x<x1 && x<80 && o<80; ++x) {
    char ch = row ? lcd_s1[x] : SEQ_LCD_BufferCharGet(x, 0);
    if( ch < 32 || ch > 126 || ch == '#' || ch == '<' || ch == '>' ) ch = ' ';
    out[o++] = ch;
  }
  out[o] = 0;
  gen_trim_upper(out, NULL);
}

// selection from the page's own LED pattern: the item whose GP columns are lit
static void sel_from_leds(void)
{
  extern u16 seq_ui_page_gp_leds;
  pg.sel = seq_ui_page_gp_leds;
}

// Track Instrument: GP1-6 as on the LCD, GP8 event page, GP9 name, name/category on the right
static s32 trkinst_fill(void)
{
  static const char *lbl[6] = {"TRACK","PORT","CHANNEL","PROGRAM","BANK H","BANK L"};
  int k;
  char t[84];
  og2_col_t *c;
  if( SEQ_LCD_BufferCharGet(0, 0) != 'T' ) return -1;      // name editor / dialogs: generic view
  OG2_PageInit(&pg);
  strcpy(pg.head[0], "TRACK INSTRUMENT");
  strcpy(pg.head[2], "EDIT");
  for(k=0; k<6; ++k) SEQ_OLED_LcdCol(&pg, k, 1, lbl[k], 0);
  c = OG2_PageCol(&pg, 7, OG2_C_ACTION, 1, "TRACK"); strcpy(c->value, "EVENT");
  c = OG2_PageCol(&pg, 8, OG2_C_ACTION, 1, "EDIT"); strcpy(c->value, "NAME");
  SEQ_OLED_LcdText(t, 55, 80, 1);
  c = OG2_PageCol(&pg, 11, OG2_C_VALUE, 5, "TRACK NAME"); strncpy(c->value, t[0] ? t : "---", sizeof(c->value)-1);
  sel_from_leds();
  return 0;
}

// Euclid: GP1-11 as on the LCD (labels of GP4/5 change with the event mode), the generated pattern on the right
static s32 euclid_fill(void)
{
  static const char *lbl[11] = {"TRACK","TRACK LENGTH",0,0,"VEL N","VEL A","RND ACC.","LENGTH","PULSES","OFFSET",0};
  char t[44];
  og2_col_t *c;
  OG2_PageInit(&pg);
  strcpy(pg.head[0], "EUCLID GENERATOR");
  SEQ_OLED_LcdCol(&pg, 0, 1, lbl[0], 0);
  SEQ_OLED_LcdCol(&pg, 1, 2, lbl[1], 0);
  SEQ_OLED_LcdCol(&pg, 3, 1, NULL, 0);
  SEQ_OLED_LcdCol(&pg, 4, 1, NULL, 0);
  SEQ_OLED_LcdCol(&pg, 5, 1, lbl[4], 0);
  SEQ_OLED_LcdCol(&pg, 6, 1, lbl[5], 0);
  SEQ_OLED_LcdCol(&pg, 7, 1, lbl[6], 0);
  SEQ_OLED_LcdCol(&pg, 8, 1, lbl[7], 0);
  SEQ_OLED_LcdCol(&pg, 9, 1, lbl[8], 0);
  SEQ_OLED_LcdCol(&pg, 10, 1, lbl[9], 0);
  {
    int x, o = 0;
    for(x=60; x<80 && o<27; ++x) { char ch = SEQ_LCD_BufferCharGet(x, 0); if( ch == '*' || ch == '.' ) t[o++] = (ch == '*') ? 'X' : '.'; }
    t[o] = 0;
  }
  c = OG2_PageCol(&pg, 12, OG2_C_VALUE, 4, "PATTERN"); strcpy(c->value, t[0] ? t : "---");
  sel_from_leds();
  return 0;
}

// MIDI monitor: per column the port, label = input (name or last event), value = output (name or last event)
static s32 midimon_fill(void)
{
  int k;
  if( seq_ui_button_state.SELECT_PRESSED ) return -1;     // filter settings: generic view
  OG2_PageInit(&pg);
  strcpy(pg.head[0], "MIDI MONITOR (IN / OUT)");
  for(k=0; k<16; ++k) {
    char in[84], out[84];
    og2_col_t *c;
    SEQ_OLED_LcdText(in, 5*k, 5*k+5, 0);
    SEQ_OLED_LcdText(out, 5*k, 5*k+5, 1);
    if( !in[0] && !out[0] ) continue;
    c = OG2_PageCol(&pg, k, OG2_C_VALUE, 1, in[0] ? in : "---");
    strcpy(c->value, out[0] ? out : "---");
  }
  return 0;
}

// pages that only show a message (LCD rows 1 and 2) in one text box
static s32 message_fill(void)
{
  char a[84], b[84];
  og2_col_t *c;
  OG2_PageInit(&pg);
  {
    const char *pn = SEQ_UI_PAGES_PageNameGet(ui_page);
    int i; for(i=0; pn && pn[i] && i<23; ++i) pg.head[0][i] = pn[i];
    pg.head[0][i] = 0; gen_trim_upper(pg.head[0], NULL);
  }
  SEQ_OLED_LcdText(a, 0, 80, 0);
  SEQ_OLED_LcdText(b, 0, 80, 1);
  c = OG2_PageCol(&pg, 0, OG2_C_TEXT, 16, "MESSAGE");
  snprintf(pg.text, sizeof(pg.text), "%s|%s", a, b);
  (void)c;
  return 0;
}

// Disk main page (dialogs: generic view): GP1 session import, GP2-3 export, GP5 MIDI file import, GP6-7 export, GP8 play
static s32 disk_fill(void)
{
  og2_col_t *c;
  char a[84];
  SEQ_OLED_LcdText(a, 0, 40, 0);
  if( strncmp(a, "SESSIONS", 8) ) return -1;
  OG2_PageInit(&pg);
  strcpy(pg.head[0], "DISK - SESSIONS");
  strcpy(pg.head[1], "MIDI FILES");
  c = OG2_PageCol(&pg, 0, OG2_C_ACTION, 1, "SESSION"); strcpy(c->value, "IMPORT");
  c = OG2_PageCol(&pg, 1, OG2_C_ACTION, 2, "SESSION"); strcpy(c->value, "EXPORT");
  c = OG2_PageCol(&pg, 4, OG2_C_ACTION, 1, "FILE"); strcpy(c->value, "IMPORT");
  c = OG2_PageCol(&pg, 5, OG2_C_ACTION, 2, "FILE"); strcpy(c->value, "EXPORT");
  c = OG2_PageCol(&pg, 7, OG2_C_ACTION, 1, "FILE"); strcpy(c->value, "PLAY");
  return 0;
}

// pages that describe themselves as a column page (og2_page_t); < 0 = not available (text view instead)
static s32 page_fill(void)
{
  switch( ui_page ) {
    case SEQ_UI_PAGE_TRKTRAN:  return SEQ_UI_TRKTRAN_OledGet(&pg);
    case SEQ_UI_PAGE_TRKGRV:   return SEQ_UI_TRKGRV_OledGet(&pg);
    case SEQ_UI_PAGE_TRGASG:   return SEQ_UI_TRGASG_OledGet(&pg);
    case SEQ_UI_PAGE_TRKMORPH: return SEQ_UI_TRKMORPH_OledGet(&pg);
    case SEQ_UI_PAGE_MANUAL:   return SEQ_UI_MANUAL_OledGet(&pg);
    case SEQ_UI_PAGE_FX:       return SEQ_UI_FX_OledGet(&pg);
    case SEQ_UI_PAGE_MIDI:     return midi_page_fill();
    case SEQ_UI_PAGE_SYSEX:    return SEQ_UI_SYSEX_OledGet(&pg);
    case SEQ_UI_PAGE_MIXER:    return mixer_page_fill();
    case SEQ_UI_PAGE_MUTE:     return SEQ_UI_MUTE_OledGet(&pg);
    case SEQ_UI_PAGE_PATTERN:  return SEQ_UI_PATTERN_OledGet(&pg);
    case SEQ_UI_PAGE_SONG:     return SEQ_UI_SONG_OledGet(&pg);
    case SEQ_UI_PAGE_UTIL:     return SEQ_UI_UTIL_OledGet(&pg);
    case SEQ_UI_PAGE_SAVE:     return SEQ_UI_SAVE_OledGet(&pg);
    case SEQ_UI_PAGE_MENU:     return SEQ_UI_MENU_OledGet(&pg);
    case SEQ_UI_PAGE_BPM:      return SEQ_UI_BPM_OledGet(&pg);
    case SEQ_UI_PAGE_OPT:      return SEQ_UI_OPT_OledGet(&pg);
    case SEQ_UI_PAGE_TRKINST:  return trkinst_fill();
    case SEQ_UI_PAGE_TRKEUCLID: return euclid_fill();
    case SEQ_UI_PAGE_MIDIMON:  return midimon_fill();
    case SEQ_UI_PAGE_TRKLIVE:  return message_fill();
    case SEQ_UI_PAGE_INFO:     return SEQ_UI_INFO_OledGet(&pg);
    case SEQ_UI_PAGE_FX_SCALE: return SEQ_UI_FX_SCALE_OledGet(&pg);
    case SEQ_UI_PAGE_DISK:     return disk_fill();
    default:                   return (lcd_page_fill() >= 0) ? 0 : lcd_generic_fill();
  }
}
extern s32 SEQ_UI_TRKMODE_OledGet(og2_mode_t *m);
extern s32 SEQ_UI_TRKEVNT_OledGet(og2_event_t *e);
#include "seq_ui_timing.h"
static char cur_layer_name[8] = "VEL";
static u32 sent_hash[OLED_NUM_CELLS];
static u8 sent_valid[OLED_NUM_CELLS];
static u8 dirty[OLED_NUM_CELLS];
static u32 cur_hash[OLED_NUM_CELLS];
static u32 ctr = 0, resend_ctr = 0;
static u8 next_cell = 0;
static u8 last_menu_button = 0xff;
static u32 last_beat_no = 0xffffffff;
static u8 cell_buf[2048];
static u8 rle_buf[2112];

s32 SEQ_OLED_Init(u32 mode)
{
  int c;
  for(c=0; c<OLED_NUM_CELLS; ++c)
    sent_valid[c] = 0;
  return 0;
}

static u32 fnv(u32 h, const u8 *p, int n)
{
  while( n-- ) { h ^= *p++; h *= 16777619u; }
  return h;
}

// verzamelt de 16 zichtbare stappen van de geselecteerde track
static void collect_steps(void)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u8 instrument = ui_selected_instrument;
  u16 base = (u16)ui_selected_step_view * 16;
  int i;

  // gekozen parameterlaag bepaalt de weergave
  u8 layer = ui_selected_par_layer;
  seq_par_layer_type_t ltype = SEQ_PAR_AssignmentGet(track, layer);
  switch( ltype ) {
    case SEQ_PAR_Type_Note: case SEQ_PAR_Type_Chord1: case SEQ_PAR_Type_Chord2: case SEQ_PAR_Type_Chord3: cur_kind = OG2_K_NOTE; break;
    case SEQ_PAR_Type_Velocity: cur_kind = OG2_K_VEL; break;
    case SEQ_PAR_Type_Length: cur_kind = OG2_K_LEN; break;
    case SEQ_PAR_Type_Probability: cur_kind = OG2_K_PROB; break;
    case SEQ_PAR_Type_Delay: cur_kind = OG2_K_DELAY; break;
    case SEQ_PAR_Type_Roll: cur_kind = OG2_K_ROLL; break;
    default: cur_kind = OG2_K_GENERIC; break;
  }
  {
    char tmp[16];
    tmp[0] = 0;
    if( SEQ_PAR_AssignedTypeStr(track, layer, instrument, tmp) >= 0 && tmp[0] ) {
      int k;
      for(k=0; k<4 && tmp[k]; ++k) cur_layer_name[k] = tmp[k];
      cur_layer_name[k] = 0;
    } else {
      strcpy(cur_layer_name, "PAR");
    }
  }

  for(i=0; i<16; ++i) {
    u16 step = base + i;
    seq_layer_evnt_t e;
    og2_step_t *s = &steps[i];
    memset(s, 0, sizeof(*s));
    s->note = 0xff;
    s->prob = 100;
    s->on = SEQ_TRG_GateGet(track, step, instrument) ? 1 : 0;
    s->acc = SEQ_TRG_AccentGet(track, step, instrument) ? 1 : 0;
    s->sld = SEQ_TRG_GlideGet(track, step, instrument) ? 1 : 0;
    s->roll = SEQ_TRG_RollGet(track, step, instrument) ? 1 : 0;
    s->skip = SEQ_TRG_SkipGet(track, step, instrument) ? 1 : 0;
    if( step < 256 ) {
      s32 p = SEQ_PAR_ProbabilityGet(track, step, instrument, 0x0000);
      if( p >= 0 && p <= 100 ) s->prob = p;
      s32 d = SEQ_PAR_StepDelayGet(track, step, instrument, 0x0000);
      if( d > 0 && d < 96 ) s->delay = (d * 100) / 96;
    }
    if( cur_kind != OG2_K_NOTE && step < 256 ) {
      s32 raw = SEQ_PAR_Get(track, step, layer, instrument);
      if( raw < 0 ) raw = 0;
      s->lval = raw & 0x7f;
      switch( cur_kind ) {
        case OG2_K_LEN:  if( raw >= 96 ) strcpy(s->txt, "Gld."); else sprintf(s->txt, "%d%%", (int)(raw * 100) / 96); break;
        case OG2_K_PROB: sprintf(s->txt, "%d%%", (int)(raw > 100 ? 100 : raw)); break;
        case OG2_K_ROLL:
          if( raw == 0 ) strcpy(s->txt, "----");
          else sprintf(s->txt, "%d%c%02d", (int)(((raw & 0x30)>>4) + 2), (raw & 0x40) ? 'U' : 'D', (int)(raw & 0xf));
          break;
        default: sprintf(s->txt, "%d", (int)raw); break;
      }
    }
    if( s->on && SEQ_LAYER_GetEvntOfLayer(track, step, 0, instrument, &e) >= 0 ) {
      s->note = e.midi_package.note;
      s->vel = e.midi_package.velocity & 0x7f;
      if( ltype == SEQ_PAR_Type_Note && SEQ_CC_Get(track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator ) {
	// Arpeggiator: de waarde is een arp-code (toets 1-4 + octaaf, of '*' = multi-arp), zoals SEQ_LCD_PrintArp
	u8 a = s->note;
	s->arp = 1;
	if( a < 4 ) {
	  strcpy(s->txt, "---");
	} else {
	  int key_num = (a >> 2) & 0x3, arp_oct = (a >> 4) & 0x7;
	  char kc;
	  if( arp_oct < 2 ) { kc = '*'; arp_oct = ((a >> 2) & 7) - 4; }
	  else { kc = '1' + key_num; arp_oct -= 4; }
	  sprintf(s->txt, "%c%c%d", kc, arp_oct >= 0 ? '+' : '-', arp_oct >= 0 ? arp_oct : -arp_oct);
	}
      }
      s32 l = e.len;
      if( l < 0 && step < 256 ) l = SEQ_PAR_LengthGet(track, step, instrument, 0x0000);
      if( l < 0 ) l = 48;
      if( l >= 96 ) {            // lengte 'Gld.' = glide (legato naar de volgende noot)
	s->len = 100;
	s->sld = 1;
      } else {
	s->len = (l * 100) / 96;
      }
      s->value = s->vel;       // actieve laag: velocity (later: laag van het paneel)
    }
  }

  // mute: track gemute of de gekozen parameterlaag gemute -> arcering + M in alle vakken
  {
    u8 muted = (((seq_core_trk_muted >> track) & 1) || ((seq_core_trk[track].layer_muted >> (layer & 0x0f)) & 1)) ? 1 : 0;
    for(i=0; i<16; ++i)
      steps[i].muted = muted;
  }

  // glide: loopt van een stap met Gld. naar de volgende getriggerde stap op de pagina, ook door lege stappen heen
  for(i=0; i<16; ++i) {
    steps[i].glide_to = 0xff;
    steps[i].gl_kind = 0;
  }
  for(i=0; i<16; ++i) {
    if( steps[i].sld && steps[i].on && steps[i].note != 0xff ) {
      int j, m;
      for(j=i+1; j<16; ++j)
	if( steps[j].on && steps[j].note != 0xff ) break;
      if( j < 16 ) {
	int p0 = steps[i].note * 16, p1 = steps[j].note * 16;
	int D = (64 - 46) + 64 * (j - i - 1) + 4;           // horizontale afstand van balk-einde tot balk-begin (px)
	steps[i].gl_kind = 1;
	steps[i].glide_to = steps[j].note;
	steps[i].gl_b = p0 + ((p1 - p0) * 15) / D;            // helling bij de rechterrand van de bronstap (x 46 -> 61)
	for(m=i+1; m<j; ++m) {
	  int a0 = (64 - 46) + 64 * (m - i - 1);
	  steps[m].gl_kind = 2;
	  steps[m].gl_a = p0 + ((p1 - p0) * a0) / D;
	  steps[m].gl_b = p0 + ((p1 - p0) * (a0 + 63)) / D;
	}
      }
    }
  }

  u8 new_play = 0xff;
  if( SEQ_BPM_IsRunning() ) {
    u8 cur = seq_core_trk[track].step;
    if( (cur / 16) == ui_selected_step_view )
      new_play = cur % 16;
  }
#if !OLED_SHOW_PLAYHEAD
  new_play = 0xff;           // geen afspeelstap op de schermen
#endif
  if( new_play != play_step ) {
    trail_step = (play_step != 0xff) ? play_step : 0xff;
    play_step = new_play;
  }
  OG2_StepRange(steps, &range_lo, &range_hi);
}

// run-length (PackBits): 0..127 = kopieer c+1 bytes; 128..255 = herhaal volgende byte (c-126) keer
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

// menu button (GP1..GP16 = 0..15) that belongs to the current UI page, 0x7f = none
u8 SEQ_OLED_MenuButtonGet(void)
{
  switch( ui_page ) {
    case SEQ_UI_PAGE_MIXER:        return 0;
    case SEQ_UI_PAGE_TRKEVNT:      return 1;
    case SEQ_UI_PAGE_TRKMODE:      return 2;
    case SEQ_UI_PAGE_TRKDIR:       return 3;
    case SEQ_UI_PAGE_TRKDIV:       return 4;
    case SEQ_UI_PAGE_TRKLEN:       return 5;
    case SEQ_UI_PAGE_TRKTRAN:      return 6;
    case SEQ_UI_PAGE_TRKGRV:       return 7;
    case SEQ_UI_PAGE_TRGASG:       return 8;
    case SEQ_UI_PAGE_FX:
    case SEQ_UI_PAGE_FX_ECHO:
    case SEQ_UI_PAGE_FX_HUMANIZE:
    case SEQ_UI_PAGE_FX_ROBOTIZE:
    case SEQ_UI_PAGE_FX_LIMIT:
    case SEQ_UI_PAGE_FX_LFO:
    case SEQ_UI_PAGE_FX_DUPL:
    case SEQ_UI_PAGE_FX_LOOP:
    case SEQ_UI_PAGE_FX_SCALE:     return 9;
    case SEQ_UI_PAGE_MANUAL:       return 10;
    case SEQ_UI_PAGE_TRKMORPH:     return 11;
    case SEQ_UI_PAGE_BPM:          return 12;
    case SEQ_UI_PAGE_SAVE:         return 13;
    case SEQ_UI_PAGE_MIDI:         return 14;
    case SEQ_UI_PAGE_SYSEX:        return 15;
    default:                       return 0x7f;
  }
}

s32 SEQ_OLED_Handler(void)
{
  // alleen in remote-servermodus (virtueel paneel); echte OLED-hardware volgt later
  if( !(seq_midi_sysex_remote_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER ||
        seq_midi_sysex_remote_active_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER) )
    return 0;

  ++ctr;
  ++resend_ctr;

  if( (ctr % OLED_PERIOD_MS) == 0 ) {
    int c;
    lcd_snapshot();
    // 0 step view, 6 Event, 7 Divider/Length/Direction, 8 Mode: own layouts; 9 = column page (own description or LCD converter)
    if( ui_page == SEQ_UI_PAGE_EDIT ) cur_mode = 0;
    else if( ui_page == SEQ_UI_PAGE_TRKEVNT && SEQ_UI_TRKEVNT_OledGet(&evt) >= 0 ) cur_mode = 6;
    else if( ui_page == SEQ_UI_PAGE_TRKDIV || ui_page == SEQ_UI_PAGE_TRKLEN || ui_page == SEQ_UI_PAGE_TRKDIR ) cur_mode = 7;
    else if( ui_page == SEQ_UI_PAGE_TRKMODE ) cur_mode = 8;
    else {
      if( page_fill() < 0 ) lcd_generic_fill();
      cur_mode = 9;
    }
    if( cur_mode == 0 )
      collect_steps();
    else if( cur_mode == 7 )
      SEQ_UI_TIMING_OledGet(&tim);
    else if( cur_mode == 8 )
      SEQ_UI_TRKMODE_OledGet(&mod);
    // mode 9: column page (pg was filled when the mode was chosen)
    // mode 6: Event page in its own layout (evt was filled when the mode was chosen)
    if( resend_ctr >= OLED_RESEND_MS ) {
      resend_ctr = 0;
      for(c=0; c<OLED_NUM_CELLS; ++c)
	sent_valid[c] = 0;
    }
    for(c=0; c<OLED_NUM_CELLS; ++c) {
      u32 h = 2166136261u;
      h = fnv(h, &cur_mode, 1);
      if( cur_mode == 0 )
	h = fnv(h, (const u8 *)&steps[c], sizeof(og2_step_t));
      else if( cur_mode == 6 )
	h = fnv(h, (const u8 *)&evt, sizeof(og2_event_t));
      else if( cur_mode == 7 )
	h = fnv(h, (const u8 *)&tim, sizeof(og2_timing_t));
      else if( cur_mode == 8 )
	h = fnv(h, (const u8 *)&mod, sizeof(og2_mode_t));
      else if( cur_mode == 9 ) {
	// only what this cell shows: header of its screen, the boxes that cover it, selection (level meters change often)
	int j;
	h = fnv(h, (const u8 *)pg.head[c / 4], sizeof(pg.head[0]));
	for(j=0; j<16; ++j) {
	  int n = pg.c[j].span ? pg.c[j].span : 1;
	  if( pg.c[j].kind != OG2_C_NONE && pg.c[j].kind != OG2_C_SPAN && c >= j && c < j + n ) {
	    u8 sb = ((pg.sel >> j) & ((1 << n) - 1)) ? 1 : 0;
	    h = fnv(h, (const u8 *)&pg.c[j], sizeof(og2_col_t));
	    h = fnv(h, &sb, 1);
	    if( pg.c[j].kind == OG2_C_GRAPH ) h = fnv(h, (const u8 *)pg.graph, sizeof(pg.graph) + 2);
	    if( pg.c[j].kind == OG2_C_TEXT ) h = fnv(h, (const u8 *)pg.text, sizeof(pg.text));
	  }
	}
      }
      h = fnv(h, (const u8 *)&range_lo, sizeof(int));
      h = fnv(h, (const u8 *)&range_hi, sizeof(int));
      u8 flags[3] = { play_step == c, trail_step == c, cur_kind };
      h = fnv(h, flags, 3);
      h = fnv(h, (const u8 *)cur_layer_name, 4);
      cur_hash[c] = h;
      dirty[c] = !sent_valid[c] || sent_hash[c] != h;
    }
  }

  // beat pulse (one per quarter note, from the sequencer clock) for the BEAT LED of the client
  if( SEQ_BPM_IsRunning() ) {
    u32 ppqn = SEQ_BPM_PPQN_Get();
    if( ppqn < 4 ) ppqn = 96;
    u32 beat_no = SEQ_BPM_TickGet() / ppqn;
    if( beat_no != last_beat_no ) {
      if( SEQ_MIDI_SYSEX_REMOTE_Server_SendBeat(beat_no & 3) >= 0 )
	last_beat_no = beat_no;
    }
  } else {
    last_beat_no = 0xffffffff;
  }

  // tell the client which menu button belongs to the current page (on change and with every full resend)
  if( (ctr % OLED_PERIOD_MS) == 7 ) {
    u8 mb = SEQ_OLED_MenuButtonGet();
    if( mb != last_menu_button || resend_ctr >= OLED_RESEND_MS - 100 ) {
      if( SEQ_MIDI_SYSEX_REMOTE_Server_SendMenuButton(mb) >= 0 )
	last_menu_button = mb;
    }
  }

  // verstuur hooguit 1 gewijzigde cel per OLED_SEND_TICK_MS
  if( (ctr % OLED_SEND_TICK_MS) == 1 ) {
    int tries;
    for(tries=0; tries<OLED_NUM_CELLS; ++tries) {
      u8 c = next_cell;
      next_cell = (next_cell + 1) % OLED_NUM_CELLS;
      if( dirty[c] ) {
	if( cur_mode == 6 )
	  OG2_RenderEventCell(cell_buf, c, &evt);
	else if( cur_mode == 7 )
	  OG2_RenderTimingCell(cell_buf, c, &tim);
	else if( cur_mode == 8 )
	  OG2_RenderModeCell(cell_buf, c, &mod);
	else if( cur_mode == 9 )
	  OG2_RenderPageCell(cell_buf, c, &pg);
	else
	  OG2_RenderCell(cell_buf, c, &steps[c], range_lo, range_hi, play_step == c, trail_step == c, cur_layer_name, cur_kind);
	int n = rle_encode(cell_buf, sizeof(cell_buf), rle_buf);
	if( SEQ_MIDI_SYSEX_REMOTE_Server_SendOLEDCell(c / 4, c % 4, rle_buf, n) >= 0 ) {
	  sent_hash[c] = cur_hash[c];
	  sent_valid[c] = 1;
	  dirty[c] = 0;
	}
	break;
      }
    }
  }

  return 0;
}
