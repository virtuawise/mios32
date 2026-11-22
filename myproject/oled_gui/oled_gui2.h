// Tekenlaag v2: 4 x SSD1322 (256x64), 4 bit grijs (0..15). Stap-weergave met meerdere lagen tegelijk
// (pianoroll-balk: toonhoogte, lengte, velocity=helderheid, probability=korrel, delay=verschuiving) + iconen + actieve laagwaarde.
#ifndef _OLED_GUI2_H
#define _OLED_GUI2_H
#include <mios32.h>
#define OG2_DISPLAYS 4
#define OG2_W 256
#define OG2_H 64
typedef struct {
  u8 note;      // 0..127, 0xff = geen
  u8 vel;       // 0..127
  u8 len;       // gatelengte 0..100 (% van de stap; >100 mag voor legato)
  u8 prob;      // 0..100
  s8 delay;     // -? .. +: verschuiving in % van de stap (0..99)
  u8 on, acc, sld, roll, skip;
  u8 value;     // waarde van de actieve laag (0..127) onderin
} og2_step_t;
typedef struct {
  u8 gray[OG2_DISPLAYS][OG2_H][OG2_W];   // 1 byte per pixel, 0..15 (prototype; verpakking naar 4 bit volgt)
  const char *layer_name;                // naam van de actieve laag, bv "VEL"
} og2_frame_t;
// play_step 0..15 of 0xff; trail_step = vorige stap voor de uitdovende staart (0xff = geen)
extern void OG2_RenderSteps(og2_frame_t *f, const og2_step_t st[16], u8 play_step, u8 trail_step);
#endif
