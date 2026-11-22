// Divider, Length and Direction pages for the 4 x OLED front panel: encoder/button mapping and OLED data
#ifndef _SEQ_UI_TIMING_H
#define _SEQ_UI_TIMING_H

#include "seq_ui.h"
#include "oled_gui2.h"

// installs the Timing mapping; buttons other than GP1-16/Up/Down go to <fallback> (the page's own button handler)
extern void SEQ_UI_TIMING_Install(u8 page, s32 (*fallback)(seq_ui_button_t button, s32 depressed));   // page: 0 Divider, 1 Length, 2 Direction
extern s32 SEQ_UI_TIMING_OledGet(og2_timing_t *t);

#endif
