// $Id$
/*
 * Divider, Length and Direction pages for the 4 x OLED front panel (myproject, branch four-screens-usb)
 *
 * Each menu has its own page; the columns follow the GP layout of the original page (GP1 = track):
 *   DIVIDER:   TRACK | DIVIDER | TRIPLET | SYNC BAR | MANUAL | steps per bar (info) | 8 x TIMEBASE (GP9-16)
 *   LENGTH:    TRACK | LENGTH | LOOP | PRESETS mode | overview | 8 x PRESET (GP9-16)
 *   DIRECTION: TRACK | 7 x DIRECTION (GP2-8) | FORWARD | JMP BACK | REPLAY (2) | REPEAT | SKIP | INTERVAL | SYNC BAR
 * The LCD of each page keeps its original text; only encoders, GP buttons and GP LEDs are remapped.
 * Values are changed through SEQ_UI_CC_* (same limits as the original pages, applied to all selected tracks).
 */

#include <mios32.h>
#include <string.h>
#include <stdio.h>
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_trg.h"
#include "seq_core.h"
#include "seq_ui_timing.h"

static u8 tim_sel = 0;
static s32 (*button_fallback)(seq_ui_button_t button, s32 depressed);

static const u16 tb_preset[8] = { 255, 127, 63, 31, 31|0x100, 15, 15|0x100, 7 };   // 1,2,4,8,8T,16,16T,32 (bit 8 = triplet)

static s32 TbPresetFind(u8 track)
{
  u16 p = SEQ_CC_Get(track, SEQ_CC_CLK_DIVIDER) | ((SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS) & 2) << 7);
  int i;
  for(i=0; i<8; ++i)
    if( tb_preset[i] == p )
      return i;
  return -1;
}

static s32 LenPresetFind(u8 track)
{
  u8 len = SEQ_CC_Get(track, SEQ_CC_LENGTH);
  int i;
  for(i=0; i<UI_QUICKSEL_NUM_PRESETS; ++i)
    if( ui_quicksel_length[i] == len )
      return i;
  return -1;
}

static void SyncMeasureHint(u8 track)
{
  if( seq_cc_trk[track].clkdiv.SYNCH_TO_MEASURE && (int)SEQ_CC_Get(track, SEQ_CC_LENGTH) > (int)seq_core_steps_per_measure ) {
    char buffer[20];
    sprintf(buffer, "active for %d steps", (int)seq_core_steps_per_measure+1);
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Synch-to-Measure is", buffer);
  }
}

static s32 FlagSet(u8 mask, s32 incrementer)
{
  if( incrementer == 0 )   // toggle
    incrementer = (SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_CLKDIV_FLAGS) & mask) ? -1 : 1;
  return SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, mask, (incrementer > 0) ? mask : 0);
}

static u8 tim_page = 0;          // 0 Divider, 1 Length, 2 Direction
static u8 qsel_loops = 0;        // Length: presets in loops mode

static s32 Flag(u8 mask, s32 incrementer, u8 manual_blocks)
{
  if( manual_blocks && (SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_CLKDIV_FLAGS) & (1<<2)) ) return 0;
  return FlagSet(mask, incrementer);
}

static s32 SetTimebase(int i)
{
  SEQ_UI_CC_Set(SEQ_CC_CLK_DIVIDER, tb_preset[i] & 0xff);
  SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<1), (tb_preset[i] & 0x100) ? (1<<1) : 0);
  return 1;
}

static s32 SetLengthPreset(u8 track, int i)
{
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  int len = qsel_loops ? ui_quicksel_loop_length[i] : ui_quicksel_length[i];
  if( (len+1) > num_steps ) {
    char buffer[20];
    sprintf(buffer, "for %d steps", num_steps);
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Track only prepared", buffer);
    return 0;
  }
  SEQ_UI_CC_Set(SEQ_CC_LENGTH, len);
  if( qsel_loops ) SEQ_UI_CC_Set(SEQ_CC_LOOP, ui_quicksel_loop_loop[i]);
  SyncMeasureHint(track);
  return 1;
}

// column k (0-15) of the current page changed by incrementer (0 = button press)
static s32 ColumnInc(int k, s32 incrementer)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  u8 manual = SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS) & (1<<2);

  if( k == 0 )
    return SEQ_UI_GxTyInc(incrementer);

  if( tim_page == 0 ) {          // DIVIDER: 1 divider, 2 triplet, 3 sync bar, 4 manual, 5-7 info, 8-15 timebase presets
    switch( k ) {
    case 1: return manual ? 0 : SEQ_UI_CC_Inc(SEQ_CC_CLK_DIVIDER, 0, 255, incrementer);
    case 2: return Flag(1<<1, incrementer, 1);
    case 3: return Flag(1<<0, incrementer, 1);
    case 4: return Flag(1<<2, incrementer, 0);
    case 5: case 6: case 7: return 0;
    default: return manual ? 0 : SetTimebase(k - 8);
    }
  }

  if( tim_page == 1 ) {          // LENGTH: 1 length, 2 loop, 3 presets mode, 4-7 overview, 8-15 presets
    switch( k ) {
    case 1:
      if( SEQ_UI_CC_Inc(SEQ_CC_LENGTH, 0, num_steps-1, incrementer) >= 1 ) { SyncMeasureHint(track); return 1; }
      return 0;
    case 2: return SEQ_UI_CC_Inc(SEQ_CC_LOOP, 0, num_steps-1, incrementer);
    case 3:
      if( incrementer == 0 ) incrementer = qsel_loops ? -1 : 1;
      return SEQ_UI_Var8_Inc(&qsel_loops, 0, 1, incrementer);
    case 4: case 5: case 6: case 7: return 0;
    default: return SetLengthPreset(track, k - 8);
    }
  }

  // DIRECTION: 1-7 directions, 8 forward, 9 jump back, 10-11 replay, 12 repeat, 13 skip, 14 interval, 15 sync bar
  switch( k ) {
  case 1: case 2: case 3: case 4: case 5: case 6: case 7:
    return SEQ_UI_CC_Set(SEQ_CC_DIRECTION, k - 1);
  case 8:  return SEQ_UI_CC_Inc(SEQ_CC_STEPS_FORWARD, 0, 7, incrementer);
  case 9:  return SEQ_UI_CC_Inc(SEQ_CC_STEPS_JMPBCK, 0, 7, incrementer);
  case 10: case 11: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPLAY, 0, 7, incrementer);
  case 12: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPEAT, 0, 16, incrementer);
  case 13: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_SKIP, 0, 16, incrementer);
  case 14: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_RS_INTERVAL, 0, 63, incrementer);
  case 15: return FlagSet(1<<0, incrementer);
  }
  return -1;
}

static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    tim_sel = (u8)encoder;
    return ColumnInc((int)encoder, incrementer);
  }
  if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    if( incrementer == 0 ) return 0;
    if( tim_page == 2 && tim_sel >= 1 && tim_sel <= 7 ) {          // walk through the directions
      int n = (int)tim_sel + (incrementer > 0 ? 1 : -1);
      if( n < 1 || n > 7 ) return 0;
      tim_sel = n;
      return ColumnInc(n, 0);
    }
    if( tim_page != 2 && tim_sel >= 8 ) {                          // walk through timebase / length presets
      int n = (int)tim_sel + (incrementer > 0 ? 1 : -1);
      if( n < 8 || n > 15 ) return 0;
      tim_sel = n;
      return ColumnInc(n, 0);
    }
    return ColumnInc(tim_sel, incrementer);
  }
  return -1;
}

static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( button <= SEQ_UI_BUTTON_GP16 ) {
    if( depressed ) return 0;
    tim_sel = (u8)button;
    return ColumnInc((int)button, 0);
  }
  if( !depressed ) {
    if( button == SEQ_UI_BUTTON_Up )   return ColumnInc(tim_sel, 1);
    if( button == SEQ_UI_BUTTON_Down ) return ColumnInc(tim_sel, -1);
  }
  return button_fallback ? button_fallback(button, depressed) : -1;
}

// LEDs: menu pages only light their menu button (done centrally in seq_ui.c)
static s32 LED_Handler(u16 *gp_leds)
{
  *gp_leds = 0;
  return 0;
}

void SEQ_UI_TIMING_Install(u8 page, s32 (*fallback)(seq_ui_button_t button, s32 depressed))
{
  u8 track = SEQ_UI_VisibleTrackGet();
  tim_page = page;
  button_fallback = fallback;
  // selection starts on the active choice (direction / timebase) or on the main value
  if( page == 2 ) tim_sel = 1 + (SEQ_CC_Get(track, SEQ_CC_DIRECTION) % 7);
  else tim_sel = 1;
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
}

s32 SEQ_UI_TIMING_OledGet(og2_timing_t *t)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u8 flags = SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS);
  s32 i;

  memset(t, 0, sizeof(*t));
  sprintf(t->track, "G%dT%d", (track / 4) + 1, (track % 4) + 1);
  t->length = SEQ_CC_Get(track, SEQ_CC_LENGTH) + 1;
  t->loop = SEQ_CC_Get(track, SEQ_CC_LOOP) + 1;
  t->num_steps = SEQ_TRG_NumStepsGet(track);
  for(i=0; i<8; ++i)
    t->presets[i] = ui_quicksel_length[i] + 1;
  i = LenPresetFind(track);
  t->len_preset = (i < 0) ? 0xff : (u8)i;
  t->divider = SEQ_CC_Get(track, SEQ_CC_CLK_DIVIDER) + 1;
  t->triplet = (flags >> 1) & 1;
  t->sync = flags & 1;
  t->manual = (flags >> 2) & 1;
  i = TbPresetFind(track);
  t->tb_preset = (i < 0) ? 0xff : (u8)i;
  t->dir = SEQ_CC_Get(track, SEQ_CC_DIRECTION);
  t->fwd = SEQ_CC_Get(track, SEQ_CC_STEPS_FORWARD) + 1;
  t->back = SEQ_CC_Get(track, SEQ_CC_STEPS_JMPBCK);
  t->replay = SEQ_CC_Get(track, SEQ_CC_STEPS_REPLAY) + 1;
  t->repeat = SEQ_CC_Get(track, SEQ_CC_STEPS_REPEAT);
  t->skip = SEQ_CC_Get(track, SEQ_CC_STEPS_SKIP);
  t->itv = SEQ_CC_Get(track, SEQ_CC_STEPS_RS_INTERVAL) + 1;
  t->sel = tim_sel;
  t->page = tim_page;
  t->qsel_loops = qsel_loops;
  if( qsel_loops ) {           // loops mode: presets set length and loop start
    t->len_preset = 0xff;
    for(i=0; i<8; ++i) {
      t->presets[i] = ui_quicksel_loop_length[i] + 1;
      t->loop_presets[i] = ui_quicksel_loop_loop[i] + 1;
      if( ui_quicksel_loop_length[i] == SEQ_CC_Get(track, SEQ_CC_LENGTH) && ui_quicksel_loop_loop[i] == SEQ_CC_Get(track, SEQ_CC_LOOP) )
        t->len_preset = i;
    }
  }
  return 0;
}
