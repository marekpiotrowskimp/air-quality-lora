// =============================================================================
// AIR QUALITY RECEIVER - ENCLOSURE v2.3
// =============================================================================
// Obudowa do druku 3D dla projektu Air Quality Receiver
// 
// Drukarka: Creality Ender-3 Pro
// Filament: Devil Design PLA 1.75mm Black
// Tolerancja druku FDM: +/- 0.3mm
//
// Data: 2026-01-04
// Wersja: 2.3 (USB-C na lewej, orientacja druku poprawiona)
// =============================================================================

// === WYMIARY RZECZYWISTE KOMPONENTOW (ZMIERZONE!) ===
// LCD Waveshare Pico-ResTouch-LCD-2.8:
//   - PCB: 71.2 x 52.5 mm (zmierzone suwmiarka)
//   - Aktywny obszar dotykowy: 62 x 48 mm
//   - Orientacja: landscape (dluzszy wymiar wzdluz X)
// Stos elektroniki (LCD + adapter + XIAO + SX1262): 32mm

// === WYMIARY ZEWNETRZNE (dostosowane do pomiaru) ===
width = 79;      // szerokosc (X) - PCB 71.2 + luz 1mm/strone + scianki 2x2.5
height = 60;     // wysokosc (Y) - PCB 52.5 + luz 1mm/strone + scianki 2x2.5
depth = 39;      // glebokosc (Z) - stos 32mm + luz 4mm

// === SCIANKA ===
wall = 2.5;      // grubosc scianki

// === WYMIARY WEWNETRZNE (obliczone) ===
// Szerokosc wewn: 79 - 2*2.5 = 74mm (PCB 71.2mm = luz 2.8mm = 1.4mm/strone)
// Wysokosc wewn: 60 - 2*2.5 = 55mm (PCB 52.5mm = luz 2.5mm = 1.25mm/strone)
// Glebokosc wewn: 39 - 2.5 = 36.5mm (stos 32mm = luz 4.5mm)

// === LCD OTWOR (wiekszy niz aktywny obszar) ===
// Aktywny obszar LCD: 62 x 48 mm
// Otwor widoczny: wiekszy dla estetyki i latwego dostepu
lcd_visible_w = 67;    // 62 + 5 = margines 2.5mm/strone
lcd_visible_h = 48;    // rowny aktywny obszar (symetrycznie wycentrowany)

// Polka na PCB: PCB 71.2 x 52.5 + luz ~0.9mm/strone
lcd_shelf_w = 73;      // 71.2 + 1.8 = luz na wejscie PCB
lcd_shelf_h = 55;      // 52.5 + 2.5 = luz na wejscie PCB
lcd_glass = 2.0;       // glebokosc uskoku (grubosc panelu LCD)

// === USB-C (LEWA scianka patrzac na LCD, NA GORZE) ===
usb_w = 14;        // szerokosc otworu wzdluz Y
usb_h = 9;         // glebokosc otworu wzdluz Z
usb_from_bottom = 8;  // od DOLNEJ krawedzi do dolu otworu (po obrocie bedzie na gorze)
usb_from_back = 6; // od tylnej krawedzi do konca otworu

// === ZAOKRAGLENIE ===
r = 2;

// === TOLERANCJA DRUKU FDM ===
tol = 0.3;         // tolerancja dla laczenia czesci

// === ZATRZASKI SNAP-FIT ===
snap_w = 10;           // szerokosc zatrzasku
snap_h = 2.5;          // wysokosc zatrzasku
snap_d = 1.5;          // glebokosc zatrzasku
snap_tol = 0.3;        // luz dla zatrzasku

// === RENDER MODE ===
// Uzyj: openscad -D 'part="body"' -o case_body.stl air_quality_case.scad
part = "both";  // "body", "frame", "both"

$fn = 32;  // jakosc krzywych

// =============================================================================
// KORPUS (case_body)
// =============================================================================
module body() {
    difference() {
        // Zewnetrzna bryla z zaokragleniami
        hull() {
            // 4 slupki w rogach
            translate([r, r, 0]) cylinder(r=r, h=depth);
            translate([width-r, r, 0]) cylinder(r=r, h=depth);
            translate([r, height-r, 0]) cylinder(r=r, h=depth);
            translate([width-r, height-r, 0]) cylinder(r=r, h=depth);
        }
        
        // Wnetrze (otwarte od przodu Z=0, zamkniete z tylu)
        translate([wall, wall, -1])
            cube([width - 2*wall, height - 2*wall, depth - wall + 1]);
        
        // Otwor USB-C na LEWEJ sciance, na GORZE (patrzac na LCD od frontu)
        // Po obrocie 180 wokol X: dolna czesc Y staje sie gorna
        // usb_w (14mm) wzdluz Y, usb_h (9mm) wzdluz Z
        translate([-1, usb_from_bottom, depth - usb_from_back - usb_h])
            cube([wall + 2, usb_w, usb_h]);
        
        // Gniazda na zatrzaski (4 szt) - wciecia w sciankach
        // Gorne lewe
        translate([wall + 10, height - wall - snap_h - snap_tol, wall - 1])
            cube([snap_w + 2*snap_tol, snap_h + 2*snap_tol, snap_d + wall + 1]);
        // Gorne prawe
        translate([width - wall - 10 - snap_w, height - wall - snap_h - snap_tol, wall - 1])
            cube([snap_w + 2*snap_tol, snap_h + 2*snap_tol, snap_d + wall + 1]);
        // Dolne lewe
        translate([wall + 10, wall - snap_tol, wall - 1])
            cube([snap_w + 2*snap_tol, snap_h + 2*snap_tol, snap_d + wall + 1]);
        // Dolne prawe
        translate([width - wall - 10 - snap_w, wall - snap_tol, wall - 1])
            cube([snap_w + 2*snap_tol, snap_h + 2*snap_tol, snap_d + wall + 1]);
    }
    
    // Wewnetrzna krawedz do oparcia ramki
    difference() {
        translate([wall - 1, wall - 1, 0])
            cube([width - 2*wall + 2, height - 2*wall + 2, wall]);
        translate([wall + 1, wall + 1, -1])
            cube([width - 2*wall - 2, height - 2*wall - 2, wall + 2]);
    }
}

// =============================================================================
// RAMKA PRZEDNIA (case_frame)
// =============================================================================
module frame() {
    frame_depth = wall + lcd_glass;
    
    difference() {
        // Zewnetrzna bryla ramki
        hull() {
            translate([r, r, 0]) cylinder(r=r, h=frame_depth);
            translate([width-r, r, 0]) cylinder(r=r, h=frame_depth);
            translate([r, height-r, 0]) cylinder(r=r, h=frame_depth);
            translate([width-r, height-r, 0]) cylinder(r=r, h=frame_depth);
        }
        
        // Otwor widoczny na LCD (WIEKSZY niz aktywny obszar - dla palca)
        translate([(width - lcd_visible_w)/2, (height - lcd_visible_h)/2, -1])
            cube([lcd_visible_w, lcd_visible_h, frame_depth + 2]);
        
        // Polka/uskok na PCB LCD (wiekszy otwor z luzem, tylko na glebok lcd_glass)
        translate([(width - lcd_shelf_w)/2, (height - lcd_shelf_h)/2, wall])
            cube([lcd_shelf_w, lcd_shelf_h, lcd_glass + 1]);
        
        // Wciecie na krawedz korpusu (wpust ramki w korpus)
        translate([wall - 1 - tol, wall - 1 - tol, frame_depth - wall])
            cube([width - 2*wall + 2 + 2*tol, height - 2*wall + 2 + 2*tol, wall + 1]);
    }
    
    // Zatrzaski snap-fit (4 szt) - wypustki
    // Gorne lewe
    translate([wall + 10, height - wall, frame_depth])
        snap_tab();
    // Gorne prawe
    translate([width - wall - 10 - snap_w, height - wall, frame_depth])
        snap_tab();
    // Dolne lewe
    translate([wall + 10, wall - snap_d, frame_depth])
        mirror([0, 1, 0]) snap_tab();
    // Dolne prawe
    translate([width - wall - 10 - snap_w, wall - snap_d, frame_depth])
        mirror([0, 1, 0]) snap_tab();
}

// Zatrzask - prosty klin
module snap_tab() {
    // Prostokatna wypustka
    cube([snap_w, snap_d, snap_h]);
}

// =============================================================================
// RENDEROWANIE
// =============================================================================
if (part == "body") {
    // Obrot 180 wokol X - tylna sciana na stole, otwor LCD na gorze
    translate([0, height, depth])
        rotate([180, 0, 0])
            body();
} else if (part == "frame") {
    frame();
} else if (part == "both") {
    // Oba elementy obok siebie do podgladu
    color("DimGray") body();
    color("Gray") translate([width + 15, 0, 0]) frame();
}

// =============================================================================
// INFO I WERYFIKACJA WYMIAROW
// =============================================================================
echo(str(""));
echo(str("=== AIR QUALITY ENCLOSURE v2.3 ==="));
echo(str(""));
echo(str("WYMIARY ZEWNETRZNE: ", width, " x ", height, " x ", depth, " mm"));
echo(str("Grubosc scianki: ", wall, " mm"));
echo(str(""));
echo(str("WYMIARY WEWNETRZNE:"));
echo(str("  Szerokosc: ", width - 2*wall, " mm (PCB 70.2mm = luz ", width - 2*wall - 70.2, " mm)"));
echo(str("  Wysokosc: ", height - 2*wall, " mm (PCB 50.2mm = luz ", height - 2*wall - 50.2, " mm)"));
echo(str("  Glebokosc: ", depth - wall, " mm (stos 32mm = luz ", depth - wall - 32, " mm)"));
echo(str(""));
echo(str("LCD OTWOR WIDOCZNY: ", lcd_visible_w, " x ", lcd_visible_h, " mm"));
echo(str("  Aktywny obszar: 62 x 48 mm"));
echo(str("  Margines na palec: ", (lcd_visible_w - 62)/2, " x ", (lcd_visible_h - 48)/2, " mm/strone"));
echo(str(""));
echo(str("LCD POLKA NA PCB: ", lcd_shelf_w, " x ", lcd_shelf_h, " mm"));
echo(str("  PCB: 71.2 x 52.5 mm"));
echo(str("  Luz: ", (lcd_shelf_w - 71.2)/2, " x ", (lcd_shelf_h - 52.5)/2, " mm/strone"));
echo(str(""));
echo(str("RAMKA WOKOL OTWORU LCD:"));
echo(str("  Szerokosc ramki: ", (width - lcd_visible_w)/2, " mm"));
echo(str("  Wysokosc ramki: ", (height - lcd_visible_h)/2, " mm"));
echo(str(""));
echo(str("USB-C: ", usb_w, "x", usb_h, "mm na LEWEJ sciance, NA GORZE (patrzac na LCD)"));
echo(str("  ", usb_from_bottom, "mm od gornej krawedzi, ", usb_from_back, "mm od tylu"));
echo(str(""));
