// Timing page for the 4 x OLED front panel: shared encoder/button/LED mapping of the Divider, Length and Direction pages
#ifndef _SEQ_UI_TIMING_H
#define _SEQ_UI_TIMING_H

#include "seq_ui.h"
#include "oled_gui2.h"

// installs the Timing mapping; buttons other than GP1-16/Up/Down go to <fallback> (the page's own button handler)
extern void SEQ_UI_TIMING_Install(s32 (*fallback)(seq_ui_button_t button, s32 depressed));
extern s32 SEQ_UI_TIMING_OledGet(og2_timing_t *t);

#endif
