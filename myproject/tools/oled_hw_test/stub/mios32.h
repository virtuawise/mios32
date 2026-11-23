// host stub for seq_oled_hw.c: J15 calls feed an SSD1322 emulator (test_oled_hw.c)
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
s32 MIOS32_BOARD_J15_PortInit(u32 mode);
s32 MIOS32_BOARD_J15_DataSet(u8 data);
s32 MIOS32_BOARD_J15_RS_Set(u8 rs);
s32 MIOS32_BOARD_J15_SerDataShift(u8 data);
