---
title: TempoFlow Preset Format 1.0
aliases:
  - TempoFlow Preset Specification
  - TempoFlow Preset Schema
project: TempoFlow
created-by: ChatGPT
created-at: 2026-09-06T21:30:00+02:00
updated:
  - by: ChatGPT
    at: 2026-09-06T21:30:00+02:00
    comment: Initial creation
  - by: ChatGPT
    at: 2026-09-06T21:30:00+02:00
    comment: Approved architecture decisions; grouping, audio, host mode and output behavior finalized
  - by: Joerg Heinrich
    at: 2026-09-07T00:28:00+02:00
    comment: complete task 3B and 3C
version: 1.1.2
status: approved
tags:
  - tempoflow
  - metronome
  - vst3
  - cubase
  - preset
  - specification
  - audio
sources:
  - "TempoFlow Base44 App screenshots supplied by Jörg"
  - "TempoFlow architecture decisions from current ChatGPT conversation"
github-managed: false
github-repository:
---

# TempoFlow Preset Format 1.0 — Approved Specification

## 1. Ziel

Das **TempoFlow Preset Format** definiert eine gemeinsame, plattformunabhängige Beschreibung eines Metronom-Setups.

Es soll mindestens von folgenden Anwendungen verwendet werden können:

- TempoFlow PWA
- TempoFlow VST3
- zukünftige Standalone-Anwendung
- zukünftige Mobile-/Desktop-Version
- zukünftige Preset-Bibliotheken

Das Preset darf nicht von Base44, Cubase oder einer bestimmten Programmiersprache abhängig sein.

---

## 2. Architekturentscheidung

TempoFlow wird logisch in vier Bereiche getrennt:

```text
TempoFlow
│
├── Musical Model
├── Preset Model
├── Playback Engine
└── User Interface
```

Das Preset beschreibt ausschließlich:

- musikalische Parameter
- Klangrollen
- Wiedergabeeinstellungen
- Übungseinstellungen

Nicht Bestandteil eines Presets sind:

- konkrete UI-Zustände
- Fenstergröße
- Cubase-Projektposition
- Browser-Zustände
- interne Audio-Device-Konfiguration
- ASIO-Konfiguration

---

## 3. Dateiformat

Dateiendung:

```text
.tempoflow
```

Technisches Format:

```text
UTF-8 JSON
```

Beispiel:

```text
Slow Blues Shuffle.tempoflow
```

Eine Datei muss gültiges JSON enthalten.

---

## 4. Root-Struktur

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

---

## 5. Vollständiges Beispiel

```json
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",

  "metadata": {
    "name": "Slow Blues Shuffle",
    "description": "Langsames Blues-Shuffle-Übungspreset",
    "category": "blues",
    "tags": [
      "blues",
      "shuffle",
      "practice"
    ],
    "author": "Jörg",
    "createdAt": "2026-09-06T21:30:00+02:00",
    "updatedAt": "2026-09-06T21:30:00+02:00"
  },

  "tempo": {
    "bpm": 72
  },

  "meter": {
    "numerator": 4,
    "denominator": 4
  },

  "subdivision": {
    "mode": "triplet",
    "partsPerBeat": 3
  },

  "pattern": {
    "beats": [
      {
        "beat": 1,
        "click": "accent"
      },
      {
        "beat": 2,
        "click": "normal"
      },
      {
        "beat": 3,
        "click": "normal"
      },
      {
        "beat": 4,
        "click": "normal"
      }
    ]
  },

  "sound": {
    "soundSet": "default",
    "volume": 0.8
  },

  "playback": {
    "mode": "internal"
  }
}
```

---

## 6. Schema-Version

Pflichtfeld:

```json
"schemaVersion": "1.0.0"
```

Es gilt Semantic Versioning.

### PATCH

Abwärtskompatible Korrekturen.

Beispiel:

```text
1.0.0 → 1.0.1
```

### MINOR

Neue optionale Felder.

Beispiel:

```text
1.0.0 → 1.1.0
```

### MAJOR

Nicht abwärtskompatible Änderung.

Beispiel:

```text
1.x → 2.0.0
```

---

## 7. Metadata

```json
"metadata": {
  "name": "Standard 4/4",
  "description": "",
  "category": "general",
  "tags": [],
  "author": "",
  "createdAt": "",
  "updatedAt": ""
}
```

### Pflichtfelder

```text
name
```

### Optionale Felder

```text
description
category
tags
author
createdAt
updatedAt
```

---

## 8. Tempo-Modell

```json
"tempo": {
  "bpm": 120
}
```

### Bereich

Aktueller TempoFlow-Bereich:

```text
20–300 BPM
```

Validierung:

```text
bpm >= 20
bpm <= 300
```

Intern sollte `bpm` als Fließkommazahl zulässig sein.

Beispiel:

```json
"bpm": 120.5
```

Die aktuelle UI darf weiterhin nur Ganzzahlen anzeigen.

Damit bleibt das Format für spätere professionelle Anwendungen offen.

---

## 9. Taktart

Das Meter-Modell besteht verbindlich aus:

- `numerator`
- `denominator`
- `grouping`

Beispiel 4/4:

```json
"meter": {
  "numerator": 4,
  "denominator": 4,
  "grouping": [1, 1, 1, 1]
}
```

Beispiel 7/8:

```json
"meter": {
  "numerator": 7,
  "denominator": 8,
  "grouping": [2, 2, 3]
}
```

Beispiele unterstützter Taktarten:

```text
2/4
3/4
4/4
5/4
6/8
7/8
9/8
12/8
```

Die aktuelle TempoFlow-PWA bietet diese Werte direkt an.

Durch die getrennten Felder lassen sich später auch benutzerdefinierte Taktarten speichern.

Beispiel:

```json
"meter": {
  "numerator": 11,
  "denominator": 8,
  "grouping": [3, 3, 3, 2]
}
```

### Grouping-Regel

`grouping` beschreibt die musikalische Gruppierung innerhalb eines Taktes.

Beispiele:

```text
7/8 → [2, 2, 3]
7/8 → [3, 2, 2]
7/8 → [2, 3, 2]
```

Die Summe aller Werte in `grouping` muss dem `numerator` entsprechen.

Für einfache Taktarten darf eine neutrale Gruppierung verwendet werden, zum Beispiel:

```text
4/4 → [1, 1, 1, 1]
3/4 → [1, 1, 1]
```

---

## 10. Subdivision-Modell

Die heutige PWA kennt:

```text
normal
Triolen
```

Das Datenmodell wird bewusst allgemeiner gehalten.

```json
"subdivision": {
  "mode": "none",
  "partsPerBeat": 1
}
```

### Unterstützte Werte V1

#### Kein Subdivision-Click

```json
{
  "mode": "none",
  "partsPerBeat": 1
}
```

#### Triolen

```json
{
  "mode": "triplet",
  "partsPerBeat": 3
}
```

---

## 11. Reservierte zukünftige Subdivisions

Noch nicht zwingend in V1 zu implementieren:

```text
eighth
sixteenth
triplet
shuffle
custom
```

Beispiel:

```json
{
  "mode": "sixteenth",
  "partsPerBeat": 4
}
```

Wichtig:

Die derzeitige Einstellung

```text
Triolen = true/false
```

wird nicht direkt im Format gespeichert.

Stattdessen:

```text
subdivision.mode
```

Dadurch bleibt das Schema erweiterbar.

---

## 12. ClickType-Modell

Die sechs bestehenden Klicktypen aus TempoFlow werden vollständig übernommen.

### Gültige Werte

```text
accent
normal
high
low
wood
mute
```

Zuordnung UI:

| Interner Wert | UI |
|---|---|
| `accent` | Akzent |
| `normal` | Normal |
| `high` | Hoch |
| `low` | Tief |
| `wood` | Holz |
| `mute` | Stumm |

---

## 13. ClickType ist eine Klangrolle

Ein ClickType ist keine konkrete WAV-Datei.

Beispiel:

```json
"click": "wood"
```

bedeutet:

> Verwende den Wood-Click des aktuell gewählten SoundSets.

Nicht:

```text
spiele wood.wav
```

Damit bleiben Pattern und Soundbibliothek getrennt.

---

## 14. Pattern-Modell

Ein einfaches 4/4-Muster:

```json
"pattern": {
  "beats": [
    {
      "beat": 1,
      "click": "accent"
    },
    {
      "beat": 2,
      "click": "normal"
    },
    {
      "beat": 3,
      "click": "normal"
    },
    {
      "beat": 4,
      "click": "normal"
    }
  ]
}
```

---

## 15. Beispiele für Beat-Patterns

### Standard 4/4

```text
1   2   3   4
A   N   N   N
```

```json
[
  { "beat": 1, "click": "accent" },
  { "beat": 2, "click": "normal" },
  { "beat": 3, "click": "normal" },
  { "beat": 4, "click": "normal" }
]
```

### Rock Backbeat

```text
1   2   3   4
L   H   L   H
```

```json
[
  { "beat": 1, "click": "low" },
  { "beat": 2, "click": "high" },
  { "beat": 3, "click": "low" },
  { "beat": 4, "click": "high" }
]
```

### Nur Beat 1

```text
1   2   3   4
A   -   -   -
```

```json
[
  { "beat": 1, "click": "accent" },
  { "beat": 2, "click": "mute" },
  { "beat": 3, "click": "mute" },
  { "beat": 4, "click": "mute" }
]
```

### Beats 2 und 4

```text
1   2   3   4
-   W   -   W
```

```json
[
  { "beat": 1, "click": "mute" },
  { "beat": 2, "click": "wood" },
  { "beat": 3, "click": "mute" },
  { "beat": 4, "click": "wood" }
]
```

---

## 16. SoundSet-Modell

```json
"sound": {
  "soundSet": "default",
  "volume": 0.8
}
```

`volume`:

```text
0.0 = stumm
1.0 = volle Lautstärke
```

Die derzeitige Anzeige

```text
80 %
```

entspricht:

```json
"volume": 0.8
```

---

## 17. SoundSet

Ein SoundSet ordnet Klangrollen konkreten Sounds zu.

### Verbindliche Audio-Strategie

TempoFlow bevorzugt geeignete frei nutzbare Samples für die benötigten Klangrollen, sofern:

- die Lizenz eine Weitergabe im Plugin erlaubt,
- die Quelle nachvollziehbar dokumentiert werden kann,
- die Samples qualitativ und technisch geeignet sind.

Wenn für eine Klangrolle kein geeignetes frei nutzbares Sample verfügbar ist, wird der entsprechende Klang synthetisch erzeugt.

Damit gilt:

```text
freie, rechtssicher nutzbare Samples → bevorzugt
keine geeigneten Samples verfügbar → synthetische Erzeugung
```

Konzeptionelles Beispiel für ein Sample-basiertes SoundSet:

```json
{
  "id": "default",
  "name": "TempoFlow Default",

  "sounds": {
    "accent": "accent.wav",
    "normal": "normal.wav",
    "high": "high.wav",
    "low": "low.wav",
    "wood": "wood.wav"
  }
}
```

`mute` benötigt keine Audioressource.

Die konkrete Sample-Lizenzierung ist Bestandteil der späteren Implementierungs- und Release-Dokumentation.

---

## 18. SoundSet und Preset bleiben getrennt

Ein Preset enthält:

```json
"click": "wood"
```

Das SoundSet entscheidet:

```text
welcher konkrete Wood-Sound gespielt wird
```

Dadurch kann ein Pattern mit mehreren Klangsets verwendet werden.

Beispiel:

```text
Slow Blues Shuffle
        │
        ├── Classic Click
        ├── Woodblock
        ├── Studio
        └── Soft Practice
```

---

## 19. Playback-Modell

```json
"playback": {
  "mode": "internal"
}
```

Unterstützte Modi:

```text
internal
host
```

### Audio-Architektur

Für TempoFlow VST3 gilt verbindlich:

```text
interne Audioverarbeitung: Mono
Plugin-Hauptausgang: Mono
zusätzliche Audioausgänge: architektonisch vorsehen
```

Mehrere Ausgänge müssen im ersten MVP noch nicht vollständig genutzt werden, die Architektur darf sie aber nicht verhindern.

Einzelne ClickTypes erhalten in V1 keine eigene Lautstärkeregelung. Die Lautstärke wird zentral über den Master-/Preset-Wert gesteuert.

---

## 20. Internal Mode

Im Internal Mode kontrolliert TempoFlow:

```text
Tempo
Start
Stop
Takt
Subdivision
Pattern
```

Dieser Modus entspricht dem heutigen Verhalten der PWA.

---

## 21. Host Mode

Wenn TempoFlow als VST3 in einer DAW geladen wird, wird der Host Mode automatisch aktiviert.

Der Host kontrolliert:

```text
Tempo
Transport
Position
Taktart
```

Beispiel:

```text
Cubase
   │
   ├── Tempo
   ├── Taktart
   ├── Start / Stop
   ├── Song Position
   └── Sample Position
        │
        ▼
TempoFlow VST3
```

TempoFlow kontrolliert weiterhin:

```text
ClickType
Pattern
SoundSet
Lautstärke
Subdivision-Verhalten
```

TempoFlow ersetzt das Cubase-Metronom nicht technisch. Beide können parallel verwendet werden.

Empfehlung für den praktischen Einsatz:

```text
TempoFlow aktiv
Cubase-Metronom optional aktiv oder deaktiviert
```

Die Entscheidung liegt beim Nutzer bzw. beim jeweiligen Cubase-Projekt.

---

## 22. Wichtige VST-Regel

Im Host Mode darf TempoFlow keine eigene Zeitbasis verwenden.

Nicht:

```text
JavaScript Timer
setInterval
Browser Clock
```

sondern:

```text
Host Transport
+
Sample Position
+
Audio Buffer Position
```

Ziel:

```text
samplegenaues Timing
```

---

## 23. Preset und Host-Tempo

Ein Preset darf einen BPM-Wert enthalten.

Im Host Mode gilt aber:

```text
Cubase BPM > Preset BPM
```

Das Preset-Tempo dient dann lediglich als:

- Ausgangswert
- Referenz
- Internal-Mode-Wert

---

## 24. Verhalten beim Wechsel in Host Mode

Beispiel:

Preset:

```text
72 BPM
4/4
```

Cubase:

```text
96 BPM
6/8
```

Im Host Mode verwendet TempoFlow:

```text
96 BPM
6/8
```

Das gespeicherte Preset bleibt unverändert.

---

## 25. Preset-Kategorien

Empfohlene Kategorien:

```text
general
practice
blues
rock
jazz
funk
metal
custom
```

Die Liste darf erweitert werden.

---

## 26. Beispiel-Preset-Bibliothek

```text
Presets/
│
├── General/
│   ├── Standard 4-4.tempoflow
│   ├── Standard 3-4.tempoflow
│   └── Standard 6-8.tempoflow
│
├── Blues/
│   ├── Slow Blues.tempoflow
│   ├── Texas Shuffle.tempoflow
│   └── 12-8 Blues.tempoflow
│
├── Rock/
│   ├── Straight Rock.tempoflow
│   └── Backbeat.tempoflow
│
└── Practice/
    ├── Beat 1 Only.tempoflow
    ├── Beats 2 and 4.tempoflow
    └── Silent Beats.tempoflow
```

---

## 27. Validierungsregeln V1

### Schema

Muss sein:

```json
"schema": "tempoflow-preset"
```

### Schema-Version

Pflichtfeld.

### BPM

```text
20 <= bpm <= 300
```

### Meter

```text
numerator >= 1
denominator > 0
```

Für V1 zulässige Nenner:

```text
2
4
8
16
```

`grouping` ist Pflichtbestandteil des Meter-Modells.

Validierung:

```text
grouping.length >= 1
alle grouping-Werte > 0
sum(grouping) == numerator
```

### Beats

Die Anzahl der Beat-Einträge muss zum Zähler passen.

Beispiel:

```text
4/4 → 4 Beats
7/8 → 7 Beats
```

### ClickType

Nur:

```text
accent
normal
high
low
wood
mute
```

### Volume

```text
0.0 <= volume <= 1.0
```

---

## 28. Fehlerverhalten

Unbekannte optionale Felder sollen ignoriert werden.

Beispiel:

```json
{
  "futureFeature": true
}
```

Eine V1-Anwendung soll das Preset trotzdem öffnen können.

Unbekannte Pflichtwerte müssen dagegen einen Fehler erzeugen.

Beispiel:

```json
"click": "laser"
```

Ergebnis:

```text
Unsupported ClickType: laser
```

---

## 29. Zukunft: Subdivision-Pattern

Die aktuelle V1-Struktur beschreibt primär Hauptschläge.

Für eine spätere Version ist folgende Erweiterung vorgesehen:

```json
{
  "beat": 1,
  "click": "accent",

  "subdivisions": [
    {
      "position": 1,
      "click": "accent"
    },
    {
      "position": 2,
      "click": "mute"
    },
    {
      "position": 3,
      "click": "wood"
    }
  ]
}
```

Damit ließen sich unter anderem:

- Shuffle
- 12/8-Patterns
- komplexe Triolen
- Ghost-Clicks

abbilden.

Diese Erweiterung sollte frühestens mit Schema `1.1.0` eingeführt werden.

---

## 30. Zukunft: Silent Bar Trainer

Vorgesehen, aber nicht Teil des minimalen V1-Kerns.

Beispiel:

```json
"trainer": {
  "enabled": true,

  "cycle": {
    "clickBars": 2,
    "silentBars": 2
  }
}
```

Ergebnis:

```text
Takt 1  Click
Takt 2  Click
Takt 3  Silent
Takt 4  Silent
Takt 5  Click
...
```

---

## 31. Zukunft: Tempo Trainer

Mögliche spätere Erweiterung:

```json
"tempoTrainer": {
  "enabled": true,
  "startBpm": 80,
  "targetBpm": 120,
  "increment": 2,
  "barsPerStep": 4
}
```

Damit ließe sich beispielsweise:

```text
80 BPM  × 4 Takte
82 BPM  × 4 Takte
84 BPM  × 4 Takte
...
120 BPM
```

automatisieren.

---

## 32. Zukunft: SoundSets

Mögliche Soundbibliothek:

```text
SoundSets/
│
├── Default/
├── Classic/
├── Woodblock/
├── Studio/
├── Soft Practice/
└── Custom/
```

Custom SoundSets könnten später eigene WAV-Dateien enthalten.

---

## 33. Nicht Bestandteil von Preset 1.0

Bewusst nicht gespeichert werden:

```text
ASIO-Gerät
Audiointerface
Cubase Bus
Sample Rate
Buffer Size
Fensterposition
Fenstergröße
Theme
PWA Installationsstatus
Browser
Base44 Benutzerinformationen
```

Diese Informationen gehören zur jeweiligen Anwendung.

---

## 34. TempoFlow Core

Das gemeinsame fachliche Modell lautet:

```text
TempoFlow Core
│
├── Tempo
├── Meter
├── Subdivision
├── Pattern
│   └── ClickType
├── SoundSet
├── Volume
└── Playback Mode
```

Implementierung:

```text
PWA:
JavaScript / TypeScript

VST3:
C++ / JUCE
```

Es wird nicht versucht, denselben Programmcode zwischen JavaScript und C++ zu teilen.

Geteilt werden:

```text
Schema
Semantik
Validierungsregeln
Preset-Dateien
```

---

## 35. Empfohlene Implementierungsreihenfolge

### Phase 3A

Preset Schema definieren.

Status:

```text
✅ Entwurf vorhanden
```

### Phase 3B

JSON Schema [[tempoflow-preset.schema.json|Tempoflow Preset Schema 1.0]] erstellt.

Damit können Presets automatisch validiert werden.

Status:

```text
✅ Schema erstellt
```

### Phase 3C

Referenz-Presets erstellt:
- [[Standard 4-4.tempoflow|Standard 4/4]]
- [[Standard 3-4.tempoflow|Standard 3/4]]
- [[Standard 6-8.tempoflow|Standard 6/8]]
- [[Beat 1 Only.tempoflow|Beat 1 Only]]
- [[Beats 2 + 4.tempoflow|Beats 2 + 4]]
- [[Slow Blues Shuffle.tempoflow|Slow Blues Shuffle]]
- [[12-8 Blues.tempoflow|12/8 Blues]]

Status:

```text
✅ Presets erstellt
```

### Phase 4

VST3-Prototyp.

Technologie:

```text
C++
JUCE
VST3
```

Minimaler Funktionsumfang:

```text
Cubase lädt Plugin
Host-Tempo wird erkannt
Host-Taktart wird erkannt
Transport wird erkannt
Click wird samplegenau erzeugt
Preset kann geladen werden
```

Noch keine aufwendige GUI.

---

## 36. Definition MVP VST3

Der erste belastbare TempoFlow-VST-Prototyp muss:

- in Cubase Elements 15 geladen werden
- Audio erzeugen
- Cubase-Transport verfolgen
- Cubase-Tempo verfolgen
- Cubase-Taktart verfolgen
- Beat 1 korrekt erkennen
- ClickTypes unterstützen:
  - accent
  - normal
  - high
  - low
  - wood
  - mute
- `.tempoflow` Presets laden
- Lautstärke unterstützen

Nicht erforderlich für MVP:

- Cloud Sync
- Base44-Anbindung
- Preset Editor
- Silent Bar Trainer
- eigene Samples
- mobile Integration
- aufwendige Animationen

---

## 37. Technisches Zielbild

```text
                         ┌──────────────────┐
                         │   .tempoflow     │
                         │     Preset       │
                         └────────┬─────────┘
                                  │
                   ┌──────────────┴──────────────┐
                   │                             │
                   ▼                             ▼
            TempoFlow PWA                 TempoFlow VST3
            Base44 / Web                     JUCE / C++
                   │                             │
                   ▼                             ▼
              Web Audio                    Cubase Host
                                                 │
                                                 ▼
                                         ASIO Audio Engine
```

---

## 38. Entscheidungsliste

### Akzeptiert

- `.tempoflow` als eigenes Presetformat
- JSON als interne Repräsentation
- plattformunabhängiges Presetmodell
- Trennung von Pattern und SoundSet
- sechs bestehende ClickTypes:
  - accent
  - normal
  - high
  - low
  - wood
  - mute
- eigener Host-Modus für DAW-Betrieb
- Host Mode wird in einer DAW automatisch aktiviert
- Cubase ist im Host Mode der Timing-Master
- Cubase-Metronom und TempoFlow dürfen parallel verwendet werden
- JUCE/C++ für VST3
- keine Browser-/ASIO-Bridge-Lösung
- keine direkte Base44-Abhängigkeit im VST
- Schema-Versionierung mit Semantic Versioning
- Audioverarbeitung in Mono
- Hauptausgang in Mono
- zusätzliche Audioausgänge werden architektonisch vorgesehen
- keine individuelle Lautstärke pro ClickType in V1
- Meter-Modell enthält verbindlich `grouping`
- geeignete frei nutzbare Samples werden bevorzugt
- fehlen geeignete frei nutzbare Samples, werden die Klicksounds synthetisch erzeugt

---

## 39. Finalisierte Architekturentscheidungen

### 39.1 Klangquellen

TempoFlow verwendet bevorzugt frei nutzbare Samples, wenn deren Lizenz eine rechtssichere Weitergabe mit dem Plugin erlaubt.

Falls für einen benötigten ClickType kein geeignetes Sample verfügbar ist, wird der Klang synthetisch erzeugt.

Die Entscheidung wird pro Klangrolle getroffen.

### 39.2 Kanalformat

TempoFlow arbeitet intern in Mono.

Der primäre Plugin-Ausgang ist Mono.

### 39.3 Zusätzliche Audioausgänge

Mehrere Ausgänge werden in der Architektur vorgesehen.

Sie sind nicht zwingender Bestandteil des ersten MVP, dürfen durch die Architektur aber nicht ausgeschlossen werden.

### 39.4 Lautstärke pro ClickType

In V1 gibt es keine individuelle Lautstärkeregelung pro ClickType.

Es gilt ein zentraler Lautstärkewert über:

```text
sound.volume
```

### 39.5 Host Mode

Wenn TempoFlow als VST3 in Cubase oder einer anderen kompatiblen DAW geladen wird, wird der Host Mode automatisch aktiviert.

### 39.6 Cubase-Metronom

TempoFlow darf parallel zum Cubase-Metronom betrieben werden.

TempoFlow deaktiviert oder ersetzt das Cubase-Metronom nicht automatisch.

### 39.7 Meter Grouping

Das Meter-Modell enthält verbindlich:

```json
"meter": {
  "numerator": 7,
  "denominator": 8,
  "grouping": [2, 2, 3]
}
```

`grouping` ist damit Bestandteil von Preset Format 1.0.

---

## 40. Meter- und Grouping-Modell

Die Gruppierung ist für zusammengesetzte und ungerade Taktarten musikalisch relevant.

Beispiele:

```text
7/8 → 2 + 2 + 3
7/8 → 3 + 2 + 2
7/8 → 2 + 3 + 2
```

Diese Varianten sind musikalisch nicht identisch und müssen von TempoFlow unterschieden werden können.

Beispiel:

```json
{
  "meter": {
    "numerator": 7,
    "denominator": 8,
    "grouping": [2, 2, 3]
  }
}
```

### Validierung

Es gilt:

```text
sum(grouping) == numerator
```

Beispiele:

```text
[2, 2, 3] → gültig für 7/8
[3, 2, 2] → gültig für 7/8
[2, 3, 2] → gültig für 7/8
[2, 2]    → ungültig für 7/8
```

Damit ist das Meter-Modell für Preset Format 1.0 festgelegt.

---

## 41. Status

**TempoFlow Preset Format 1.0 ist fachlich freigegeben.**

Status:

```text
approved
```

Dokumentversion:

```text
1.1.0
```

Die folgenden Architekturentscheidungen sind abgeschlossen:

1. Beat-Gruppierung über `meter.grouping`
2. Subdivision-Modell
3. Sound-Erzeugung: freie Samples bevorzugt, synthetische Fallbacks
4. Host-/Internal-Verhalten
5. Mono-Audioarchitektur
6. optionale zukünftige Mehrfachausgänge
7. paralleler Betrieb mit Cubase-Metronom

Als nächster technischer Schritt folgt:

```text
Phase 3B
→ formales JSON Schema
→ tempoflow-preset.schema.json
```

Danach:

```text
Phase 3C
→ Referenz-Presets
```

und anschließend:

```text
Phase 4
→ TempoFlow VST3 MVP
→ C++ / JUCE / VST3
```
