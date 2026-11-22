# Analyse MBSEQ V4+ bediening -> 4 OLED-schermen met blokjes (2026-11-22, voorstel)

Bron: `apps/sequencers/midibox_seq_v4/core/seq_ui_*.c` (de indeling van elke pagina staat bovenin beschreven).
Uitgangspunt: 4 schermen x 4 kolommen = 16 kolommen, onder elke kolom 1 encoder (GP1-16) + 1 knop. De originele firmware verdeelt de instellingen over
veel pagina's omdat 2x40 tekens weinig plek bieden; hier kan een pagina 16 parameters tegelijk tonen (4 groepen van 4, 1 groep per scherm).

## Blok-typen (per kolom, of over 2-3 kolommen samengevoegd met een kader)
- **Waarde**: label klein, waarde groot, balkje (bv. Length, Loop, Intensity).
- **Keuzelijst**: huidige optie groot, vorige/volgende gedimd boven/onder (bv. Direction, Mode, Type).
- **Schakelaar**: AAN = gevuld blok, UIT = rand (bv. Hold, Sort, Sync, Manual).
- **Kiezer** (2-3 kolommen): raster van opties met de gekozen optie gemarkeerd (bv. Timebase 1 2 4 8 8T 16 16T 32; Length-presets; Scale).
- **Grafiek** (2-4 kolommen): visualisatie (groove-curve, LFO-golfvorm, euclid-ring, morph-balk, loop-gebied).

## Inventaris pagina's (wat er in de firmware zit)
| Pagina | Parameters (uit de LCD-indeling) |
|---|---|
| TRKEVNT (Event) | Trk., Type (Note/Chord/CC/Drum), Steps/ParL/TrgL, Port, Chn., Instrument, Layer-keuze, Layer-controls (types), PRESETS, INIT |
| TRKMODE | Mode (Off/Normal/Transpose/Arpeggiator), Bus, Note-stack: Hold, Sort, ReSt., STrg, FTS, Sustain |
| TRKDIR | Direction (Forward/Backward/PingPong/Pendulum/Rand.Dir/Rand.Step/Rand.D+S), Fwd., Back, Replay, Repeat, Skip, Itv., SyncM |
| TRKDIV | Clock divider (timebase 1,2,4,8,8T,16,16T,32 + 'normal'), SyncToMeasure, Manual |
| TRKLEN | Length (1-256), Loop, QuickSel (lengte-presets 4-256, loop-presets) |
| TRKTRAN | Transpose: Scale, Oct, Semi (-7..+7 per GP-knop) |
| TRKGRV | Groove Style, Intensity, Global, SyncStep, Dly., Len., Vel., NumSteps, Clr |
| TRKMORPH | Morph aan/uit, bereik, Morph-waarde |
| TRKRND | Random-generator: Generate, Clr/Util/Undo, lagen |
| TRKEUCLID | Euclid: lengte, laag, stappen, pulsen, offset |
| TRKJAM (Live) | Step / Live / Ptn / MIDI / Misc: Poly, Gate, patroon-grid (*.*.*.), Copy/Paste |
| FX Echo | Repeats, Delay (1/16..), Velocity %, Feedback %, Note-shift, Gate %, Ticks % |
| FX Humanize / Robotize / Limit | Intensiteiten, laag-schakelaars; Limit lower/upper note |
| FX LFO | Golfvorm (Sine...), Amplitude, Phase, Steps, CC, 4 doel-schakelaars, ... |
| FX Dupl / Loop / Scale | Kanalen dupliceren; globale loop (mode/offset/steps); Root + schaal-lijst (50+ schalen) |
| MANUAL | Tracks handmatig triggeren (Selected Tracks 1234 ...), SELECT = sync naar maat |
| BPM | Mode (Master/Slave/Auto), Preset, Tempo, Ramp, MClk In/Out, Delay, Ext. Restart, Tap |
| OPT / MIDI / SYSEX / CV / ETH / DISK / INFO | Globale opties, MIDI-router, SysEx, CV/gate/clock-config (AOUT), netwerk, schijf, info |
| SONG / PATTERN / MUTE / MIXER | al gedaan: dashboard, track-overzicht, mixerhokjes |

## Voorgestelde indeling van de nieuwe pagina's (16 kolommen elk)
1. **TRACK** (Event+Mode+Inst): scherm 1 TYPE (type-lijst, steps, par/trg-lagen), scherm 2 OUTPUT (port, kanaal, instrument), scherm 3 MODE (Off/Normal/Transpose/Arp als keuzelijst + bus), scherm 4 NOTE-STACK (Hold, Sort, ReSt., STrg, FTS, Sustain als schakelaars).
2. **TIMING** (Div+Len+Dir): scherm 1 LENGTE (length + loop + lengte-kiezer), scherm 2 KLOK (timebase-kiezer, sync-naar-maat, manual), scherm 3 RICHTING (7 richtingen als pijl-icons + Fwd/Back), scherm 4 HERHALING (Replay, Repeat, Skip, Itv., SyncM).
3. **GROOVE & TRANSPOSE**: groove-curve als grafiek, style/intensity/global/dly/len/vel; transpose (oct, semi, scale).
4. **GENERATE** (Rnd + Euclid + Morph): euclid-ring, random, morph-balk.
5. **FX**: echo, humanize, robotize, limit, LFO (golfvorm als grafiek), dupl, loop, scale (kiezer).
6. **LIVE** (Jam), **MANUAL**.
7. **GLOBAAL** (BPM, OPT, MIDI, SysEx, CV, ETH, Disk, Info): elk 1 pagina of per scherm 1 onderwerp.
Navigatie: de bestaande knoppen/shortcuts (MENU+GPn, trackknoppen) blijven; ze openen de nieuwe pagina's. De GP-rij onder de schermen kiest binnen de pagina.

## Wat dit in de firmware betekent
Geen LCD-spiegeling meer, maar echte UI-pagina's: lezen/schrijven met `SEQ_CC_Get/Set(track, SEQ_CC_...)` en `SEQ_UI_CC_Inc` met dezelfde min/max als de oorspronkelijke pagina's, werkend op alle geselecteerde tracks (`ui_selected_tracks`). Encoders (`SEQ_UI_ENCODER_GP1..16`) en knoppen worden per pagina aan een blok gekoppeld. Het LCD blijft werken. Testen kan zonder hardware met het virtuele paneel (/oled) omdat de bediening via de remote-keys loopt.
Eerste stap voorstel: pagina TIMING op de Mac tekenen, dan in de firmware bouwen, dan de rest per pagina.
