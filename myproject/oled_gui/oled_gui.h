// Tekenlaag voor 4 OLED-schermen (SSD1322, elk 256x64, 1 bit) - stap-weergave voor MIDIbox SEQ V4+
// Platformonafhankelijk (alleen <mios32.h>-types + glcd_font); op de Mac testbaar met host_test.c
#ifndef _OLED_GUI_H
#define _OLED_GUI_H
#include <mios32.h>

#define OLEDGUI_NUM_DISPLAYS 4
#define OLEDGUI_W 256
#define OLEDGUI_H 64
#define OLEDGUI_ROW_BYTES (OLEDGUI_W/8)                 // 1 bit per pixel, bit 0 = links
#define OLEDGUI_DISPLAY_BYTES (OLEDGUI_ROW_BYTES*OLEDGUI_H)   // 2048

typedef struct {
  u8 note;        // MIDI-noot 0..127, 0xff = geen
  u8 value;       // 0..127 (velocity/parameter), getoond in de ring
  u8 on;          // gate aan (getriggerd)
  u8 acc;         // ACC-label
  u8 sld;         // SLD-label (glide)
} oledgui_step_t;

typedef struct {
  u8 fb[OLEDGUI_NUM_DISPLAYS][OLEDGUI_DISPLAY_BYTES];
} oledgui_frame_t;

// 16 stappen over 4 schermen (4 per scherm); play_step 0..15 = looplicht, 0xff = geen
extern void OLEDGUI_RenderSteps(oledgui_frame_t *f, const oledgui_step_t steps[16], u8 play_step);
extern void OLEDGUI_NoteName(u8 note, char *out);      // "C-1".."G#9": 3 tekens + 0
#endif
