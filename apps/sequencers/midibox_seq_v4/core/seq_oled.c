// $Id$
/*
 * 4x OLED (SSD1322, 256x64) stap-weergave, zie seq_oled.h
 * Tekenen gebeurt in myproject/oled_gui (platformonafhankelijk, op de Mac getest).
 */
#include <mios32.h>
#include <string.h>
#include "tasks.h"

#include "seq_oled.h"
#include "seq_ui.h"
#include "seq_core.h"
#include "seq_bpm.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_layer.h"
#include "seq_midi_sysex.h"

#include "oled_gui.h"

#define OLED_PERIOD_MS      20     // beeld opbouwen/vergelijken: max 50x per seconde
#define OLED_RESEND_MS    3000     // alles opnieuw sturen (client kan later binnenkomen)
#define OLED_SEND_TICK_MS    4     // hooguit 1 cel per 4 ms versturen (USB MIDI is langzaam)
#define OLED_NUM_CELLS      (OLEDGUI_NUM_DISPLAYS*4)

static oledgui_frame_t frame;
static u8 sent_frame[OLEDGUI_NUM_DISPLAYS][OLEDGUI_DISPLAY_BYTES];
static u32 ctr = 0;
static u32 resend_ctr = 0;
static u8 next_cell = 0;
static u8 dirty[OLED_NUM_CELLS];
static u8 cell_buf[64*8];

// cel c: scherm c/4, kolom c%4; 64 rijen van 8 bytes
static void cell_extract(const u8 *fb, u8 col, u8 *out)
{
  int y;
  for(y=0; y<OLEDGUI_H; ++y)
    memcpy(&out[y*8], &fb[y*OLEDGUI_ROW_BYTES + col*8], 8);
}

s32 SEQ_OLED_Init(u32 mode)
{
  int c;
  for(c=0; c<OLED_NUM_CELLS; ++c)
    dirty[c] = 1;
  return 0;
}

// verzamelt de 16 zichtbare stappen van de geselecteerde track
static void collect_steps(oledgui_step_t steps[16], u8 *play_step)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u8 instrument = ui_selected_instrument;
  u16 base = (u16)ui_selected_step_view * 16;
  int i;

  for(i=0; i<16; ++i) {
    u16 step = base + i;
    seq_layer_evnt_t e;
    steps[i].on = SEQ_TRG_GateGet(track, step, instrument) ? 1 : 0;
    steps[i].note = 0xff;
    steps[i].value = 0;
    steps[i].acc = 0;
    steps[i].sld = 0;
    if( steps[i].on && SEQ_LAYER_GetEvntOfLayer(track, step, 0, instrument, &e) >= 0 ) {
      steps[i].note = e.midi_package.note;
      steps[i].value = e.midi_package.velocity & 0x7f;
    }
  }

  *play_step = 0xff;
  if( SEQ_BPM_IsRunning() ) {
    u8 cur = seq_core_trk[track].step;
    if( (cur / 16) == ui_selected_step_view )
      *play_step = cur % 16;
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

  // elke OLED_PERIOD_MS: beeld opbouwen en vergelijken; per aanroep hooguit 1 scherm versturen
  if( (ctr % OLED_PERIOD_MS) == 0 ) {
    oledgui_step_t steps[16];
    u8 play_step;
    int d;

    collect_steps(steps, &play_step);
    OLEDGUI_RenderSteps(&frame, steps, play_step);

    if( resend_ctr >= OLED_RESEND_MS ) {
      resend_ctr = 0;
      for(d=0; d<OLED_NUM_CELLS; ++d)
	dirty[d] = 1;
    }
    for(d=0; d<OLEDGUI_NUM_DISPLAYS; ++d) {
      int col, y;
      for(col=0; col<4; ++col)
	for(y=0; y<OLEDGUI_H; ++y)
	  if( memcmp(&frame.fb[d][y*OLEDGUI_ROW_BYTES + col*8], &sent_frame[d][y*OLEDGUI_ROW_BYTES + col*8], 8) != 0 ) {
	    dirty[d*4 + col] = 1;
	    break;
	  }
    }
  }

  // verstuur hooguit 1 gewijzigde cel per OLED_SEND_TICK_MS
  if( (ctr % OLED_SEND_TICK_MS) == 1 ) {
    int tries;
    for(tries=0; tries<OLED_NUM_CELLS; ++tries) {
      u8 c = next_cell;
      next_cell = (next_cell + 1) % OLED_NUM_CELLS;
      if( dirty[c] ) {
	u8 d = c / 4, col = c % 4;
	cell_extract(frame.fb[d], col, cell_buf);
	if( SEQ_MIDI_SYSEX_REMOTE_Server_SendOLEDCell(d, col, cell_buf, sizeof(cell_buf)) >= 0 ) {
	  int y;
	  for(y=0; y<OLEDGUI_H; ++y)
	    memcpy(&sent_frame[d][y*OLEDGUI_ROW_BYTES + col*8], &frame.fb[d][y*OLEDGUI_ROW_BYTES + col*8], 8);
	  dirty[c] = 0;
	}
	break;
      }
    }
  }

  return 0;
}
