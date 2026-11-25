// $Id$
/*
 * Real SSD1322 OLEDs on core J15 (see seq_oled_hw.h). Own extension (branch four-screens-usb).
 *
 * Sources: SSD1322 datasheet rev 1.2 (commands), Newhaven NHD-3.12-25664UC datasheet rev 7 (4-wire SPI pinout, 256 pixels
 * at RAM columns 0x1C..0x5B = 64 column addresses of 4 pixels), MBHP_CORE_STM32F4 schematic (J15 pins), mios32_board.c.
 */
#include <mios32.h>
#include "tasks.h"
#include "seq_oled_hw.h"

u8 seq_oled_hw_active = 0;
static u8 num_disp = 0;

#if defined(MIOS32_BOARD_MBHP_CORE_STM32F4) && !defined(MIOS32_FAMILY_EMULATION)
// own bit-bang with a slower clock than MIOS32_BOARD_J15_SerDataShift (SSD1322: SCLK cycle >= 100 ns, low/high >= 50 ns;
// the bus runs over ribbon cables to 4 boards). E1 = PC8 and E2 = PC9 both carry SCLK, RW = PC11 carries SDIN.
#define SCLK_0   { GPIOC->BSRRH = GPIO_Pin_8 | GPIO_Pin_9; }
#define SCLK_1   { GPIOC->BSRRL = GPIO_Pin_8 | GPIO_Pin_9; }
#define SDIN(b)  { if( b ) GPIOC->BSRRL = GPIO_Pin_11; else GPIOC->BSRRH = GPIO_Pin_11; }
static void shift(u8 data)
{
  int i;
  MIOS32_IRQ_Disable();
  for(i=0; i<8; ++i, data <<= 1) {
    SDIN(data & 0x80);
    SCLK_0; SCLK_0; SCLK_0; SCLK_0; SCLK_0; SCLK_0; SCLK_0; SCLK_0;   // setup + low phase (~60 ns at 168 MHz)
    SCLK_1; SCLK_1; SCLK_1; SCLK_1; SCLK_1; SCLK_1; SCLK_1; SCLK_1;   // data sampled on the rising edge, high phase
  }
  SCLK_0;
  SDIN(0);      // RW is also OE# of the core's 74HC595 that drives the CS lines: keep it low (CS outputs active) between bytes
  MIOS32_IRQ_Enable();
}
#else
static void shift(u8 data) { MIOS32_BOARD_J15_SerDataShift(data); }
#endif

static void sel(u8 mask) { MIOS32_BOARD_J15_DataSet(~mask); }            // CS0..3 = J15 D0..D3, low active
static void cmd(u8 c)    { MIOS32_BOARD_J15_RS_Set(0); shift(c); }
static void dat(u8 d)    { MIOS32_BOARD_J15_RS_Set(1); shift(d); }

/////////////////////////////////////////////////////////////////////////////
// Initialise all displays at once (all CS lines active). /RES is driven by the supervisor on the left PCB.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_OLED_HW_Init(u8 num_displays, u8 contrast)
{
  if( num_displays == 0 || num_displays > 4 ) {
    seq_oled_hw_active = 0;
    return -1;
  }
  MUTEX_LCD_TAKE;
  if( MIOS32_BOARD_J15_PortInit(0) < 0 ) {          // 0 = push-pull 3.3 V (core J15_S jumper at 3.3 V!)
    MUTEX_LCD_GIVE;
    return -2;
  }
  num_disp = num_displays;
  seq_oled_hw_active = 1;
  sel((1 << num_disp) - 1);

  cmd(0xfd); dat(0x12);                  // unlock commands
  cmd(0xae);                             // display off
  cmd(0xb3); dat(0x91);                  // clock divider / oscillator
  cmd(0xca); dat(0x3f);                  // multiplex ratio 64
  cmd(0xa2); dat(0x00);                  // display offset
  cmd(0xa1); dat(0x00);                  // start line
  cmd(0xa0); dat(0x14); dat(0x11);       // horizontal increment, nibble remap, COM scan direction, dual COM (Newhaven)
  cmd(0xb5); dat(0x00);                  // GPIO off
  cmd(0xab); dat(0x01);                  // internal VDD regulator
  cmd(0xb4); dat(0xa0); dat(0xfd);       // display enhancement A: external VSL, enhanced low GS quality
  cmd(0xc1); dat(contrast);              // contrast current
  cmd(0xc7); dat(0x0f);                  // master current
  cmd(0xb9);                             // linear grey scale table
  cmd(0xb1); dat(0xe2);                  // phase length
  cmd(0xd1); dat(0x82); dat(0x20);       // display enhancement B
  cmd(0xbb); dat(0x1f);                  // pre-charge voltage
  cmd(0xb6); dat(0x08);                  // second pre-charge period
  cmd(0xbe); dat(0x07);                  // VCOMH
  cmd(0xa6);                             // normal display

  // clear the visible RAM (columns 0x1c..0x5b, rows 0..63)
  cmd(0x15); dat(0x1c); dat(0x5b);
  cmd(0x75); dat(0x00); dat(0x3f);
  cmd(0x5c);
  MIOS32_BOARD_J15_RS_Set(1);
  {
    int i;
    for(i=0; i<128*64; ++i)
      shift(0x00);
  }
  cmd(0xaf);                             // display on
  sel(0);
  MUTEX_LCD_GIVE;
  return 0;
}

s32 SEQ_OLED_HW_Contrast(u8 contrast)
{
  if( !seq_oled_hw_active )
    return -1;
  MUTEX_LCD_TAKE;
  sel((1 << num_disp) - 1);
  cmd(0xc1); dat(contrast);
  sel(0);
  MUTEX_LCD_GIVE;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// One 64x64 cell (= the column above one GP encoder) of display <disp>: column address window 16 x 4 pixels.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_OLED_HW_WriteCell(u8 disp, u8 cell, const u8 *buf2048)
{
  if( !seq_oled_hw_active || disp >= num_disp || cell >= 4 )
    return -1;
  MUTEX_LCD_TAKE;
  sel(1 << disp);
  cmd(0x15); dat(0x1c + 16*cell); dat(0x1c + 16*cell + 15);
  cmd(0x75); dat(0x00); dat(0x3f);
  cmd(0x5c);                             // write RAM
  MIOS32_BOARD_J15_RS_Set(1);
  {
    int i;
    for(i=0; i<2048; ++i)
      shift(buf2048[i]);
  }
  sel(0);
  MUTEX_LCD_GIVE;
  return 0;
}
