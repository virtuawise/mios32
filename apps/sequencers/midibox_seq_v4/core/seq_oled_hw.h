// $Id$
/*
 * Real SSD1322 OLEDs (4 x Newhaven NHD-3.12-25664UC, 256x64) on core J15, 4-wire SPI:
 * SCLK = J15 E1/E2 (PC8/PC9), SDIN = J15 RW (PC11), D/C = J15 RS (PA8), /CS0..3 = J15 D0..D3 (core 74HC595, low active).
 * Enabled with "OLED_SSD1322 4" in MBSEQ_HW.V4; then the character LCD output to J15 is switched off (same pins).
 * Own extension (branch four-screens-usb).
 */
#ifndef _SEQ_OLED_HW_H
#define _SEQ_OLED_HW_H

extern u8 seq_oled_hw_active;                                          // 1 = J15 belongs to the OLEDs (no CLCD output)

extern s32 SEQ_OLED_HW_Init(u8 num_displays, u8 contrast);
extern s32 SEQ_OLED_HW_WriteCell(u8 disp, u8 cell, const u8 *buf2048);   // 64x64 px, 4 bit, row-major, high nibble = left pixel
extern s32 SEQ_OLED_HW_Contrast(u8 contrast);

#endif /* _SEQ_OLED_HW_H */
