# Air Condition (transmitter) - 3D enclosure

## Version 1.2

Dimensions are based on measurements of the electronics.

## Files

| File | Description |
|------|-------------|
| `air_condition_case.scad` | Parametric OpenSCAD source |
| `case_body.stl` | Enclosure body (STL for printing) |
| `case_body.gcode` | Body - G-code for the Ender-3 Pro |
| `case_cover.stl` | Back cover (STL for printing) |
| `case_cover.gcode` | Cover - G-code for the Ender-3 Pro |
| `ender3_config.ini` | Printer profile |

## Dimensions

### External
- **61 × 86 × 55 mm** (width × height × depth)
- Wall thickness: 2 mm

### Internal
- Width: 57 mm
- Height: 82 mm
- Depth: 49 mm (including a 1 mm recess for the acrylic window)

### OLED window (looking at the front from outside)
- **Visible opening: 55 × 30 mm**
- Position: 1 mm from each side edge, 4 mm from the top
- Acrylic recess: 57 × 32 × 1 mm

### USB (right wall - looking at the front from outside)
- Opening: 10 × 18 mm (vertical)
- Position: 40 mm from the top, 4 mm from the front

### Push button (right wall - looking at the front from outside)
- **Tact switch 6x6 mm / 5 mm THT - 2-pin** (Botland)
- Pocket: 7 × 7 × 1 mm (for the switch body)
- 2× Ø1.5 mm holes (for the THT pins, 5 mm pitch)
- Position: 10 mm from the bottom, 10 mm from the front
- Mounting: soldered from inside the enclosure

### Ventilation (left wall - looking at the front from outside)
- 1.5 × 1.5 mm square holes in an offset honeycomb pattern
- Spacing: 3.5 mm
- Walls: left (entire wall - for the PMS7003 sensor), top, bottom

## Print settings (Ender-3 Pro + Devil Design PLA)

| Parameter | Value |
|-----------|-------|
| **Layer height** | 0.2 mm |
| **Walls (perimeters)** | 3 |
| **Infill** | 20% gyroid |
| **Nozzle temperature** | 210°C (215°C first layer) |
| **Bed temperature** | 55°C (60°C first layer) |
| **Print speed** | 45 mm/s |
| **Tolerance** | 0.3 mm |

## Print orientation

### Body (case_body)
- **Orientation:** the wall with the acrylic window on the bed (Z=0)
- Side walls grow vertically upwards
- **Brim:** YES (5 mm)
- **Supports:** NO

### Cover (case_cover)
- **Orientation:** plate on the bed, frame grows upwards
- **Brim:** YES (5 mm)
- **Supports:** NO

## Estimated print time

| Part | Layers | Filament | Time |
|------|--------|----------|------|
| Body | 265 | ~13 m (~31 cm³) | ~5 h |
| Cover | 20 | ~4 m (~10 cm³) | ~1 h |
| **Total** | - | **~17 m (~41 cm³)** | **~6 h** |

## Assembly

1. Print the body and the cover
2. Glue the 57×32×1 mm acrylic into the recess (from the inside)
3. Mount the electronics in the body
4. Insert the cover (the frame slides into the body)
5. Fix the cover with hot glue

## STL export (OpenSCAD)

```bash
# Body
openscad -D 'part="body"' -o case_body.stl air_condition_case.scad

# Cover
openscad -D 'part="cover"' -o case_cover.stl air_condition_case.scad

# Dimension check (preview)
openscad -D 'part="check"' air_condition_case.scad
```

## G-code generation (PrusaSlicer)

```bash
# Body
/Applications/PrusaSlicer.app/Contents/MacOS/PrusaSlicer \
  --load ender3_config.ini --export-gcode \
  --output case_body.gcode case_body.stl

# Cover
/Applications/PrusaSlicer.app/Contents/MacOS/PrusaSlicer \
  --load ender3_config.ini --export-gcode \
  --output case_cover.gcode case_cover.stl
```

## Dimension check

To check that the internal space has the required 57×82×49 mm:

```bash
openscad -D 'part="check"' air_condition_case.scad
```

The red box shows the actual internal space.

## Tongue-and-groove joint

How the body joins the cover:

```
BODY (cross-section of the back edge):

    ←── 2mm ──→
    ┌─────────┐
    │  solid  │
    │  wall   │
    ├────┬────┤  ─┐
    │ 1mm│    │   │ 2mm (cover_lip)
    └────┴────┘  ─┘

COVER:

    ┌─────────────┐  ─┐
    │    plate    │   │ 2mm
    ├──┬───────┬──┤  ─┘
    │  │       │  │  ─┐
    │  │ empty │  │   │ 2mm (frame)
    └──┴───────┴──┘  ─┘
    ↑1mm       1mm↑
```

Frame tolerance: 0.3 mm (easy fit when gluing)

## BOM - additional parts

| Part | Dimensions | Qty |
|------|------------|-----|
| Clear acrylic | 57 × 32 × 1 mm | 1 |
| Tact switch 6x6 mm/5 mm THT | 2-pin | 1 |
| Hot glue | - | for gluing |

## Version history

- **v1.2** - THT push button (2026-01-06)
  - Push-button opening changed from 4×4 mm to 2× Ø1.5 mm holes (5 mm THT pitch)
  - Switch soldered from inside the enclosure

- **v1.1** - Fixes (2026-01-06)
  - OLED window changed to 55×30 mm (1 mm from each side edge)
  - Acrylic 57×32×1 mm
  - Corrected orientation: USB/push button on the right, ventilation on the left (looking from outside)
  - G-code files added

- **v1.0** - Initial version (2026-01-06)
  - Internal dimensions 57×82×49 mm
  - OLED window with an acrylic recess
  - USB and push button on the right wall
  - Honeycomb ventilation on the left, top and bottom walls
  - Tongue-and-groove joint for gluing
