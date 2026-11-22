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
} og2_tile_t;
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
typedef struct { char l1[6]; char l2[6]; } og2_text_t;
extern void OG2_RenderTextCell(u8 *out2048, int cell, const og2_text_t *t);
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
