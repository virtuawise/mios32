#!/bin/sh
# Build, route and check one board:  ./route.sh column   (or: ./route.sh left)
# build_<b>.py -> Specctra DSN -> Freerouting (GND left to the planes) -> route_board.py (SES import, GND planes, stitching)
# -> verify_<b>.py (every pad against the datasheets) -> KiCad DRC (drc.json)
set -e
B=$1; D=$(cd "$(dirname "$0")" && pwd)/$B
PY=/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3
JAVA=$HOME/toolchains/jre25/Contents/Home/bin/java; [ -x "$JAVA" ] || JAVA=$(ls -d $HOME/toolchains/jre25*/bin/java $HOME/toolchains/jre25/*/bin/java 2>/dev/null | head -1)
cd "$D"
"$PY" build_$B.py 2>&1 | grep -v "wxApp\|stdpbase\|assert\|memory leak"
"$PY" -c "import pcbnew; b=pcbnew.LoadBoard('$B.kicad_pcb'); assert pcbnew.ExportSpecctraDSN(b, '$B.dsn')" 2>/dev/null
"$JAVA" -jar $HOME/toolchains/freerouting/freerouting-2.5.0.jar -de $B.dsn -do $B.ses -mp 100 -inc GND --gui.enabled=false > /tmp/freerouting_$B.log 2>&1
"$PY" ../route_board.py $B.kicad_pcb 2>&1 | grep -v "wxApp\|stdpbase\|assert\|memory leak"
"$PY" verify_$B.py 2>&1 | grep -v "wxApp\|stdpbase\|assert\|memory leak"
kicad-cli pcb drc --severity-all --format json -o drc.json $B.kicad_pcb > /dev/null 2>&1 || true
python3 -c "
import json; from collections import Counter; d=json.load(open('drc.json'))
print('DRC', dict(Counter((v['severity'], v['type']) for v in d['violations'])), 'unconnected', len(d['unconnected_items']))"
kicad-cli pcb render --side top -w 1600 -h 2200 -o render_top.png $B.kicad_pcb > /dev/null 2>&1
kicad-cli pcb render --side bottom -w 1600 -h 2200 -o render_bottom.png $B.kicad_pcb > /dev/null 2>&1
