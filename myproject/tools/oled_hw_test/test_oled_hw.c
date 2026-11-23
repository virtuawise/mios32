// Host test of apps/sequencers/midibox_seq_v4/core/seq_oled_hw.c: emulates 4 SSD1322 (command/data parser, column/row
// window with horizontal address increment, CS low active on J15 D0..D3) and checks that every cell ends up at the right
// place of the right display, and that nothing is written to a display that is not selected.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mios32.h"
#include "seq_oled_hw.h"
static u8 cs_mask = 0xff, dc = 0;
static struct { u8 ram[128][256]; u8 cmd; int nparam; u8 par[4]; int writing; int c0, c1, r0, r1, col, row, half; int on; } d[4];
s32 MIOS32_BOARD_J15_PortInit(u32 m) { return 0; }
s32 MIOS32_BOARD_J15_DataSet(u8 v) { cs_mask = v; return 0; }
s32 MIOS32_BOARD_J15_RS_Set(u8 rs) { dc = rs; return 0; }
s32 MIOS32_BOARD_J15_SerDataShift(u8 v) {
  for(int i=0; i<4; ++i) {
    if( cs_mask & (1 << i) ) continue;                 // not selected
    if( !dc ) { d[i].cmd = v; d[i].nparam = 0; d[i].writing = (v == 0x5c);
      if( v == 0x5c ) { d[i].col = d[i].c0; d[i].row = d[i].r0; d[i].half = 0; }
      if( v == 0xaf ) d[i].on = 1; if( v == 0xae ) d[i].on = 0; continue; }
    if( d[i].writing ) {                                  // 2 bytes per column address (4 pixels)
      d[i].ram[d[i].row][d[i].col * 2 + d[i].half] = v;
      if( ++d[i].half == 2 ) { d[i].half = 0; if( ++d[i].col > d[i].c1 ) { d[i].col = d[i].c0; if( ++d[i].row > d[i].r1 ) d[i].row = d[i].r0; } }
      continue;
    }
    if( d[i].nparam < 4 ) d[i].par[d[i].nparam++] = v;
    if( d[i].cmd == 0x15 && d[i].nparam == 2 ) { d[i].c0 = d[i].par[0]; d[i].c1 = d[i].par[1]; }
    if( d[i].cmd == 0x75 && d[i].nparam == 2 ) { d[i].r0 = d[i].par[0]; d[i].r1 = d[i].par[1]; }
  }
  return 0;
}
int main(void) {
  memset(d, 0x55, sizeof(d));
  for(int i=0; i<4; ++i) { d[i].on = 0; }
  if( SEQ_OLED_HW_Init(4, 0x9f) < 0 ) { printf("init failed\n"); return 1; }
  int err = 0;
  for(int i=0; i<4; ++i) {                              // init: display on, visible RAM cleared
    if( !d[i].on ) { printf("display %d not on\n", i); ++err; }
    for(int y=0; y<64; ++y) for(int x=0x1c*2; x<0x5c*2; ++x) if( d[i].ram[y][x] ) { ++err; }
  }
  static u8 cells[16][2048];
  srand(1);
  for(int c=0; c<16; ++c) for(int k=0; k<2048; ++k) cells[c][k] = rand();
  for(int c=0; c<16; ++c) SEQ_OLED_HW_WriteCell(c / 4, c % 4, cells[c]);
  for(int c=0; c<16; ++c) {                             // cell c: display c/4, pixels 64*(c%4).., RAM column 0x1c + 16*(c%4)
    int disp = c / 4, x0 = (0x1c + 16 * (c % 4)) * 2;
    for(int y=0; y<64; ++y) if( memcmp(&d[disp].ram[y][x0], &cells[c][y * 32], 32) ) { printf("cell %d row %d differs\n", c, y); ++err; break; }
  }
  if( SEQ_OLED_HW_WriteCell(4, 0, cells[0]) >= 0 ) { printf("display 4 accepted\n"); ++err; }
  printf("%s (%d errors)\n", err ? "FAIL" : "OK: init + 16 cells on 4 displays", err);
  return err ? 1 : 0;
}
