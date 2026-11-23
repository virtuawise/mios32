// $Id$
/*
 * Track length page
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "seq_lcd.h"
#include "seq_ui.h"
#include <string.h>
#include <stdio.h>
#include "oled_gui2.h"
#include "seq_cc.h"
#include "seq_scale.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4
#define ITEM_SCALE         0
#define ITEM_OCTAVE        1
#define ITEM_SEMITONE      2
#define ITEM_GXTY          3


/////////////////////////////////////////////////////////////////////////////
// Local function declaration
/////////////////////////////////////////////////////////////////////////////
static s32 UpdateScaledSemitoneTab(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 scaled_semitone_tab[8];


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  switch( ui_selected_item ) {
  case ITEM_GXTY: *gp_leds = 0x0001; break;
  case ITEM_SCALE: {
    *gp_leds = 0;

    int oct = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT);
    if( oct >= 8 )
      oct -= 16;
    if( oct >= -3 && oct <= 3 )
      *gp_leds |= (1 << (oct + 4));

    int i;
    for(i=0; i<8; ++i) {
      u8 value = scaled_semitone_tab[i];
      if( value != 0xff && (value & 0x80) )
	*gp_leds |= (1 << (i+8));
    }
  } break;
  case ITEM_OCTAVE: *gp_leds = (1 << ((SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT)-8)&0xf)); break;
  case ITEM_SEMITONE: *gp_leds = (1 << ((SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI)-8)&0xf)); break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( encoder == SEQ_UI_ENCODER_GP1 )
    ui_selected_item = ITEM_GXTY;
  else if( encoder >= SEQ_UI_ENCODER_GP2 && encoder <= SEQ_UI_ENCODER_GP16 ) {
    if( ui_selected_item == ITEM_GXTY )
      ui_selected_item = ITEM_SCALE;

    switch( ui_selected_item ) {
    case ITEM_OCTAVE:   return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_OCT, (encoder-8) & 0xf);
    case ITEM_SEMITONE: return SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_SEMI, (encoder-8) & 0xf);
    case ITEM_SCALE: {
      if( encoder < 8 ) {
	SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_OCT, (encoder-4) & 0xf);
      } else {
	int oct = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT);
	if( oct >= 8 )
	  oct -= 16;

	int semi = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI);
	if( semi >= 8 )
	  semi -= 16;

	int i = encoder - 8;
	int value = scaled_semitone_tab[i];
	if( value != 0xff ) {
	  value &= 0x7f;
	  // since value ranges from -8..+7, we've to wrap at 8
	  if( value >= 8 ) {
	    value -= 12;

	    // octave transition?
	    if( semi >= 0 )
	      oct += 1;
	  } else {
	    // octave transition?
	    if( semi < 0 )
	      oct -= 1;
	  }
	  SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_SEMI, value & 0xf);
	  SEQ_UI_CC_Set(SEQ_CC_TRANSPOSE_OCT, oct & 0xf);
	}
      }
      return 1;
    } break;
    }
  }

  // for GXTY via GP encoder + Datawheel
  u8 change_cc = 0;
  switch( ui_selected_item ) {
  case ITEM_GXTY:     return SEQ_UI_GxTyInc(incrementer);
  case ITEM_OCTAVE:   change_cc = SEQ_CC_TRANSPOSE_OCT; break;
  case ITEM_SEMITONE: change_cc = SEQ_CC_TRANSPOSE_SEMI; break;
  case ITEM_SCALE:    change_cc = SEQ_CC_TRANSPOSE_OCT; break; // cheap...
  }

  if( change_cc ) {
    u8 value = (SEQ_CC_Get(visible_track, change_cc) + 7) & 0xf;
    if( SEQ_UI_Var8_Inc(&value, 0, 14, incrementer) ) {
      SEQ_UI_CC_Set(change_cc, (value - 7) & 0xf);
      return 1;
    }
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

  if( button >= SEQ_UI_BUTTON_GP2 && button <= SEQ_UI_BUTTON_GP16 ) {
    // -> forward to encoder handler
    return Encoder_Handler((int)button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Transpose: >Scale<>Oct:+0<>Semi:+0<                                           
  // GxTy  -7   -6   -5   -4   -3   -2   -1  >+0<  +1   +2   +3   +4   +5   +6   +7

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Transpose: ");

  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SCALE ? '>' : ' ');
  if( ui_selected_item == ITEM_SCALE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintString("Scale");
  }
  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SCALE ? '<' : ' ');

  SEQ_LCD_PrintChar(ui_selected_item == ITEM_OCTAVE ? '>' : ' ');
  if( ui_selected_item == ITEM_OCTAVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT);
    SEQ_LCD_PrintFormattedString("Oct:%c%d", (value < 8) ? '+' : '-', (value < 8) ? value : (16-value));
  }
  SEQ_LCD_PrintChar(ui_selected_item == ITEM_OCTAVE ? '<' : ' ');

  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SEMITONE ? '>' : ' ');
  if( ui_selected_item == ITEM_SEMITONE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(7);
  } else {
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI);
    SEQ_LCD_PrintFormattedString("Semi:%c%d", (value < 8) ? '+' : '-', (value < 8) ? value : (16-value));
  }
  SEQ_LCD_PrintChar(ui_selected_item == ITEM_SEMITONE ? '<' : ' ');

  SEQ_LCD_PrintSpaces(40); // second half


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_OCTAVE || ui_selected_item == ITEM_SEMITONE ) {
    u8 value = SEQ_CC_Get(visible_track, ui_selected_item == ITEM_SEMITONE ? SEQ_CC_TRANSPOSE_SEMI : SEQ_CC_TRANSPOSE_OCT);

    int i;
    for(i=1; i<8; ++i) {
      SEQ_LCD_PrintChar(value == (8+i) ? '>' : ' ');

      if( value == (8+i) && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(2);
      else
	SEQ_LCD_PrintFormattedString("-%d", 8-i);

      SEQ_LCD_PrintChar(value == (8+i) ? '<' : ' ');
      SEQ_LCD_PrintChar(' ');
    }

    for(i=8; i<16; ++i) {
      SEQ_LCD_PrintChar(value == (i-8) ? '>' : ' ');

      if( value == (i-8)  && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(2);
      else
	SEQ_LCD_PrintFormattedString("+%d", i-8);

      SEQ_LCD_PrintChar(value == (i-8) ? '<' : ' ');
      SEQ_LCD_PrintChar(' ');
    }
  } else {
    const char keys_str[12][3] = {
      "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "
    };

    int i;

    int oct = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_OCT);
    if( oct >= 8 )
      oct -= 16;
    for(i=-3; i<=3; ++i) {
      SEQ_LCD_PrintChar((i == oct) ? '>' : ' ');

      if( ui_selected_item != ITEM_GXTY && (i == oct) && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(2);
      else {
	if( i < 0 )
	  SEQ_LCD_PrintFormattedString("%d", i);
	else
	  SEQ_LCD_PrintFormattedString("+%d", i);
      }

      SEQ_LCD_PrintChar((i == oct) ? '<' : ' ');
      SEQ_LCD_PrintChar(' ');
    }

    // update semitone table
    // note: also used for LED display and value selection
    UpdateScaledSemitoneTab();

    for(i=0; i<8; ++i) {
      u8 value = scaled_semitone_tab[i];
      if( value == 0xff ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 semitone = value & 0x7f;
	u8 semitone_active = value & 0x80;

	SEQ_LCD_PrintChar(semitone_active ? '>' : ' ');
	SEQ_LCD_PrintString(keys_str[semitone]);
	SEQ_LCD_PrintChar(semitone_active ? '<' : ' ');
	SEQ_LCD_PrintChar(' ');
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// 4 x OLED (myproject): own layout and mapping (the original page switches GP2-16 between scale/octave/semitone,
// which is unclear on boxes). Columns:
//   0 TRACK | 1 OCTAVE | 2 SEMITONE | 3 TOTAL (info) | 4-7 keyboard (info) | 8-15 SCALE steps (press/turn = choose)
/////////////////////////////////////////////////////////////////////////////
static u8 tran_sel = 1;
static void tran_sgn(char *o, int v, const char *suffix){ sprintf(o, "%c%d%s", v < 0 ? '-' : '+', v < 0 ? -v : v, suffix); }   // MIOS32 sprintf has no %+d
static const char *tran_note[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

static s32 Encoder_HandlerMap(seq_ui_encoder_t encoder, s32 incrementer)
{
  int k = (encoder <= SEQ_UI_ENCODER_GP16) ? (int)encoder : -1;
  if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    k = tran_sel;
    if( k >= 8 ) {                     // walk through the scale steps
      int n = k + (incrementer > 0 ? 1 : -1);
      if( incrementer == 0 || n < 8 || n > 15 ) return 0;
      k = n;
      incrementer = 0;
    }
  }
  if( k < 0 ) return Encoder_Handler(encoder, incrementer);
  tran_sel = k;
  switch( k ) {
  case 0: ui_selected_item = ITEM_GXTY; return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, incrementer);
  case 1: ui_selected_item = ITEM_OCTAVE; return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, incrementer);
  case 2: ui_selected_item = ITEM_SEMITONE; return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, incrementer);
  case 3: case 4: case 5: case 6: case 7: return 0;
  }
  UpdateScaledSemitoneTab();
  ui_selected_item = ITEM_SCALE;
  return Encoder_Handler((seq_ui_encoder_t)k, incrementer);    // GP9-16 in scale mode = scale steps
}

static s32 Button_HandlerMap(seq_ui_button_t button, s32 depressed)
{
  if( button <= SEQ_UI_BUTTON_GP16 ) {
    if( depressed ) return 0;
    int k = (int)button;
    if( k >= 8 ) return Encoder_HandlerMap((seq_ui_encoder_t)k, 0);
    if( k <= 2 ) tran_sel = k;
    return 0;
  }
  if( !depressed && (button == SEQ_UI_BUTTON_Up || button == SEQ_UI_BUTTON_Down) )
    return Encoder_HandlerMap(SEQ_UI_ENCODER_Datawheel, (button == SEQ_UI_BUTTON_Up) ? 1 : -1);
  return Button_Handler(button, depressed);
}

s32 SEQ_UI_TRKTRAN_OledGet(og2_page_t *p)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  int oct = SEQ_CC_Get(track, SEQ_CC_TRANSPOSE_OCT); if( oct >= 8 ) oct -= 16;
  int semi = SEQ_CC_Get(track, SEQ_CC_TRANSPOSE_SEMI); if( semi >= 8 ) semi -= 16;
  int total = oct * 12 + semi, key = ((semi % 12) + 12) % 12, i;
  og2_col_t *c;
  UpdateScaledSemitoneTab();
  OG2_PageInit(p);
  sprintf(p->head[0], "TRANSPOSE G%dT%d", (track/4)+1, (track%4)+1);
  strcpy(p->head[1], "RESULT");
  strcpy(p->head[2], "SCALE STEPS (FROM C)");
  c = OG2_PageCol(p, 0, OG2_C_VALUE, 1, "TRACK"); sprintf(c->value, "G%dT%d", (track/4)+1, (track%4)+1);
  c = OG2_PageCol(p, 1, OG2_C_VALUE, 1, "OCTAVE"); tran_sgn(c->value, oct, "");
  c = OG2_PageCol(p, 2, OG2_C_VALUE, 1, "SEMITONE"); tran_sgn(c->value, semi, "");
  c = OG2_PageCol(p, 3, OG2_C_VALUE, 1, "TOTAL"); tran_sgn(c->value, total, " ST"); c->dim = (total == 0);
  c = OG2_PageCol(p, 4, OG2_C_KEYS, 4, "C BECOMES");
  c->keys = 1 << key;
  c->meter = 0;              // C is where the transposition starts
  { int okt = (total - key) / 12; if( okt ) { char o[8]; tran_sgn(o, okt, " OCT"); sprintf(c->value, "%s, %s", tran_note[key], o); } else strcpy(c->value, tran_note[key]); }
  for(i=0; i<8; ++i) {
    u8 v = scaled_semitone_tab[i];
    c = OG2_PageCol(p, 8+i, OG2_C_CHOICE, 1, "STEP");
    if( v == 0xff ) { strcpy(c->value, "--"); c->dim = 1; }
    else { strcpy(c->value, tran_note[v & 0x0f]); c->on = (v & 0x80) ? 1 : 0; }
  }
  p->sel = 1 << tran_sel;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKTRAN_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_HandlerMap);
  SEQ_UI_InstallEncoderCallback(Encoder_HandlerMap);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This table contains up to 8 scaled semitones.
// MSB is set if the semitone is actively selected
// Value is 0xff if table item is out-of-scale (not selectable)
/////////////////////////////////////////////////////////////////////////////
static s32 UpdateScaledSemitoneTab(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  int semi = SEQ_CC_Get(visible_track, SEQ_CC_TRANSPOSE_SEMI);
  if( semi >= 8 )
    semi -= 16;

  // normalize negative semitones (from -8..-1)
  if( semi < 0 )
    semi = 12 + semi;

  // range: 0..11
  semi %= 12;

  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(visible_track, ui_selected_step, ui_selected_instrument, &seq_cc_trk[visible_track], &scale, &root_selection, &root);
  if( root_selection == 0 )
    root = 0; // force root to C (don't use KEYB based root)
  u8 note = 0x3c; // C-3

  int i;
  for(i=0; i<8; ++i) {
    if( note >= (0x3c + 12) ) {
      scaled_semitone_tab[i] = 0xff;
    } else {
      u8 value = note % 12;
      if( value == semi ) {
	value |= 0x80; // set MSB: semitone actively selected
      }
      scaled_semitone_tab[i] = value;
    }
    note = SEQ_SCALE_NextNoteInScale(note, scale, root);
  }

  return 0; // no error
}

