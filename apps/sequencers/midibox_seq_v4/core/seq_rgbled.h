// $Id$
/*
 * SK6812 (WS2812 compatible) key LEDs of the 4-OLED front panel, data on core J4B.SC (PB6), modules/ws2812.
 * Enabled with "RGBLED_ENABLED 1" in MBSEQ_HW.V4. Own extension (branch four-screens-usb).
 */
#ifndef _SEQ_RGBLED_H
#define _SEQ_RGBLED_H

extern s32 SEQ_RGBLED_Init(u8 brightness);
extern s32 SEQ_RGBLED_Handler(void);     // call every ms (low prio); updates every 20 ms

#endif /* _SEQ_RGBLED_H */
