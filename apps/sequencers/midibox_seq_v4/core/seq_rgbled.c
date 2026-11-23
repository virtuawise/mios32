// $Id$
/*
 * SK6812 key LEDs (see seq_rgbled.h). The SEQ logic keeps switching its LEDs as usual: in MBSEQ_HW.V4 the LEDs of the big
 * keys are assigned to DOUT shift registers that do not exist physically (above the 2 real DOUT registers of the left PCB).
 * This module reads those virtual DOUT bits back and shows them on the RGB chain in the panel palette.
 *
 * Chain order (hardware/README.md): left PCB D1..D5 = Rew, Fwd, Stop, Play, Pause; then column 1..4, each 12 LEDs:
 * GP 1-4, select 1-4, function key 1-4 of that column.
 * Palette (front panel rules): amber #FFB547 = active/selection, warm white #FFF0DA = current track / play position /
 * menu key, sage green #9FD89A = play. Own extension (branch four-screens-usb).
 */
#include <mios32.h>
#include <ws2812.h>
#include "seq_rgbled.h"
#include "seq_hwcfg.h"
#include "seq_led.h"

extern void SEQ_UI_SelLedsGet(u16 *green, u16 *red);

#define RGB_NUM_LEDS  (5 + 4*12)
#define RGB_PERIOD_MS 20

static u8 enabled = 0;
static u8 bright = 64;
static u32 ctr = 0;

typedef struct { u8 r, g, b; } rgb_t;
static const rgb_t C_OFF = {0x00, 0x00, 0x00};
static const rgb_t C_AMBER = {0xff, 0xb5, 0x47};
static const rgb_t C_WHITE = {0xff, 0xf0, 0xda};
static const rgb_t C_GREEN = {0x9f, 0xd8, 0x9a};

static void set(int led, rgb_t c)
{
  WS2812_LED_SetRGB(led, 0, ((u16)c.r * bright) >> 8);
  WS2812_LED_SetRGB(led, 1, ((u16)c.g * bright) >> 8);
  WS2812_LED_SetRGB(led, 2, ((u16)c.b * bright) >> 8);
}

// state of a LED that the SEQ drives on a (virtual) DOUT pin; "0 0" in the hwcfg gives 0xfff8 = not assigned
static u8 pin_on(u16 pin)
{
  return (pin < 8*MIOS32_SRIO_NUM_SR) ? (MIOS32_DOUT_PinGet(pin) ? 1 : 0) : 0;
}

s32 SEQ_RGBLED_Init(u8 brightness)
{
  bright = brightness ? brightness : 1;
  if( WS2812_Init(0) < 0 )
    return -1;
  enabled = 1;
  return 0;
}

s32 SEQ_RGBLED_Handler(void)
{
  if( !enabled || (++ctr % RGB_PERIOD_MS) != 0 )
    return 0;

  // transport (left PCB)
  const u16 tr[5] = { seq_hwcfg_led.rew, seq_hwcfg_led.fwd, seq_hwcfg_led.stop, seq_hwcfg_led.play, seq_hwcfg_led.pause };
  int i;
  for(i=0; i<5; ++i)
    set(i, pin_on(tr[i]) ? (i == 3 ? C_GREEN : C_WHITE) : C_OFF);

  // GP keys: green layer of the SEQ = active (amber), red layer = position marker (warm white)
  u16 gp_g = 0, gp_r = 0;
  if( seq_hwcfg_led.gp_dout_l_sr )  gp_g |= SEQ_LED_SRGet(seq_hwcfg_led.gp_dout_l_sr-1);
  if( seq_hwcfg_led.gp_dout_r_sr )  gp_g |= SEQ_LED_SRGet(seq_hwcfg_led.gp_dout_r_sr-1) << 8;
  if( seq_hwcfg_led.gp_dout_l2_sr ) gp_r |= SEQ_LED_SRGet(seq_hwcfg_led.gp_dout_l2_sr-1);
  if( seq_hwcfg_led.gp_dout_r2_sr ) gp_r |= SEQ_LED_SRGet(seq_hwcfg_led.gp_dout_r2_sr-1) << 8;

  // select keys: as the virtual panel (green = group/selection amber, red = current track warm white)
  u16 sel_g = 0, sel_r = 0;
  SEQ_UI_SelLedsGet(&sel_g, &sel_r);

  // function row Copy..Utility (hwcfg order of the panel)
  const u16 fn[16] = { seq_hwcfg_led.copy, seq_hwcfg_led.paste, seq_hwcfg_led.clear, seq_hwcfg_led.move,
                       seq_hwcfg_led.scroll, seq_hwcfg_led.fast, seq_hwcfg_led.all, seq_hwcfg_led.select,
                       seq_hwcfg_led.down, seq_hwcfg_led.up, seq_hwcfg_led.exit, seq_hwcfg_led.menu,
                       seq_hwcfg_led.edit, seq_hwcfg_led.pattern, seq_hwcfg_led.song, seq_hwcfg_led.utility };

  for(i=0; i<16; ++i) {
    int base = 5 + 12*(i / 4), k = i % 4;
    set(base + k,     ((gp_r >> i) & 1) ? C_WHITE : ((gp_g >> i) & 1) ? C_AMBER : C_OFF);
    set(base + 4 + k, ((sel_r >> i) & 1) ? C_WHITE : ((sel_g >> i) & 1) ? C_AMBER : C_OFF);
    set(base + 8 + k, pin_on(fn[i]) ? C_AMBER : C_OFF);
  }
  return 0;
}
