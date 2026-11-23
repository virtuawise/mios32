#!/bin/sh
# Host test of the SSD1322 driver (seq_oled_hw.c) against an SSD1322 emulator. The driver is copied next to the stubs
# because its own directory has the real tasks.h (FreeRTOS).
set -e
D=$(cd "$(dirname "$0")" && pwd); C=$D/../../../apps/sequencers/midibox_seq_v4/core; B=$(mktemp -d)
cp $C/seq_oled_hw.c $C/seq_oled_hw.h $D/stub/*.h $B/
cc -Wall -I $B -o $B/t $D/test_oled_hw.c $B/seq_oled_hw.c
$B/t
