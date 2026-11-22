# Ontwerpideeën 4 x OLED (256x64) voor de SEQ V4+  (2026-11-22, voorstel – nog niet gebouwd)

Uitgangspunt: de SEQ heeft veel meer informatie dan 2x40 tekens kunnen tonen (16 tracks, 3 parameterlagen + triggerlagen per stap,
probability, step-delay, roll, glide, accent, echo/FX, arp, akkoordbus, scale/root, patronen, song, mixer, 4 groepen).
Met 4 grafische schermen kun je dat tegelijk én als beeld tonen. Ontwerpregels: nooit een scherm als tekstregel, alles als
beeld/ritme; elke encoder krijgt zijn eigen parameternaam + waarde; de SSD1322 kan 16 grijstinten (4 bit) -> diepte i.p.v. alleen aan/uit.

## 1. Stap-weergave met meerdere lagen tegelijk (vervangt de huidige pagina)
Per stap-kolom (64 px) tegelijk: pianoroll-stip (toonhoogte op 2 oktaven), gatelengte-balkje, velocity-hoogte, probability (dither/vulling),
step-delay (verschuiving van de stip), accent/glide/roll/skip als kleine iconen, glide als lijntje naar de volgende stap, afspeelpositie met
uitdovende "staart". Bovenin de actieve laag (tabs Note | Vel | Len | Prob | Dly | CC) = wat de encoder onder de kolom nu wijzigt, met waarde.
Winst: je ziet de hele stap, niet één laag; je ziet meteen groove en melodie als vorm.

## 2. Track-overzicht (live-weergave)
Elk scherm = 1 trackgroep (G1..G4): 4 tracks als banen van 16 px, 64 stappen op 4 px, afspeelpositie per baan, mute/solo-markering,
activiteit (noot-aan flits), patroonnaam en lengte (polymeter: banen met verschillende lengte lopen zichtbaar uit fase – precies wat de GEN-sessies doen).
Winst: de hele sessie in één blik, ideaal live; polymeter wordt zichtbaar.

## 3. Harmonie / akkoordbus
Pianotoetsen-strook met de klinkende noten gemarkeerd, akkoordnaam (Fmin9), toonsoort + scale, root-beweging van de bus, volgend akkoord
met aftelbalk. Voor arp-tracks: de arp-noten als lopende stippen. Winst: past bij de bus/arp-structuur die we gebruiken; je ziet wat de akkoorden doen.

## 4. Mixer
Per scherm 4 tracks: verticale fader (volume), pan, VU/activiteit, mute/solo-icoon, MIDI-poort + kanaal. Draaien aan encoder toont het getal groot.

## 5. Song / patroon-keten
Tijdlijn van de song-fasen met huidig/volgend patroon, aftellen naar patroonwissel (balk), BPM, A1-H8 raster van patronen met actief patroon.

## 6. Altijd zichtbare statusstrook (bovenste 8 px van scherm 1 en 4)
BPM met pulserende beat, play/stop, bar:beat, sync-bron, sessienaam, MIDI-activiteit per poort, SD/CPU. Zo verdwijnen losse menu's.

## 7. Feedback bij bediening ("popover")
Draai je aan een encoder: de waarde verschijnt groot boven die kolom en dooft na 1 s; tijdens indrukken van een knop de functienaam.
Ook "ghost": vorige waarde als dunne markering zodat je ziet hoeveel je verstelde.

## Techniek
- 4 bit grijs: ssd1322 ondersteunt 16 niveaus; de huidige driver/tekenlaag is 1 bit -> tekenlaag uitbreiden naar 4 bpp (framebuffer 4x8 kB), SysEx-cellen dan 2 kB per cel -> alleen gewijzigde cellen sturen, of delta's.
- Tekenlaag blijft platformonafhankelijk (Mac-test met PNG), paneel (/oled) toont dezelfde frames zodat we alles zonder hardware beoordelen.
- Pagina-structuur: een tabel van "weergaven" (stap, overzicht, harmonie, mixer, song) per scherm instelbaar; encoders/knoppen blijven de normale SEQ-functies bedienen.
