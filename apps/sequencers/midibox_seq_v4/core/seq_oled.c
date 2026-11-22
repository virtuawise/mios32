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

#define OLED_PERIOD_MS      80     // maximaal ~12 updates per seconde
#define OLED_RESEND_MS    2000     // alles opnieuw sturen (client kan later binnenkomen)

static oledgui_frame_t frame;
static u8 sent_frame[OLEDGUI_NUM_DISPLAYS][OLEDGUI_DISPLAY_BYTES];
static u32 ctr = 0;
static u32 resend_ctr = 0;
static u8 next_display = 0;
static u8 dirty[OLEDGUI_NUM_DISPLAYS];

s32 SEQ_OLED_Init(u32 mode)
{
  int d;
  for(d=0; d<OLEDGUI_NUM_DISPLAYS; ++d)
    dirty[d] = 1;
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
      for(d=0; d<OLEDGUI_NUM_DISPLAYS; ++d)
	dirty[d] = 1;
    }
    for(d=0; d<OLEDGUI_NUM_DISPLAYS; ++d)
      if( memcmp(frame.fb[d], sent_frame[d], OLEDGUI_DISPLAY_BYTES) != 0 )
	dirty[d] = 1;
  }

  // verstuur om de 8 ms hooguit 1 vervuild scherm (houdt de USB-buffer leeg)
  if( (ctr % 8) == 3 ) {
    int tries;
    for(tries=0; tries<OLEDGUI_NUM_DISPLAYS; ++tries) {
      u8 d = next_display;
      next_display = (next_display + 1) % OLEDGUI_NUM_DISPLAYS;
      if( dirty[d] ) {
	if( SEQ_MIDI_SYSEX_REMOTE_Server_SendOLED(d, frame.fb[d], OLEDGUI_DISPLAY_BYTES) >= 0 ) {
	  memcpy(sent_frame[d], frame.fb[d], OLEDGUI_DISPLAY_BYTES);
	  dirty[d] = 0;
	}
	break;
      }
    }
  }

  return 0;
}
