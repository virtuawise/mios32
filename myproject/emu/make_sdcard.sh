#!/bin/sh
# Creates the emulator's SD card: sdcard.img = 256 MB raw FAT32 image with the folders the SEQ uses
# (/SESSIONS/DEFAULT, /MIDI, /PRESETS, /SYSEX) and the files of a source folder:
#   ./make_sdcard.sh                      newest backup in ../sdcard_backup/<date>/
#   ./make_sdcard.sh /Volumes/MBSEQ       a copy of your real SD card (card in the Mac's reader)
# Look into the image in Finder:  hdiutil attach -imagekey diskimage-class=CRawDiskImage sdcard.img
# (eject it again before starting the emulator)
set -e
cd "$(dirname "$0")"
IMG=sdcard.img
SRC=${1:-$(ls -d ../sdcard_backup/*/ 2>/dev/null | sort | tail -1)}
[ -e "$IMG" ] && { echo "$IMG exists - remove it first (rm $IMG)"; exit 1; }
mkfile -n 256m "$IMG"
DEV=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount "$IMG" | awk 'NR==1{print $1}')
newfs_msdos -F 32 -v MBSEQ "$DEV" > /dev/null
hdiutil detach "$DEV" > /dev/null
MNT=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage "$IMG" | awk -F'\t' 'END{print $NF}')
mkdir -p "$MNT/SESSIONS/DEFAULT" "$MNT/MIDI" "$MNT/PRESETS" "$MNT/SYSEX"
if [ -n "$SRC" ] && [ -d "$SRC" ]; then cp -R "${SRC%/}"/ "$MNT"/ 2>/dev/null || true; echo "copied $SRC"; fi
find "$MNT" -maxdepth 2 -not -name ".*" | sed "s#$MNT#  #"
hdiutil detach "$MNT" > /dev/null
echo "created $IMG"
