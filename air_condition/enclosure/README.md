# Air Condition (Nadajnik) - Obudowa 3D

## Wersja 1.2

Wymiary oparte na pomiarach elektroniki.

## Pliki

| Plik | Opis |
|------|------|
| `air_condition_case.scad` | Źródło parametryczne OpenSCAD |
| `case_body.stl` | Korpus obudowy (STL do druku) |
| `case_body.gcode` | Korpus - G-code dla Ender-3 Pro |
| `case_cover.stl` | Tylna pokrywa (STL do druku) |
| `case_cover.gcode` | Pokrywa - G-code dla Ender-3 Pro |
| `ender3_config.ini` | Profil drukarki |

## Wymiary

### Zewnętrzne
- **61 × 86 × 55 mm** (szerokość × wysokość × głębokość)
- Grubość ścianki: 2 mm

### Wewnętrzne
- Szerokość: 57 mm
- Wysokość: 82 mm
- Głębokość: 49 mm (w tym 1mm wgłębienie na plexi)

### Okno OLED (patrząc z zewnątrz na przód)
- **Otwór widoczny: 55 × 30 mm**
- Pozycja: 1mm od każdej krawędzi bocznej, 4mm od góry
- Wgłębienie na plexi: 57 × 32 × 1 mm

### USB (prawa ściana - patrząc z zewnątrz na przód)
- Otwór: 10 × 18 mm (pionowy)
- Pozycja: 40mm od góry, 4mm od frontu

### Mikrostyk (prawa ściana - patrząc z zewnątrz na przód)
- **Tact Switch 6x6mm / 5mm THT - 2pin** (Botland)
- Wpust: 7 × 7 × 1 mm (na korpus switcha)
- 2× otwór Ø1.5mm (na piny THT, raster 5mm)
- Pozycja: 10mm od dołu, 10mm od frontu
- Montaż: lutowany od wewnątrz obudowy

### Wentylacja (lewa ściana - patrząc z zewnątrz na przód)
- Otwory kwadratowe 1.5 × 1.5 mm w układzie honeycomb offset
- Rozstaw: 3.5 mm
- Ściany: lewa (cała - dla czujnika PMS7003), górna, dolna

## Parametry druku (Ender-3 Pro + Devil Design PLA)

| Parametr | Wartość |
|----------|---------|
| **Wysokość warstwy** | 0.2 mm |
| **Ścianka (perimetry)** | 3 |
| **Wypełnienie** | 20% gyroid |
| **Temperatura dyszy** | 210°C (215°C pierwsza warstwa) |
| **Temperatura stołu** | 55°C (60°C pierwsza warstwa) |
| **Prędkość druku** | 45 mm/s |
| **Tolerancja** | 0.3 mm |

## Orientacja druku

### Korpus (case_body)
- **Orientacja:** Ściana z oknem plexi na stole (Z=0)
- Ściany boczne rosną pionowo do góry
- **Brim:** TAK (5mm)
- **Supports:** NIE

### Pokrywa (case_cover)
- **Orientacja:** Płyta na stole, ramka rośnie do góry
- **Brim:** TAK (5mm)
- **Supports:** NIE

## Szacowany czas druku

| Element | Warstwy | Filament | Czas |
|---------|---------|----------|------|
| Korpus | 265 | ~13m (~31 cm³) | ~5h |
| Pokrywa | 20 | ~4m (~10 cm³) | ~1h |
| **Łącznie** | - | **~17m (~41 cm³)** | **~6h** |

## Montaż

1. Wydrukuj korpus i pokrywę
2. Wklej plexi 57×32×1mm w wgłębienie (od wewnątrz)
3. Zamontuj elektronikę w korpusie
4. Włóż pokrywę (ramka wchodzi w korpus)
5. Przyklej pokrywę klejem na gorąco

## Eksport STL (OpenSCAD)

```bash
# Korpus
openscad -D 'part="body"' -o case_body.stl air_condition_case.scad

# Pokrywa
openscad -D 'part="cover"' -o case_cover.stl air_condition_case.scad

# Weryfikacja wymiarów (podgląd)
openscad -D 'part="check"' air_condition_case.scad
```

## Generowanie G-code (PrusaSlicer)

```bash
# Korpus
/Applications/PrusaSlicer.app/Contents/MacOS/PrusaSlicer \
  --load ender3_config.ini --export-gcode \
  --output case_body.gcode case_body.stl

# Pokrywa
/Applications/PrusaSlicer.app/Contents/MacOS/PrusaSlicer \
  --load ender3_config.ini --export-gcode \
  --output case_cover.gcode case_cover.stl
```

## Weryfikacja wymiarów

Aby sprawdzić czy przestrzeń wewnętrzna ma wymagane 57×82×49mm:

```bash
openscad -D 'part="check"' air_condition_case.scad
```

Czerwony prostopadłościan pokazuje rzeczywistą przestrzeń wewnętrzną.

## Konstrukcja pióro-wpust

System łączenia korpusu z pokrywą:

```
KORPUS (przekrój krawędzi tylnej):

    ←── 2mm ──→
    ┌─────────┐
    │  pełna  │
    │ ściana  │
    ├────┬────┤  ─┐
    │ 1mm│    │   │ 2mm (cover_lip)
    └────┴────┘  ─┘

POKRYWA:

    ┌─────────────┐  ─┐
    │    płyta    │   │ 2mm
    ├──┬───────┬──┤  ─┘
    │  │       │  │  ─┐
    │  │ puste │  │   │ 2mm (ramka)
    └──┴───────┴──┘  ─┘
    ↑1mm       1mm↑
```

Tolerancja ramki: 0.3mm (łatwe dopasowanie przy klejeniu)

## BOM - Elementy dodatkowe

| Element | Wymiary | Ilość |
|---------|---------|-------|
| Plexi przezroczyste | 57 × 32 × 1 mm | 1 szt |
| Tact Switch 6x6mm/5mm THT | 2-pin | 1 szt |
| Klej na gorąco | - | do klejenia |

## Historia wersji

- **v1.2** - Mikrostyk THT (2026-01-06)
  - Zmiana otworu mikrostyka z 4×4mm na 2× otwory Ø1.5mm (raster 5mm THT)
  - Switch lutowany od wewnątrz obudowy

- **v1.1** - Poprawki (2026-01-06)
  - Okno OLED zmienione na 55×30mm (1mm od każdej krawędzi bocznej)
  - Plexi 57×32×1mm
  - Poprawiona orientacja: USB/mikrostyk na prawej, wentylacja na lewej (patrząc z zewnątrz)
  - Dodane pliki G-code

- **v1.0** - Początkowa wersja (2026-01-06)
  - Wymiary wewnętrzne 57×82×49mm
  - Okno OLED z wgłębieniem na plexi
  - USB i mikrostyk na prawej ścianie
  - Wentylacja honeycomb na lewej, górnej i dolnej ścianie
  - System pióro-wpust do klejenia
