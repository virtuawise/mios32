// $Id$
/*
 * Timing page for the 4 x OLED front panel (myproject, branch four-screens-usb)
 *
 * The Divider, Length and Direction pages share one layout of 16 columns:
 *   0 LENGTH | 1 LOOP | 2,3 PRESETS (length)
 *   4 TIMEBASE preset | 5 DIVIDER (raw, button: triplet) | 6 SYNC BAR | 7 MANUAL
 *   8,9 DIRECTION | 10 FORWARD | 11 JUMP BACK
 *   12 REPLAY | 13 REPEAT | 14 SKIP | 15 INTERVAL
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

// column k (0-15) changed by incrementer (0 = button press)
static s32 ColumnInc(int k, s32 incrementer)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  u8 manual = SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS) & (1<<2);

  switch( k ) {
  case 0:
    if( SEQ_UI_CC_Inc(SEQ_CC_LENGTH, 0, num_steps-1, incrementer) >= 1 ) { SyncMeasureHint(track); return 1; }
    return 0;
  case 1:
    return SEQ_UI_CC_Inc(SEQ_CC_LOOP, 0, num_steps-1, incrementer);
  case 2: case 3: {
    if( incrementer == 0 ) return 0;
    int i = LenPresetFind(track);
    if( i < 0 ) {     // not on a preset: go to the nearest one in the turning direction
      u8 len = SEQ_CC_Get(track, SEQ_CC_LENGTH);
      for(i = (incrementer > 0) ? 0 : UI_QUICKSEL_NUM_PRESETS-1; i >= 0 && i < UI_QUICKSEL_NUM_PRESETS; i += (incrementer > 0) ? 1 : -1)
	if( (incrementer > 0) ? (ui_quicksel_length[i] > len) : (ui_quicksel_length[i] < len) )
	  break;
    } else
      i += (incrementer > 0) ? 1 : -1;
    if( i < 0 || i >= UI_QUICKSEL_NUM_PRESETS || (ui_quicksel_length[i]+1) > num_steps )
      return 0;
    SEQ_UI_CC_Set(SEQ_CC_LENGTH, ui_quicksel_length[i]);
    SyncMeasureHint(track);
    return 1;
  }
  case 4: {
    if( manual || incrementer == 0 ) return 0;
    int i = TbPresetFind(track);
    if( i < 0 ) {     // nearest preset in the turning direction (by divider value; larger divider = slower = lower index)
      u8 div = SEQ_CC_Get(track, SEQ_CC_CLK_DIVIDER);
      for(i = (incrementer > 0) ? 0 : 7; i >= 0 && i < 8; i += (incrementer > 0) ? 1 : -1)
	if( (incrementer > 0) ? ((tb_preset[i] & 0xff) < div) : ((tb_preset[i] & 0xff) > div) )
	  break;
    } else
      i += (incrementer > 0) ? 1 : -1;
    if( i < 0 || i >= 8 ) return 0;
    SEQ_UI_CC_Set(SEQ_CC_CLK_DIVIDER, tb_preset[i] & 0xff);
    SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<1), (tb_preset[i] & 0x100) ? (1<<1) : 0);
    return 1;
  }
  case 5:
    if( manual ) return 0;
    if( incrementer == 0 ) return FlagSet(1<<1, 0);      // button: triplet
    return SEQ_UI_CC_Inc(SEQ_CC_CLK_DIVIDER, 0, 255, incrementer);
  case 6:
    if( manual ) return 0;
    return FlagSet(1<<0, incrementer);
  case 7:
    return FlagSet(1<<2, incrementer);
  case 8: case 9:
    if( incrementer == 0 ) {                             // button: next direction (wraps)
      u8 d = SEQ_CC_Get(track, SEQ_CC_DIRECTION);
      return SEQ_UI_CC_Set(SEQ_CC_DIRECTION, (d >= 6) ? 0 : d+1);
    }
    return SEQ_UI_CC_Inc(SEQ_CC_DIRECTION, 0, 6, incrementer);
  case 10: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_FORWARD, 0, 7, incrementer);
  case 11: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_JMPBCK, 0, 7, incrementer);
  case 12: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPLAY, 0, 7, incrementer);
  case 13: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPEAT, 0, 16, incrementer);
  case 14: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_SKIP, 0, 16, incrementer);
  case 15: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_RS_INTERVAL, 0, 63, incrementer);
  }
  return -1;
}

static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    tim_sel = (u8)encoder;
    return ColumnInc((int)encoder, incrementer);
  }
  if( encoder == SEQ_UI_ENCODER_Datawheel )
    return ColumnInc(tim_sel, incrementer);
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

// LEDs: the selected column (both columns of a 2-column box) and the switches that are on
static s32 LED_Handler(u16 *gp_leds)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u8 flags = SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS);
  u16 n = 1 << tim_sel;
  if( tim_sel == 2 || tim_sel == 3 ) n = 0x000c;
  if( tim_sel == 8 || tim_sel == 9 ) n = 0x0300;
  if( (flags & (1<<0)) && !(flags & (1<<2)) ) n |= 1 << 6;
  if( flags & (1<<2) ) n |= 1 << 7;
  *gp_leds = n;
  return 0;
}

void SEQ_UI_TIMING_Install(s32 (*fallback)(seq_ui_button_t button, s32 depressed))
{
  button_fallback = fallback;
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
  return 0;
}
