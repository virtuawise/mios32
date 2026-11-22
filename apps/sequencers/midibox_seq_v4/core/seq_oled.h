// $Id$
/*
 * 4x OLED (SSD1322, 256x64) stap-weergave: bouwt het beeld uit de SEQ-status en stuurt het in remote-servermodus
 * als framebuffer naar de client (virtueel paneel, /oled). Eigen uitbreiding (branch four-screens-usb).
 */
#ifndef _SEQ_OLED_H
#define _SEQ_OLED_H

extern s32 SEQ_OLED_Init(u32 mode);
extern u8  SEQ_OLED_MenuButtonGet(void);   // GP button (0-15) of the current menu page, 0x7f = none
extern s32 SEQ_OLED_Handler(void);   // aanroepen elke ms (laagste prioriteit); doet zelf rate-limiting

#endif /* _SEQ_OLED_H */
