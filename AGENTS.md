## Doel
Je bent Codex en werkt in een ESP32 project dat gebouwd wordt met PlatformIO. Je taak is het aanpassen of toevoegen van firmware code, met focus op correctheid, reproduceerbare builds en minimale regressierisico's.

## Project context
1. Target: ESP32
2. Build systeem: PlatformIO
3. Framework: meestal Arduino via PlatformIO, tenzij expliciet anders vermeld
4. Belangrijke libraries kunnen o.a. AsyncWebServer, AsyncTCP, WiFiManager, AccelStepper, ArduinoJson zijn, maar vertrouw alleen op wat in platformio.ini en lib_deps staat

## Werkwijze en output
1. Werk uitsluitend met de bestaande repo als bron van waarheid
2. Verzint geen niet bestaande bestanden, paden, APIs of hardware aansluitingen
3. Als informatie ontbreekt die nodig is om correct te bouwen of te testen, stel gerichte vragen, anders ga door met wat aantoonbaar in de repo staat
4. Lever wijzigingen als concrete code edits, geen pseudocode
5. Voeg korte rationale toe per wijziging, gekoppeld aan exacte bestanden en functies

## Build en test eisen
1. Elke wijziging moet lokaal buildbaar zijn via PlatformIO
2. Gebruik altijd deze commands tenzij het project iets anders voorschrijft
   1. `pio run`
   2. `pio run -t clean` alleen als je build artefacts wilt uitsluiten
   3. `pio device monitor` alleen als seriele output nodig is
3. Als er meerdere environments zijn, bouw minimaal de environment die in README of in het issue genoemd is, anders bouw alles wat in platformio.ini staat

## PlatformIO richtlijnen
1. platformio.ini is leidend voor board, platform versie, framework, build_flags en lib_deps
2. Pas platform_packages of platform versies alleen aan als het probleem aantoonbaar daar zit en je de impact kan onderbouwen
3. Voeg nieuwe libraries bij voorkeur toe via lib_deps met versie pinning

## Code conventies
### C plus plus en Arduino stijl
1. Geen heap allocaties in hot paths zonder noodzaak
2. Vermijd String concatenatie in loops, gebruik vaste buffers of `snprintf`
3. Houd ISR code minimaal en lock free
4. Gebruik `constexpr` voor constanten waar passend
5. Gebruik duidelijke type keuzes, bijvoorbeeld `uint32_t` voor millis timing

### Foutafhandeling en logs
1. Voeg log output toe die diagnose mogelijk maakt, maar voorkom spam
2. Gebruik een centraal log mechanisme als dat bestaat in de repo
3. Fouten moeten terug te vinden zijn in serial output, zeker bij netwerk en JSON parsing

### JSON en protocol compatibiliteit
1. Protocol velden en versies zijn contract, wijzig ze alleen als expliciet gevraagd
2. Bij ArduinoJson: kies document type bewust
   1. StaticJsonDocument bij bekende bovengrens
   2. JsonDocument alleen als grootte dynamisch moet zijn en je heap impact accepteert
3. Valideer input JSON en behandel ontbrekende velden defensief

## Netwerk en async
1. Bij ESPAsyncWebServer: geen blocking calls in callbacks
2. Protect shared state tussen callbacks en loop taak, gebruik FreeRTOS primitives alleen als nodig
3. WiFi reconnect gedrag moet stabiel blijven, voorkom tight reconnect loops

## Timing en state machines
1. Gebruik `millis` gebaseerde timers, geen `delay` in normale flow
2. State machines moeten expliciet zijn met duidelijke transitions
3. Timeouts moeten configureerbaar zijn als er al een config patroon bestaat

## Bestandsstructuur verwachtingen
1. Broncode meestal in `src`
2. Config en board settings in platformio.ini
3. Eventuele unit tests in `test` als aanwezig
4. Documentatie in README of docs map als aanwezig

## Wijzigingen scope
1. Minimal change principle: pas alleen aan wat nodig is voor de gevraagde functionaliteit of bugfix
2. Refactors alleen als ze direct nodig zijn voor correctheid of testbaarheid
3. Geen grote herstructurering zonder expliciete opdracht

## Checklist voor oplevering
1. `pio run` slaagt
2. Geen nieuwe warnings die op regressie wijzen, behandel warnings waar praktisch
3. Nieuwe functies hebben duidelijke interface en worden gebruikt
4. Belangrijke randgevallen zijn afgevangen, vooral null pointers, buffer grenzen, JSON parse errors
5. Documenteer nieuwe configuratie opties in README of in een relevante docs sectie

## Wat je aan mij terugrapporteert
1. Welke files zijn gewijzigd
2. Waarom de wijziging nodig was
3. Welke build command is relevant en of het zou moeten slagen gegeven de repo configuratie
4. Eventuele resterende risico's of aannames, expliciet gelabeld als aannames
