# Air Quality Receiver - Obudowa 3D

## Wersja 2.3

Wymiary oparte na **rzeczywistych pomiarach** PCB suwmiarką.

## Pliki

| Plik | Opis |
|------|------|
| `air_quality_case.scad` | Źródło parametryczne OpenSCAD |
| `case_body.stl` | Korpus obudowy (STL do druku) |
| `case_body.gcode` | Korpus - G-code dla Ender-3 Pro |
| `case_frame.stl` | Ramka przednia (STL do druku) |
| `case_frame.gcode` | Ramka - G-code dla Ender-3 Pro |
| `ender3_config.ini` | Profil drukarki |
| `setup_blender.py` | Skrypt ustawiający Blender na mm |

## Wymiary

### Zewnętrzne
- **79 × 60 × 39 mm**
- Grubość ścianki: 2.5 mm

### Wewnętrzne
- Szerokość: 74 mm (PCB 71.2mm = luz 1.4mm/stronę)
- Wysokość: 55 mm (PCB 52.5mm = luz 1.25mm/stronę)
- Głębokość: 36.5 mm (stos 32mm = luz 4.5mm)

### LCD
- **Otwór widoczny: 67 × 48 mm**
- Aktywny obszar: 62 × 48 mm
- Margines: 2.5mm/stronę (szerokość)
- Półka na PCB: 73 × 55 mm

### USB-C
- Otwór: 14 × 9 mm (**lewa strona** patrząc na LCD)
- Pozycja: 8mm od góry, 6mm od tyłu

## Parametry druku (Ender-3 Pro + Devil Design PLA)

| Parametr | Wartość |
|----------|---------|
| **Wysokość warstwy** | 0.2 mm |
| **Ścianka (perimetry)** | 3 |
| **Wypełnienie** | 20% gyroid |
| **Temperatura dyszy** | 205°C (210°C pierwsza warstwa) |
| **Temperatura stołu** | 60°C |
| **Prędkość druku** | 50 mm/s |
| **Tolerancja** | 0.3 mm |

## Czas druku i zużycie filamentu

### Korpus (case_body.gcode)
- **Warstwy:** 130
- **Filament:** 13.9m (~33.5 cm³)
- **Czas:** ~3-4h
- **Brim:** TAK (5mm)
- **Supports:** NIE (orientacja poprawiona - tylna ściana na stole)

### Ramka (case_frame.gcode)
- **Warstwy:** 23
- **Filament:** 1.7m (~4.1 cm³)
- **Czas:** ~20-30 min
- **Brim:** NIE
- **Supports:** NIE

### Łącznie: ~3.5-4.5h, ~15.5m filamentu (~37 cm³)

## Orientacja druku

### Korpus
- **Orientacja:** Tylna ściana solidna na stole (Z=0), otwór LCD na górze
- Drukuje się bez supportów
- Ściany rosną pionowo od solidnej podstawy

### Ramka  
- Orientacja: płasko na stole, stroną LCD do góry
- Drukuje się bez supportów

## Montaż

1. Włóż elektronikę (LCD + adapter + XIAO + SX1262) do korpusu
2. Nałóż ramkę od przodu
3. Zatrzaski snap-fit zablokują ramkę
4. Opcjonalnie: naklej gumowe nóżki na spód

## Weryfikacja w Blender

1. Otwórz Blender 3.3.1
2. File → Import → STL (.stl)
3. Zakładka Scripting → Open → `setup_blender.py` → Run Script
4. Teraz wymiary są w mm (panel Item → `N`)

## Modyfikacja parametrów (OpenSCAD)

Aby zmienić wymiary, edytuj `air_quality_case.scad`:

```openscad
// Wymiary zewnętrzne
width = 79;      // szerokość
height = 60;     // wysokość  
depth = 39;      // głębokość
wall = 2.5;      // grubość ścianki

// LCD otwór widoczny
lcd_visible_w = 67;    // szerokość otworu
lcd_visible_h = 48;    // wysokość otworu

// USB-C pozycja
usb_from_top = 8;   // od góry
usb_from_back = 6;  // od tyłu
```

Eksport STL:
```bash
openscad -o case_body.stl -D 'part="body"' air_quality_case.scad
openscad -o case_frame.stl -D 'part="frame"' air_quality_case.scad
```

## Troubleshooting

### LCD nie pasuje
- Zwiększ `lcd_shelf_w` i `lcd_shelf_h` o 0.5-1mm

### Zatrzaski za ciasne/luźne
- Zmień `snap_tol` (domyślnie 0.3mm)

### USB-C w złym miejscu
- Zmierz rzeczywistą pozycję portu
- Zmień `usb_from_top` i `usb_from_back`

## Historia wersji

- **v2.3** - USB-C na lewej ściance, poprawiona orientacja druku (tylna ściana na stole)
- **v2.2** - Wymiary z rzeczywistych pomiarów PCB (71.2×52.5mm), otwór LCD 67×48mm
- **v2.1** - Większy otwór LCD dla wygodnego dotyku palcem
- **v2.0** - Zwiększone wymiary dla bezpiecznego luzu montażowego
- **v1.0** - Początkowa wersja
