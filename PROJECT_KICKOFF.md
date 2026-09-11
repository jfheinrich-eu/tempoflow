---
title: TempoFlow – Projekt-Kickoff
aliases:
  - TempoFlow Kickoff
project: TempoFlow
status: discovery
updated: 2026-09-11
tags:
  - tempoflow
  - metronome
  - base44
  - pwa
  - vst3
  - cubase
  - preset
---

# TempoFlow – Projekt-Kickoff

## Kurzfassung

TempoFlow ist ein plattformübergreifendes Metronom-System. Es besteht zunächst aus:

1. einer Base44-PWA für den eigenständigen Betrieb im Browser,
2. einem nativen VST3-Plugin für den synchronen Betrieb in einer DAW,
3. dem gemeinsamen, plattformunabhängigen `.tempoflow`-Presetformat.

PWA und VST3 teilen nicht denselben Programmcode. Sie teilen Preset-Schema, musikalische Semantik, Validierungsregeln und Preset-Dateien.

Der erste technische Schwerpunkt ist ein schlankes VST3-MVP für Cubase Elements 15. Es folgt dem Host samplegenau, erzeugt Metronom-Audio und lädt die bereits definierten `.tempoflow`-Presets. Eine direkte Cloud- oder Base44-Anbindung gehört ausdrücklich nicht zum MVP.

## Produktidee

TempoFlow stellt Metronom-Setups unabhängig von Oberfläche und Laufzeitumgebung bereit.

Ein Setup beschreibt:

- Tempo,
- Taktart und musikalische Gruppierung,
- Subdivision,
- Schlagmuster,
- Klangrollen,
- SoundSet,
- Gesamtlautstärke,
- Wiedergabemodus.

Dasselbe Preset soll mindestens in der PWA und im VST3 nutzbar sein. Spätere Standalone-, Mobile- oder Desktop-Versionen bleiben möglich.

## Zielgruppen und Nutzen

Die primäre Zielgruppe ist noch nicht ausdrücklich dokumentiert. Aus den vorhandenen Presets und Trainer-Ideen ergibt sich als Arbeitshypothese:

> TempoFlow richtet sich an Musiker, die flexible Metronom-Patterns für Übung und Produktion verwenden und zwischen Browser und DAW austauschen wollen.

Diese Zielgruppendefinition muss noch bestätigt werden. Die technische Produktidee ist dagegen bereits konkret.

## Bestand

### Base44-PWA

- Name: TempoFlow
- Base44-App-ID: `6a1fa5495a74e26d9d8fe055`
- Geplanter Betrieb: öffentlich
- Besucher-Login: nicht vorgesehen
- Der Base44-Login dient nur dem Betreiberzugriff über „Edit with Base44“.
- Im bisherigen Base44-Zugriff war nur das Benutzer-Modell mit `admin` und `user` sichtbar.
- Eigene Entity-Schemas waren nicht sichtbar.
- Die PWA bildet den Internal Mode ab und kontrolliert Tempo, Start, Stop, Takt, Subdivision und Pattern.
- Die PWA-Implementierung ist für JavaScript beziehungsweise TypeScript und Web Audio vorgesehen.

Noch offen ist, ob die PWA Daten clientseitig, in nicht sichtbaren Base44-Ressourcen oder über externe Dienste verarbeitet.

### Preset Format 1.0

Vorhanden sind:

- eine fachlich freigegebene Spezifikation,
- ein JSON Schema nach Draft 2020-12,
- sieben Referenz-Presets.

Hauptdokument:

- [[docs/Preset Format 1.0/Tempoflow Preset Format 1.0|TempoFlow Preset Format 1.0]]

Formales Schema:

- [[docs/Preset Format 1.0/tempoflow-preset.schema.json|TempoFlow Preset JSON Schema]]

Referenz-Presets:

- [[docs/Preset Format 1.0/presets/Standard 4-4.tempoflow|Standard 4/4]]
- [[docs/Preset Format 1.0/presets/Standard 3-4.tempoflow|Standard 3/4]]
- [[docs/Preset Format 1.0/presets/Standard 6-8.tempoflow|Standard 6/8]]
- [[docs/Preset Format 1.0/presets/Beat 1 Only.tempoflow|Beat 1 Only]]
- [[docs/Preset Format 1.0/presets/Beats 2 + 4.tempoflow|Beats 2 + 4]]
- [[docs/Preset Format 1.0/presets/Slow Blues Shuffle.tempoflow|Slow Blues Shuffle]]
- [[docs/Preset Format 1.0/presets/12-8 Blues.tempoflow|12/8 Blues]]

Alle sieben Dateien sind gültiges JSON und bestehen die dokumentierten semantischen Kernprüfungen zu BPM, Taktart, Gruppierung, Beat-Positionen, ClickTypes, Subdivision und Lautstärke.

### VST3

Der VST3-Prototyp ist spezifiziert, aber noch nicht implementiert.

- Technologie: C++ und JUCE
- Plugin-Format: VST3
- Referenz-Host: Cubase Elements 15
- Betriebsart in der DAW: automatisch Host Mode
- Audioverarbeitung: intern Mono
- Hauptausgang: Mono
- Zusätzliche Audioausgänge: architektonisch vorsehen, nicht im MVP umsetzen
- Timing: Host-Transport, Sample-Position und Audio-Buffer-Position
- Ziel: samplegenaue Click-Erzeugung

Eine Browser-/ASIO-Bridge und eine direkte Base44-Abhängigkeit im Plugin sind ausgeschlossen.

## Gemeinsames fachliches Modell

```text
TempoFlow Core
│
├── Tempo
├── Meter
│   └── Grouping
├── Subdivision
├── Pattern
│   └── ClickType
├── SoundSet
├── Volume
└── Playback Mode
```

Die vier technischen Bereiche bleiben getrennt:

```text
TempoFlow
│
├── Musical Model
├── Preset Model
├── Playback Engine
└── User Interface
```

## Presetformat

### Technische Form

- Dateiendung: `.tempoflow`
- Inhalt: UTF-8 JSON
- Schema-Kennung: `tempoflow-preset`
- aktuelle Schema-Version der Referenzdateien: `1.0.0`
- Versionierung: Semantic Versioning

### Root-Struktur

```json
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": {},
  "tempo": {},
  "meter": {},
  "subdivision": {},
  "pattern": {},
  "sound": {},
  "playback": {}
}
```

### Verbindliche Regeln

- BPM: `20` bis `300`, Fließkomma zulässig
- Takt-Nenner in V1: `2`, `4`, `8`, `16`
- `meter.grouping` ist Pflicht
- Summe von `grouping` entspricht `meter.numerator`
- Anzahl der Beats entspricht `meter.numerator`
- Beat-Positionen decken lückenlos `1..numerator` ab
- Lautstärke: `0.0` bis `1.0`
- Unbekannte optionale Felder werden ignoriert
- Nicht unterstützte Pflichtwerte erzeugen einen Fehler

### Subdivision V1

- `none` mit `partsPerBeat: 1`
- `triplet` mit `partsPerBeat: 3`

Weitere Subdivision-Typen sind reserviert, aber nicht Teil von V1.

### ClickTypes

```text
accent
normal
high
low
wood
mute
```

Ein ClickType ist eine Klangrolle und keine konkrete Audiodatei. Das gewählte SoundSet ordnet die Rolle einem Sample oder einem synthetisch erzeugten Klang zu.

### Nicht im Preset

Nicht gespeichert werden unter anderem:

- ASIO-Gerät,
- Audiointerface,
- Cubase-Bus,
- Sample Rate,
- Buffer Size,
- Fensterposition und Fenstergröße,
- UI-Theme,
- Browser- und PWA-Zustände,
- Base44-Benutzerinformationen.

Diese Werte gehören zur jeweiligen Anwendung und nicht zum musikalischen Preset.

## Wiedergabemodi

### Internal Mode

Die PWA beziehungsweise eine spätere Standalone-Anwendung kontrolliert:

- Tempo,
- Start und Stop,
- Takt,
- Subdivision,
- Pattern.

Der BPM-Wert des Presets ist aktiv.

### Host Mode

Beim Laden als VST3 wird der Host Mode automatisch aktiv.

Die DAW kontrolliert:

- Tempo,
- Transport,
- Song- und Sample-Position,
- Taktart.

TempoFlow kontrolliert weiterhin:

- ClickTypes,
- Pattern,
- SoundSet,
- Gesamtlautstärke,
- Subdivision-Verhalten.

Im Host Mode haben Host-Tempo und Host-Taktart Vorrang vor den Presetwerten. Das Preset selbst wird dadurch nicht verändert.

TempoFlow darf parallel zum Cubase-Metronom laufen. Es deaktiviert oder ersetzt das Cubase-Metronom nicht automatisch.

## Audio-Strategie

- Geeignete frei nutzbare Samples werden bevorzugt.
- Die Lizenz muss die Weitergabe mit dem Plugin erlauben.
- Quelle und Lizenz müssen für ein Release dokumentiert sein.
- Fehlt ein geeignetes Sample, wird der Klang synthetisch erzeugt.
- `mute` benötigt keine Audioressource.
- V1 besitzt nur eine zentrale Lautstärke über `sound.volume`.
- Einzelne ClickTypes erhalten in V1 keine eigene Lautstärkeregelung.

## Technisches Zielbild

```text
                         .tempoflow Preset
                                 │
                  ┌──────────────┴──────────────┐
                  │                             │
                  ▼                             ▼
          TempoFlow PWA                  TempoFlow VST3
       Base44 / TypeScript                 JUCE / C++
                  │                             │
                  ▼                             ▼
             Web Audio                    Cubase Host
                                                │
                                                ▼
                                        ASIO Audio Engine
```

Zwischen PWA und VST3 wird kein Quellcode geteilt. Gemeinsamer Vertrag sind Schema, Semantik, Validierung und Preset-Dateien.

## VST3-MVP

Der erste belastbare Prototyp muss:

- in Cubase Elements 15 als VST3 geladen werden,
- einen Mono-Hauptausgang bereitstellen,
- Audio erzeugen,
- Cubase-Transport verfolgen,
- Cubase-Tempo verfolgen,
- Cubase-Taktart verfolgen,
- Beat 1 korrekt und samplegenau erkennen,
- alle sechs ClickTypes unterstützen,
- `.tempoflow`-Presets laden und validieren,
- die zentrale Lautstärke anwenden.

Nicht Teil des MVP sind:

- Cloud-Synchronisation,
- direkte Base44-Anbindung,
- Preset-Editor im Plugin,
- Silent Bar Trainer,
- Tempo Trainer,
- benutzerdefinierte SoundSets,
- Mobile-Integration,
- aufwendige GUI und Animationen,
- vollständig nutzbare Mehrfachausgänge.

## Base44-PWA und Veröffentlichung

Für die öffentliche PWA gilt der vereinbarte Minimalstandard:

- Impressum,
- Datenschutzerklärung,
- zentrale Kontaktadresse,
- Betreiberanschrift über einen geeigneten Impressumsdienst,
- dokumentierte Datenflüsse und externe Dienste.

Der „Edit with Base44“-Button gilt als niedriges technisches Risiko und als Branding-Thema:

- Entfernung nach geplantem Tarifwechsel
- Priorität P3
- kein Tarifwechsel allein wegen dieses Buttons

Tarife, Preise, GitHub-Synchronisation und externer Agentenzugriff müssen vor einer Buchung aktuell geprüft werden.

## Abgeschlossene Entscheidungen

- `.tempoflow` ist das gemeinsame Presetformat.
- JSON ist die interne Repräsentation.
- Das Presetmodell bleibt plattformunabhängig.
- Pattern und SoundSet sind getrennt.
- `meter.grouping` ist verbindlich.
- PWA und VST3 teilen keine Implementierung.
- VST3 wird mit C++ und JUCE entwickelt.
- VST3 nutzt Host-Timing statt einer eigenen Zeitbasis.
- Cubase ist im Host Mode Timing-Master.
- Host-Tempo und Host-Taktart überschreiben Presetwerte nur zur Laufzeit.
- Audioverarbeitung und Hauptausgang sind Mono.
- Zusätzliche Ausgänge bleiben architektonisch möglich.
- Base44 ist keine Laufzeitabhängigkeit des VST3.
- Eine Browser-/ASIO-Bridge wird nicht gebaut.
- Rechtssicher weitergebbare Samples werden bevorzugt, Synthese ist der Fallback.

## Offene Entscheidungen

### Produkt

- Primäre Zielgruppe verbindlich bestätigen
- wichtigste drei Nutzerabläufe definieren
- klare Abgrenzung zu bestehenden DAW-Metronomen formulieren
- Nutzen des Preset-Austauschs zwischen PWA und VST3 konkretisieren

### VST3

- JUCE- und Toolchain-Version festlegen
- unterstützte Windows-Versionen definieren
- Plugin-ID, Herstellerkennung und Produktkennung festlegen
- Preset-Suchpfade und Installationsort festlegen
- Verhalten bei ungültigen und inkompatiblen Presets spezifizieren
- synthetische MVP-Klicksounds oder lizenzierte Samples auswählen
- Strategie für Host-Taktartwechsel innerhalb eines Projekts definieren
- Testmatrix für Buffer-Größen, Sample Rates und Tempoänderungen erstellen

### Base44

- Datenhaltung und sichtbare beziehungsweise versteckte Ressourcen inventarisieren
- Local Storage, Cookies, Analytics und externe Requests prüfen
- Import und Export von `.tempoflow`-Dateien in der PWA verifizieren
- Impressum und Datenschutz an die tatsächlichen Datenflüsse anpassen
- Betreiber- und Admin-Routen auf öffentliche Erreichbarkeit prüfen

### Presetformat

- Migrationsstrategie für spätere Schema-Versionen definieren
- Fehlercodes und nutzerverständliche Fehlermeldungen festlegen
- semantische Validierung als gemeinsame Testsuite formulieren
- Verhalten bei unbekannten Major-Versionen festlegen

## Dokumentationsstatus

Die Spezifikation wurde auf Dokumentversion `1.1.2` konsolidiert:

- Das vollständige Beispiel enthält das verbindliche `meter.grouping`.
- Die Phasen 3A, 3B und 3C sind als abgeschlossen dokumentiert.
- Das JSON Schema und die sieben Referenz-Presets bilden den freigegebenen Stand ab.
- Phase 4, das TempoFlow VST3 MVP, ist der nächste technische Schritt.

„Preset Format 1.0“ bezeichnet die fachliche Formatgeneration. `1.1.2` bezeichnet die Version des Spezifikationsdokuments. Die Referenz-Presets verwenden Schema-Version `1.0.0`.

## Risiken

### P1 – Echtzeit- und Host-Synchronisation

Transport, Taktartwechsel, Tempoänderungen und Buffer-Grenzen müssen ohne Drift oder Doppeltrigger verarbeitet werden.

### P1 – Semantische Presetvalidierung

Das JSON Schema kann Summen und Beziehungen zwischen Geschwisterfeldern nicht vollständig prüfen. Anwendungen müssen unter anderem Grouping-Summe, Beat-Anzahl und Beat-Positionen zusätzlich validieren.

### P1 – Ungeklärte PWA-Datenflüsse

Die Base44-App ist noch nicht vollständig inventarisiert. Datenschutz- und Sicherheitsstatus bleiben deshalb vorläufig.

### P2 – Sound-Lizenzen

Unklare Sample-Lizenzen können eine Veröffentlichung verhindern. Synthetische Sounds sind der sichere MVP-Fallback.

### P3 – Base44-Branding

Der sichtbare „Edit with Base44“-Button wirkt unprofessionell, ist aber kein akutes Sicherheitsproblem.

## Umsetzungsabschnitte

### Abschnitt A – Restarbeiten an der Spezifikation

- Schema und semantische Regeln als verbindliche Testfälle festhalten
- MVP-Soundquelle entscheiden

### Abschnitt B – VST3-Grundgerüst

- JUCE-Projekt anlegen
- VST3-Metadaten definieren
- Mono-Bus konfigurieren
- Plugin in Cubase Elements 15 laden

### Abschnitt C – Host-Synchronisation

- Transport lesen
- Tempo lesen
- Taktart lesen
- Sample- und Buffer-Position verarbeiten
- Beat- und Taktgrenzen samplegenau bestimmen

### Abschnitt D – Audio und Pattern

- Click-Engine implementieren
- sechs ClickTypes abbilden
- Master-Lautstärke anwenden
- Meter-Grouping und Triplet-Subdivision verarbeiten

### Abschnitt E – Presets

- `.tempoflow` laden
- strukturell und semantisch validieren
- Fehler robust melden
- sieben Referenz-Presets als Integrationstests verwenden

### Abschnitt F – PWA-Abgleich

- Preset-Import und -Export prüfen
- gleiche Semantik mit denselben Referenzdateien testen
- Base44-Datenflüsse und Veröffentlichungsanforderungen prüfen

### Abschnitt G – Release-Vorbereitung

- Installations- und Presetpfade dokumentieren
- Lizenznachweise für Samples und Abhängigkeiten erstellen
- Testmatrix ausführen
- Impressum, Datenschutz und Downloadseite fertigstellen

## Definition of Done für den VST3-Prototyp

Der Prototyp gilt als erfolgreich, wenn:

- Cubase Elements 15 das Plugin zuverlässig lädt,
- Start, Stop, Position, Tempo und Taktart korrekt erkannt werden,
- der Click über mehrere Buffer-Größen hinweg samplegenau bleibt,
- alle sechs ClickTypes hörbar beziehungsweise stumm korrekt funktionieren,
- alle sieben Referenz-Presets geladen werden,
- ungültige Presets kontrolliert abgewiesen werden,
- Preset-Tempo und -Taktart im Host Mode nicht den Host überschreiben,
- ein erneutes Öffnen des Cubase-Projekts einen konsistenten Zustand herstellt,
- keine Base44- oder Netzwerkverbindung für den Kernbetrieb erforderlich ist.

## Nächster sinnvoller Schritt

Vor dem JUCE-Grundgerüst sollte die Spezifikation kurz konsolidiert werden. Danach folgt unmittelbar das VST3-Projekt mit Mono-Ausgang, Host-Transport und einer zunächst synthetischen Click-Engine.

Die sieben vorhandenen Presets werden von Beginn an als ausführbare Akzeptanztests behandelt. So bleibt das gemeinsame Format der verbindliche Vertrag zwischen PWA und VST3.
