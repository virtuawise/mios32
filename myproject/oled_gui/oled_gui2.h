// Tekenlaag v2: 4 x SSD1322 (256x64), 4 bit grijs (0..15). Stap-weergave met meerdere lagen tegelijk
// (pianoroll-balk: toonhoogte, lengte, velocity=helderheid, probability=korrel, delay=verschuiving) + iconen + actieve laagwaarde.
#ifndef _OLED_GUI2_H
#define _OLED_GUI2_H
#include <mios32.h>
#define OG2_DISPLAYS 4
#define OG2_W 256
#define OG2_H 64
typedef enum { OG2_K_NOTE=0, OG2_K_VEL, OG2_K_LEN, OG2_K_PROB, OG2_K_DELAY, OG2_K_ROLL, OG2_K_GENERIC } og2_kind_t;

typedef struct {
  u8 note;      // 0..127, 0xff = geen
  u8 vel;       // 0..127
  u8 len;       // gatelengte 0..100 (% van de stap; >100 mag voor legato)
  u8 prob;      // 0..100
  s8 delay;     // -? .. +: verschuiving in % van de stap (0..99)
  u8 on, acc, sld, roll, skip;
  u8 muted;     // 1 = track (of laag) is gemute: arcering + M
  u8 arp;       // 1 = Arpeggiator-track: de 'noot' is een arp-code, de tekst staat in txt (bv "1-2", "*+2")
  u8 glide_to;  // (oud) noot waar de glide naartoe loopt, 0xff = onbekend
  u8 gl_kind;   // 0 geen, 1 = bron van een glide, 2 = stap waar de glide doorheen loopt (lege stap tussen bron en doel)
  s16 gl_a, gl_b; // toonhoogte*16 van de glide-lijn aan de linker (alleen kind 2) / rechterrand van de cel
  u8 value;     // waarde van de actieve laag (0..127) onderin (alleen NOTE-weergave)
  u8 lval;      // ruwe waarde van de gekozen parameterlaag (0..127)
  char txt[6];  // tekst van die waarde, bv "75%", "Gld.", "3U05" (max 4 tekens + 0)
} og2_step_t;
typedef struct {
  u8 gray[OG2_DISPLAYS][OG2_H][OG2_W];   // 1 byte per pixel, 0..15 (prototype; verpakking naar 4 bit volgt)
  const char *layer_name;                // naam van de actieve laag, bv "VEL"
  og2_kind_t kind;                       // soort laag: bepaalt de weergave
} og2_frame_t;
// ---- overzichtspagina's: elke cel (64x64) is een tegel: een track (MUTE-pagina) of een mixerkanaal (MIXER-pagina)
typedef struct {
  char label[6];     // "G1T1" / "CH01"
  u8 flags;          // bit0 geselecteerd, bit1 gemute, bit2 solo
  u8 len;            // tracklengte in stappen (1..256)
  u8 pos;            // absolute afspeelstap, 0xff = stil
  u8 win;            // 16-stappenvenster dat getoond wordt (0 = stap 1-16)
  u16 gates;         // 16 bits: gate per stap in het venster
  u8 v1, v2, v3;     // (oud) mixer: volume, pan, programma
  char txt[6];       // mixer: waarde van de gekozen mixerpagina als tekst ("USB1", "100", "L24")
  char pname[8];     // mixer: naam van de pagina ("VOLUME")
  u8 vmax;           // mixer: maximale waarde voor de balk (0 = geen balk)
  u8 pan;            // mixer: 1 = pan-schuif met middenstreep
  char port[6];      // mixer: poortnaam ("USB1") voor het kleine vakje
  u8 chn;            // mixer: MIDI-kanaal 0-15 voor het kleine vakje
} og2_tile_t;
// ---- Event-pagina (menu -> event), eigen indeling over de 4 schermen
typedef struct {
  char track[6];        // "G1T1"
  u8 mode;              // 0 Note, 1 Chord, 2 CC, 3 Drum
  u8 size_idx, size_n;  // positie binnen de lagen-configuraties van deze modus (0-based), aantal
  u16 steps;            // aantal stappen per parameterlaag
  u8 par_layers, trg_layers, instruments;
  char port[6];
  u8 chn;               // 0-15
  u8 layer_sel;         // gekozen parameterlaag 0-15
  char ltype[6];        // type van de gekozen laag (of het nog niet bevestigde nieuwe type)
  u8 ltype_pending;     // 1 = nieuw type nog niet bevestigd (knop indrukken)
  char param[6];        // CC-nummer ("#074") of "---"
  u8 init_pct;          // 0-100: voortgang van het vasthouden van INIT (2 s)
  u8 init_needed;       // 1 = configuratie gewijzigd: INIT nodig
  u8 sel;               // last touched column 0-15
  // drum mode
  u8 drum_inst;         // selected instrument (0-based)
  u8 drum_note, vel_n, vel_a;   // MIDI note, normal and accented velocity of that instrument
  char lay_a[6], lay_b[6];      // types of the two drum parameter layers
  u8 trg_steps_drum;    // (unused, steps holds the trigger steps in drum mode)
} og2_event_t;
extern void OG2_RenderEventCell(u8 *out2048, int cell, const og2_event_t *e);

// ---- Timing pages (menu -> Divider / Length / Direction): one struct, each menu has its own layout (page)
typedef struct {
  char track[6];        // "G1T1"
  u16 length, loop;     // 1-based
  u16 num_steps;        // steps the track is prepared for
  u16 presets[8];       // quick selection lengths (1-based)
  u8 len_preset;        // index of the matching preset, 0xff = none
  u16 divider;          // clock divider (1-256)
  u8 triplet, sync, manual;
  u8 tb_preset;         // index of the matching timebase preset (1,2,4,8,8T,16,16T,32), 0xff = none
  u8 dir;               // 0..6 Forward/Backward/PingPong/Pendulum/Rand.Dir/Rand.Step/Rand.D+S
  u8 fwd, back;         // steps forward (1-8), jump back (0-7)
  u8 replay;            // 1-8
  u8 repeat, skip;      // 0-16
  u8 itv;               // repeat/skip interval 1-64
  u8 sel;               // last touched column 0-15
  u8 page;              // 0 Divider, 1 Length, 2 Direction (each menu has its own page)
  u8 qsel_loops;        // Length: 1 = presets set length + loop (loops mode)
  u16 loop_presets[8];  // Length, loops mode: loop start of each preset (1-based); presets[] then holds the lengths
} og2_timing_t;
extern void OG2_RenderTimingCell(u8 *out2048, int cell, const og2_timing_t *t);

// ---- Mode page (menu -> Mode): track mode, bus, note stack options
typedef struct {
  char track[6];        // "G1T1"
  u8 mode;              // 0 off, 1 normal, 2 transpose, 3 arpeggiator
  u8 bus;               // 0-3
  u8 first;             // 1 = first note, 0 = last note
  u8 hold, sort, restart, steptrg, fts, sustain;   // 1 = on (sort already de-inverted)
  u8 sel;               // last touched column 0-15
} og2_mode_t;
extern void OG2_RenderModeCell(u8 *out2048, int cell, const og2_mode_t *m);

// ---- generic column page: every menu page describes its 16 GP columns, the renderer applies the UI rules (DESIGN.md)
typedef enum {
  OG2_C_NONE = 0,   // nothing in this column
  OG2_C_VALUE,      // label + value
  OG2_C_TOGGLE,     // label + ON/OFF (on = filled)
  OG2_C_CHOICE,     // one option of a choice: label + value, on = active (filled)
  OG2_C_ACTION,     // label + framed button text (value), e.g. STORE, CLEAR
  OG2_C_TEXT,       // label + up to 3 lines of page.text, separated by '|'
  OG2_C_GRAPH,      // label + bar graph of page.graph[] (signed, centre line), value = text right in the label row
  OG2_C_METER,      // label + value + horizontal meter (meter = 0..100)
  OG2_C_KEYS,       // label + one-octave keyboard: keys = bit mask of lit keys (bit 0 = C), value = text right in the label row
  OG2_C_SPAN        // continuation of the box that starts in a column to the left
} og2_ckind_t;
typedef struct {
  u8 kind;          // og2_ckind_t
  u8 span;          // number of columns of the box (1-16), only in the first column
  u8 on;            // TOGGLE/CHOICE: active
  u8 dim;           // not applicable
  u8 meter;         // METER: 0..100
  u16 keys;         // KEYS: lit keys
  char label[14];
  char value[28];
} og2_col_t;
typedef struct {
  char head[4][24];   // header per screen ("" = only the dotted line)
  og2_col_t c[16];
  u16 sel;            // columns of the selected item (double frame)
  s8 graph[16];       // GRAPH data, -100..100
  u8 graph_n;         // number of graph bars
  s8 graph_mark;      // highlighted bar, -1 = none
  char text[112];      // TEXT box content (one TEXT box per page), lines separated by '|'
} og2_page_t;
extern void OG2_RenderPageCell(u8 *out2048, int cell, const og2_page_t *p);
extern void OG2_PageInit(og2_page_t *p);                                          // clears the page
extern og2_col_t *OG2_PageCol(og2_page_t *p, int k, u8 kind, int span, const char *label);   // sets up column k (and SPAN columns)

// ---- dashboard (globale pagina's: BPM, PATTERN, SONG, SAVE): 4 schermen als samengesteld beeld
typedef struct {
  u16 bpm10;          // BPM x 10
  u8 running, master;
  u16 bar;            // 1-gebaseerd
  u8 beat;            // 0..3
  u8 sixt;            // 0..15 (zestiende binnen de maat)
  char session[14];
  u8 pat_group[4], pat_num[4], pat_req[4];   // patroon A..H / 1..8, aanvraag voor wissel
  char pat_name[4][10];
  u8 sel_track;       // 0..15
  u8 muted_cnt;
} og2_dash_t;
// ---- tekst-weergave (alle overige pagina's): de LCD-tekst van de SEQ, 5 tekens per GP-kolom, 2x vergroot
typedef struct { char l1[6]; char l2[6]; } og2_text_t;       // (oud, per cel)
extern void OG2_RenderPlainCell(u8 *out2048, int cell, const og2_text_t *t);   // platte tekst, 2x, 5 tekens per GP-kolom
// gegroepeerde tekstweergave: woorden die over de 5-tekengrens van een GP-kolom lopen worden 1 groot vak
typedef struct {
  char label[28];     // regel 1 van het LCD (klein)
  char value[28];     // regel 2 van het LCD (groot)
  u8 start;           // eerste cel (GP-kolom) van de groep, 0-15
  u8 n;               // aantal cellen in de groep (0 = lege cel)
  u8 sel;             // 1 = op het LCD gekozen (tussen > en <)
} og2_tgroup_t;
extern void OG2_TextGroups(const u8 *row0, const u8 *row1, og2_tgroup_t g[16]);   // row0/row1: 80 LCD-tekens
extern void OG2_RenderTextGroupCell(u8 *out2048, int cell, const og2_tgroup_t *g);
extern void OG2_RenderDashCell(u8 *out2048, int cell, const og2_dash_t *d);
extern void OG2_RenderTrackTile(u8 *out2048, int num, const og2_tile_t *t);
extern void OG2_RenderMixerTile(u8 *out2048, int num, const og2_tile_t *t);

// play_step 0..15 of 0xff; trail_step = vorige stap voor de uitdovende staart (0xff = geen)
extern void OG2_RenderSteps(og2_frame_t *f, const og2_step_t st[16], u8 play_step, u8 trail_step);
// voorstel-layout: vierkant hoofdvak + 3 symboolvakjes eronder (accent / glide / roll-skip-kans)
extern void OG2_RenderCellBoxed(u8 *out2048, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer, og2_kind_t kind);
extern int og2_vel_style;
extern void OG2_StepRange(const og2_step_t st[16], int *lo, int *hi);
extern void OG2_RenderCell(u8 *out2048, int num, const og2_step_t *s, int lo, int hi, int playing, int trailing, const char *layer, og2_kind_t kind);
#endif
