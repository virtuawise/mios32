// $Id$
/*
 * 4x OLED (SSD1322, 256x64) stap-weergave, zie seq_oled.h
 * Tekenen gebeurt in myproject/oled_gui/oled_gui2 (platformonafhankelijk, op de Mac getest).
 * Elke stap-kolom (64x64 px, 4 bit grijs) wordt apart gerenderd, alleen als zijn invoer verandert, en run-length
 * gecodeerd naar de client gestuurd (remote servermodus, SysEx 09 07).
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

#include "oled_gui2.h"

#define OLED_PERIOD_MS      20     // invoer verzamelen/vergelijken: max 50x per seconde
#define OLED_RESEND_MS    3000     // alles opnieuw sturen (client kan later binnenkomen)
#define OLED_SEND_TICK_MS    4     // hooguit 1 cel per 4 ms versturen
#define OLED_NUM_CELLS      16

static og2_step_t steps[16];
static u8 play_step = 0xff, trail_step = 0xff;
static int range_lo, range_hi;
static u32 sent_hash[OLED_NUM_CELLS];
static u8 sent_valid[OLED_NUM_CELLS];
static u8 dirty[OLED_NUM_CELLS];
static u32 cur_hash[OLED_NUM_CELLS];
static u32 ctr = 0, resend_ctr = 0;
static u8 next_cell = 0;
static u8 cell_buf[2048];
static u8 rle_buf[2112];

s32 SEQ_OLED_Init(u32 mode)
{
  int c;
  for(c=0; c<OLED_NUM_CELLS; ++c)
    sent_valid[c] = 0;
  return 0;
}

static u32 fnv(u32 h, const u8 *p, int n)
{
  while( n-- ) { h ^= *p++; h *= 16777619u; }
  return h;
}

// verzamelt de 16 zichtbare stappen van de geselecteerde track
static void collect_steps(void)
{
  u8 track = SEQ_UI_VisibleTrackGet();
  u8 instrument = ui_selected_instrument;
  u16 base = (u16)ui_selected_step_view * 16;
  int i;

  for(i=0; i<16; ++i) {
    u16 step = base + i;
    seq_layer_evnt_t e;
    og2_step_t *s = &steps[i];
    memset(s, 0, sizeof(*s));
    s->note = 0xff;
    s->prob = 100;
    s->on = SEQ_TRG_GateGet(track, step, instrument) ? 1 : 0;
    s->acc = SEQ_TRG_AccentGet(track, step, instrument) ? 1 : 0;
    s->sld = SEQ_TRG_GlideGet(track, step, instrument) ? 1 : 0;
    s->roll = SEQ_TRG_RollGet(track, step, instrument) ? 1 : 0;
    s->skip = SEQ_TRG_SkipGet(track, step, instrument) ? 1 : 0;
    if( step < 256 ) {
      s32 p = SEQ_PAR_ProbabilityGet(track, step, instrument, 0x0000);
      if( p >= 0 && p <= 100 ) s->prob = p;
      s32 d = SEQ_PAR_StepDelayGet(track, step, instrument, 0x0000);
      if( d > 0 && d < 96 ) s->delay = (d * 100) / 96;
    }
    if( s->on && SEQ_LAYER_GetEvntOfLayer(track, step, 0, instrument, &e) >= 0 ) {
      s->note = e.midi_package.note;
      s->vel = e.midi_package.velocity & 0x7f;
      s32 l = e.len;
      if( l < 0 && step < 256 ) l = SEQ_PAR_LengthGet(track, step, instrument, 0x0000);
      if( l < 0 ) l = 48;
      if( l >= 96 ) {            // lengte 'Gld.' = glide (legato naar de volgende noot)
	s->len = 100;
	s->sld = 1;
      } else {
	s->len = (l * 100) / 96;
      }
      s->value = s->vel;       // actieve laag: velocity (later: laag van het paneel)
    }
  }

  u8 new_play = 0xff;
  if( SEQ_BPM_IsRunning() ) {
    u8 cur = seq_core_trk[track].step;
    if( (cur / 16) == ui_selected_step_view )
      new_play = cur % 16;
  }
  if( new_play != play_step ) {
    trail_step = (play_step != 0xff) ? play_step : 0xff;
    play_step = new_play;
  }
  OG2_StepRange(steps, &range_lo, &range_hi);
}

// run-length (PackBits): 0..127 = kopieer c+1 bytes; 128..255 = herhaal volgende byte (c-126) keer
static int rle_encode(const u8 *in, int n, u8 *out)
{
  int i = 0, o = 0;
  while( i < n ) {
    int run = 1;
    while( i+run < n && in[i+run] == in[i] && run < 129 ) ++run;
    if( run >= 2 ) {
      out[o++] = 126 + run;     // 128..255
      out[o++] = in[i];
      i += run;
    } else {
      int start = i, len = 0;
      while( i < n && len < 128 && !(i+1 < n && in[i] == in[i+1]) ) { ++i; ++len; }
      if( len == 0 ) { ++i; len = 1; }
      out[o++] = len - 1;
      memcpy(&out[o], &in[start], len);
      o += len;
    }
  }
  return o;
}

s32 SEQ_OLED_Handler(void)
{
  // alleen in remote-servermodus (virtueel paneel); echte OLED-hardware volgt later
  if( !(seq_midi_sysex_remote_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER ||
        seq_midi_sysex_remote_active_mode == SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER) )
    return 0;

  ++ctr;
  ++resend_ctr;

  if( (ctr % OLED_PERIOD_MS) == 0 ) {
    int c;
    collect_steps();
    if( resend_ctr >= OLED_RESEND_MS ) {
      resend_ctr = 0;
      for(c=0; c<OLED_NUM_CELLS; ++c)
	sent_valid[c] = 0;
    }
    for(c=0; c<OLED_NUM_CELLS; ++c) {
      u32 h = 2166136261u;
      h = fnv(h, (const u8 *)&steps[c], sizeof(og2_step_t));
      h = fnv(h, (const u8 *)&range_lo, sizeof(int));
      h = fnv(h, (const u8 *)&range_hi, sizeof(int));
      u8 flags[2] = { play_step == c, trail_step == c };
      h = fnv(h, flags, 2);
      cur_hash[c] = h;
      dirty[c] = !sent_valid[c] || sent_hash[c] != h;
    }
  }

  // verstuur hooguit 1 gewijzigde cel per OLED_SEND_TICK_MS
  if( (ctr % OLED_SEND_TICK_MS) == 1 ) {
    int tries;
    for(tries=0; tries<OLED_NUM_CELLS; ++tries) {
      u8 c = next_cell;
      next_cell = (next_cell + 1) % OLED_NUM_CELLS;
      if( dirty[c] ) {
	OG2_RenderCell(cell_buf, c, &steps[c], range_lo, range_hi, play_step == c, trail_step == c, "VEL");
	int n = rle_encode(cell_buf, sizeof(cell_buf), rle_buf);
	if( SEQ_MIDI_SYSEX_REMOTE_Server_SendOLEDCell(c / 4, c % 4, rle_buf, n) >= 0 ) {
	  sent_hash[c] = cur_hash[c];
	  sent_valid[c] = 1;
	  dirty[c] = 0;
	}
	break;
      }
    }
  }

  return 0;
}
