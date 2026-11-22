// $Id$
/*
 * 4x OLED (SSD1322, 256x64) stap-weergave, zie seq_oled.h
 * Tekenen gebeurt in myproject/oled_gui/oled_gui2 (platformonafhankelijk, op de Mac getest).
 * Elke stap-kolom (64x64 px, 4 bit grijs) wordt apart gerenderd, alleen als zijn invoer verandert, en run-length
 * gecodeerd naar de client gestuurd (remote servermodus, SysEx 09 07).
 */
#include <mios32.h>
#include <string.h>
#include <stdio.h>
#include "tasks.h"

#include "seq_oled.h"
#include "seq_ui.h"
#include "seq_core.h"
#include "seq_bpm.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_layer.h"
#include "seq_midi_sysex.h"
#include "seq_mixer.h"
#include "seq_cc.h"
#include "seq_pattern.h"
#include "seq_file.h"

#include "oled_gui2.h"

#define OLED_PERIOD_MS      20     // invoer verzamelen/vergelijken: max 50x per seconde
#define OLED_RESEND_MS    3000     // alles opnieuw sturen (client kan later binnenkomen)
#define OLED_SEND_TICK_MS    4     // hooguit 1 cel per 4 ms versturen
#define OLED_NUM_CELLS      16

static og2_step_t steps[16];
static u8 play_step = 0xff, trail_step = 0xff;
static int range_lo, range_hi;
static og2_kind_t cur_kind = OG2_K_NOTE;
static u8 cur_mode = 0;                 // 0 = stapweergave, 1 = track-overzicht (MUTE), 2 = mixer (MIXER), 3 = dashboard (BPM/PATTERN/SONG/SAVE)
static og2_tile_t tiles[16];
static og2_dash_t dash;
static char cur_layer_name[8] = "VEL";
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

  // gekozen parameterlaag bepaalt de weergave
  u8 layer = ui_selected_par_layer;
  seq_par_layer_type_t ltype = SEQ_PAR_AssignmentGet(track, layer);
  switch( ltype ) {
    case SEQ_PAR_Type_Note: case SEQ_PAR_Type_Chord1: case SEQ_PAR_Type_Chord2: case SEQ_PAR_Type_Chord3: cur_kind = OG2_K_NOTE; break;
    case SEQ_PAR_Type_Velocity: cur_kind = OG2_K_VEL; break;
    case SEQ_PAR_Type_Length: cur_kind = OG2_K_LEN; break;
    case SEQ_PAR_Type_Probability: cur_kind = OG2_K_PROB; break;
    case SEQ_PAR_Type_Delay: cur_kind = OG2_K_DELAY; break;
    case SEQ_PAR_Type_Roll: cur_kind = OG2_K_ROLL; break;
    default: cur_kind = OG2_K_GENERIC; break;
  }
  {
    char tmp[16];
    tmp[0] = 0;
    if( SEQ_PAR_AssignedTypeStr(track, layer, instrument, tmp) >= 0 && tmp[0] ) {
      int k;
      for(k=0; k<4 && tmp[k]; ++k) cur_layer_name[k] = tmp[k];
      cur_layer_name[k] = 0;
    } else {
      strcpy(cur_layer_name, "PAR");
    }
  }

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
    if( cur_kind != OG2_K_NOTE && step < 256 ) {
      s32 raw = SEQ_PAR_Get(track, step, layer, instrument);
      if( raw < 0 ) raw = 0;
      s->lval = raw & 0x7f;
      switch( cur_kind ) {
        case OG2_K_LEN:  if( raw >= 96 ) strcpy(s->txt, "Gld."); else sprintf(s->txt, "%d%%", (int)(raw * 100) / 96); break;
        case OG2_K_PROB: sprintf(s->txt, "%d%%", (int)(raw > 100 ? 100 : raw)); break;
        case OG2_K_ROLL:
          if( raw == 0 ) strcpy(s->txt, "----");
          else sprintf(s->txt, "%d%c%02d", (int)(((raw & 0x30)>>4) + 2), (raw & 0x40) ? 'U' : 'D', (int)(raw & 0xf));
          break;
        default: sprintf(s->txt, "%d", (int)raw); break;
      }
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

  // glide: loopt van een stap met Gld. naar de volgende getriggerde stap op de pagina, ook door lege stappen heen
  for(i=0; i<16; ++i) {
    steps[i].glide_to = 0xff;
    steps[i].gl_kind = 0;
  }
  for(i=0; i<16; ++i) {
    if( steps[i].sld && steps[i].on && steps[i].note != 0xff ) {
      int j, m;
      for(j=i+1; j<16; ++j)
	if( steps[j].on && steps[j].note != 0xff ) break;
      if( j < 16 ) {
	int p0 = steps[i].note * 16, p1 = steps[j].note * 16;
	int D = (64 - 46) + 64 * (j - i - 1) + 4;           // horizontale afstand van balk-einde tot balk-begin (px)
	steps[i].gl_kind = 1;
	steps[i].glide_to = steps[j].note;
	steps[i].gl_b = p0 + ((p1 - p0) * 15) / D;            // helling bij de rechterrand van de bronstap (x 46 -> 61)
	for(m=i+1; m<j; ++m) {
	  int a0 = (64 - 46) + 64 * (m - i - 1);
	  steps[m].gl_kind = 2;
	  steps[m].gl_a = p0 + ((p1 - p0) * a0) / D;
	  steps[m].gl_b = p0 + ((p1 - p0) * (a0 + 63)) / D;
	}
      }
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

// overzichtspagina's: elke cel is een track (MUTE-pagina) of een mixerkanaal (MIXER-pagina)
static void collect_tiles(u8 mode)
{
  int t;
  u8 running = SEQ_BPM_IsRunning();
  for(t=0; t<16; ++t) {
    og2_tile_t *tl = &tiles[t];
    memset(tl, 0, sizeof(*tl));
    if( mode == 1 ) {
      u16 len = SEQ_CC_Get(t, SEQ_CC_LENGTH) + 1;
      if( len > 255 ) len = 255;
      u8 pos = running ? seq_core_trk[t].step : 0xff;
      u8 win = (pos != 0xff) ? (pos / 16) : ui_selected_step_view;
      int i;
      sprintf(tl->label, "G%dT%d", (t / 4) + 1, (t % 4) + 1);
      tl->flags = ((ui_selected_tracks >> t) & 1) | (((seq_core_trk_muted >> t) & 1) << 1) | (((seq_core_trk_soloed >> t) & 1) << 2);
      tl->len = len;
      tl->pos = pos;
      tl->win = win;
      for(i=0; i<16; ++i) {
	u16 step = (u16)win * 16 + i;
	if( step < len && SEQ_TRG_GateGet(t, step, 0) )
	  tl->gates |= (1 << i);
      }
    } else {
      s32 v;
      sprintf(tl->label, "CH%02d", t + 1);
      v = SEQ_MIXER_Get(t, SEQ_MIXER_PAR_VOLUME);   tl->v1 = (v < 0) ? 0 : (v & 0x7f);
      v = SEQ_MIXER_Get(t, SEQ_MIXER_PAR_PANORAMA); tl->v2 = (v < 0) ? 64 : (v & 0x7f);
      v = SEQ_MIXER_Get(t, SEQ_MIXER_PAR_PRG);      tl->v3 = (v < 0) ? 0 : (v & 0x7f);
    }
  }
}

// dashboard: BPM, maat/tel, patronen per groep, sessienaam
static void collect_dash(void)
{
  int g, k;
  u32 tick = SEQ_BPM_TickGet();
  u32 ppqn = SEQ_BPM_PPQN_Get();
  if( ppqn < 4 ) ppqn = 96;
  memset(&dash, 0, sizeof(dash));
  dash.bpm10 = (u16)(SEQ_BPM_Get() * 10.0f + 0.5f);
  dash.running = SEQ_BPM_IsRunning() ? 1 : 0;
  dash.master = SEQ_BPM_IsMaster() ? 1 : 0;
  dash.bar = (tick / (ppqn * 4)) + 1;
  dash.beat = (tick / ppqn) % 4;
  dash.sixt = (tick / (ppqn / 4)) % 16;
  for(k=0; k<12 && seq_file_session_name[k]; ++k)
    dash.session[k] = seq_file_session_name[k];
  for(g=0; g<4; ++g) {
    char *nm = SEQ_PATTERN_NameGet(g);
    dash.pat_group[g] = seq_pattern[g].group;
    dash.pat_num[g] = seq_pattern[g].num;
    dash.pat_req[g] = seq_pattern[g].REQ ? 1 : 0;
    if( nm )
      for(k=0; k<9 && nm[k]; ++k)
	dash.pat_name[g][k] = nm[k];
  }
  dash.sel_track = SEQ_UI_VisibleTrackGet();
  for(k=0; k<16; ++k)
    if( (seq_core_trk_muted >> k) & 1 )
      ++dash.muted_cnt;
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
    cur_mode = (ui_page == SEQ_UI_PAGE_MUTE) ? 1 : ((ui_page == SEQ_UI_PAGE_MIXER) ? 2 :
	       ((ui_page == SEQ_UI_PAGE_BPM || ui_page == SEQ_UI_PAGE_PATTERN || ui_page == SEQ_UI_PAGE_SONG || ui_page == SEQ_UI_PAGE_SAVE) ? 3 : 0));
    if( cur_mode == 0 )
      collect_steps();
    else if( cur_mode == 3 )
      collect_dash();
    else
      collect_tiles(cur_mode);
    if( resend_ctr >= OLED_RESEND_MS ) {
      resend_ctr = 0;
      for(c=0; c<OLED_NUM_CELLS; ++c)
	sent_valid[c] = 0;
    }
    for(c=0; c<OLED_NUM_CELLS; ++c) {
      u32 h = 2166136261u;
      h = fnv(h, &cur_mode, 1);
      if( cur_mode == 0 )
	h = fnv(h, (const u8 *)&steps[c], sizeof(og2_step_t));
      else if( cur_mode == 3 ) {
	// alleen de velden die dit scherm (c/4) tekent: voorkomt dat alle cellen bij elke zestiende opnieuw gestuurd worden
	switch( c / 4 ) {
	  case 0: h = fnv(h, (const u8 *)&dash.bpm10, 2); h = fnv(h, &dash.running, 2); h = fnv(h, &dash.beat, 1); break;
	  case 1: h = fnv(h, (const u8 *)&dash.bar, 2); h = fnv(h, &dash.beat, 1); h = fnv(h, &dash.sixt, 1); h = fnv(h, &dash.running, 1); break;
	  case 2: h = fnv(h, dash.pat_group, 12); h = fnv(h, (const u8 *)dash.pat_name, sizeof(dash.pat_name)); break;
	  default: h = fnv(h, (const u8 *)dash.session, sizeof(dash.session)); h = fnv(h, &dash.sel_track, 2); h = fnv(h, &dash.running, 1); break;
	}
      }
      else
	h = fnv(h, (const u8 *)&tiles[c], sizeof(og2_tile_t));
      h = fnv(h, (const u8 *)&range_lo, sizeof(int));
      h = fnv(h, (const u8 *)&range_hi, sizeof(int));
      u8 flags[3] = { play_step == c, trail_step == c, cur_kind };
      h = fnv(h, flags, 3);
      h = fnv(h, (const u8 *)cur_layer_name, 4);
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
	if( cur_mode == 3 )
	  OG2_RenderDashCell(cell_buf, c, &dash);
	else if( cur_mode == 1 )
	  OG2_RenderTrackTile(cell_buf, c, &tiles[c]);
	else if( cur_mode == 2 )
	  OG2_RenderMixerTile(cell_buf, c, &tiles[c]);
	else
	  OG2_RenderCell(cell_buf, c, &steps[c], range_lo, range_hi, play_step == c, trail_step == c, cur_layer_name, cur_kind);
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
