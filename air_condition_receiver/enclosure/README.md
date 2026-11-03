# Air Quality Receiver - 3D enclosure

## Version 2.3

Dimensions are based on **actual caliper measurements** of the PCB.

## Files

| File | Description |
|------|-------------|
| `air_quality_case.scad` | Parametric OpenSCAD source |
| `case_body.stl` | Enclosure body (STL for printing) |
| `case_body.gcode` | Body - G-code for the Ender-3 Pro |
| `case_frame.stl` | Front frame (STL for printing) |
| `case_frame.gcode` | Frame - G-code for the Ender-3 Pro |
| `ender3_config.ini` | Printer profile |
| `setup_blender.py` | Script that sets Blender units to mm |

## Dimensions

### External
- **79 × 60 × 39 mm**
- Wall thickness: 2.5 mm

### Internal
- Width: 74 mm (PCB 71.2 mm = 1.4 mm clearance per side)
- Height: 55 mm (PCB 52.5 mm = 1.25 mm clearance per side)
- Depth: 36.5 mm (stack 32 mm = 4.5 mm clearance)

### LCD
- **Visible opening: 67 × 48 mm**
- Active area: 62 × 48 mm
- Margin: 2.5 mm per side (width)
- PCB ledge: 73 × 55 mm

### USB-C
- Opening: 14 × 9 mm (**left side** when looking at the LCD)
- Position: 8 mm from the top, 6 mm from the back

## Print settings (Ender-3 Pro + Devil Design PLA)

| Parameter | Value |
|-----------|-------|
| **Layer height** | 0.2 mm |
| **Walls (perimeters)** | 3 |
| **Infill** | 20% gyroid |
| **Nozzle temperature** | 205°C (210°C first layer) |
| **Bed temperature** | 60°C |
| **Print speed** | 50 mm/s |
| **Tolerance** | 0.3 mm |

## Print time and filament usage

### Body (case_body.gcode)
- **Layers:** 130
- **Filament:** 13.9 m (~33.5 cm³)
- **Time:** ~3-4 h
- **Brim:** YES (5 mm)
- **Supports:** NO (orientation corrected - back wall on the bed)

### Frame (case_frame.gcode)
- **Layers:** 23
- **Filament:** 1.7 m (~4.1 cm³)
- **Time:** ~20-30 min
- **Brim:** NO
- **Supports:** NO

### Total: ~3.5-4.5 h, ~15.5 m of filament (~37 cm³)

## Print orientation

### Body
- **Orientation:** solid back wall on the bed (Z=0), LCD opening on top
- Prints without supports
- Walls grow vertically from the solid base

### Frame
- Orientation: flat on the bed, LCD side up
- Prints without supports

## Assembly

1. Insert the electronics (LCD + adapter + XIAO + SX1262) into the body
2. Fit the frame from the front
3. The snap-fit latches lock the frame
4. Optional: stick rubber feet on the bottom

## Checking in Blender

1. Open Blender 3.3.1
2. File → Import → STL (.stl)
3. Scripting tab → Open → `setup_blender.py` → Run Script
4. Dimensions are now in mm (Item panel → `N`)

## Changing parameters (OpenSCAD)

To change the dimensions, edit `air_quality_case.scad`:

```openscad
// External dimensions
width = 79;      // width
height = 60;     // height
depth = 39;      // depth
wall = 2.5;      // wall thickness

// Visible LCD opening
lcd_visible_w = 67;    // opening width
lcd_visible_h = 48;    // opening height

// USB-C position
usb_from_top = 8;   // from the top
usb_from_back = 6;  // from the back
```

STL export:
```bash
openscad -o case_body.stl -D 'part="body"' air_quality_case.scad
openscad -o case_frame.stl -D 'part="frame"' air_quality_case.scad
```

## Troubleshooting

### The LCD doesn't fit
- Increase `lcd_shelf_w` and `lcd_shelf_h` by 0.5-1 mm

### Latches too tight/loose
- Change `snap_tol` (default 0.3 mm)

### USB-C in the wrong place
- Measure the actual port position
- Change `usb_from_top` and `usb_from_back`

## Version history

- **v2.3** - USB-C on the left wall, corrected print orientation (back wall on the bed)
- **v2.2** - Dimensions from actual PCB measurements (71.2×52.5 mm), LCD opening 67×48 mm
- **v2.1** - Larger LCD opening for comfortable finger touch
- **v2.0** - Larger dimensions for safe assembly clearance
- **v1.0** - Initial version
