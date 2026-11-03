// =============================================================================
// AIR QUALITY RECEIVER - ENCLOSURE v3.0
// =============================================================================
// Obudowa do druku 3D - KLEJONA (2 czesci)
// 
// Drukarka: Creality Ender-3 Pro
// Filament: Devil Design PLA 1.75mm Black
// Tolerancja druku FDM: +/- 0.3mm
//
// Data: 2026-01-05
// Wersja: 3.0 (klejona obudowa, ramka zintegrowana z korpusem)
// =============================================================================
//
// KONSTRUKCJA:
// - Czesc 1: Korpus z ramka LCD (case_body_v3)
// - Czesc 2: Tylna pokrywa/dekiel (case_cover_v3)
// - Laczenie: klej na goraco (latwe otwarcie w przyszlosci)
//
// DRUK (bez supportow!):
// - Korpus: ramka LCD na stole, sciany rosna do gory
// - Pokrywa: plyta na stole, ramka 2mm rosnie do gory
// =============================================================================

// === WYMIARY ZEWNETRZNE ===
width = 79;       // szerokosc (X)
height = 60;      // wysokosc (Y)
depth = 38;       // glebokosc (Z) calkowita

// === SCIANKA ===
wall = 2.5;       // grubosc scianki

// === WYMIARY WEWNETRZNE ===
// Szerokosc wewn: 79 - 2*2.5 = 74 mm
// Wysokosc wewn: 60 - 2*2.5 = 55 mm
// Glebokosc wewn: 38 - 2.5 (ramka) - 2.5 (dekiel) = 33 mm (32 elektronika + 1 luz)

// === LCD ===
lcd_visible_w = 67;    // otwor widoczny LCD
lcd_visible_h = 48;    // otwor widoczny LCD
lcd_shelf_w = 73;      // polka na PCB
lcd_shelf_h = 55;      // polka na PCB
lcd_glass = 2.0;       // glebokosc uskoku na panel LCD

// === USB-C (lewa scianka, gora, kabel do tylu) ===
usb_w = 19;            // szerokosc otworu wzdluz Y (bylo 14, +5mm)
usb_h = 9;             // glebokosc otworu wzdluz Z
usb_from_top = 8;      // od gornej krawedzi
usb_from_back = 6;     // od tylnej krawedzi

// === POLACZENIE KORPUS-POKRYWA ===
edge_thin = wall / 2;  // 1.25mm - ciensza krawedz tylna korpusu
cover_lip = 2.0;       // wysokosc ramki pokrywy wchodzacej w korpus
cover_lip_thick = wall / 2;  // 1.25mm - grubosc ramki pokrywy
tol = 0.3;             // tolerancja dopasowania

// === ZAOKRAGLENIE ===
r = 2;

// === RENDER MODE ===
// Uzyj: openscad -D 'part="body"' -o case_body_v3.stl air_quality_case_v3.scad
part = "both";  // "body", "cover", "both"

$fn = 32;

// =============================================================================
// KORPUS Z RAMKA LCD (case_body_v3)
// =============================================================================
// Drukowany ramka LCD na stole (Z=0), sciany rosna do gory
// Krawedz tylna ciensza (edge_thin) - miejsce na ramke pokrywy

module body_v3() {
    // Glebokosc korpusu (bez dekla)
    body_depth = depth - wall;  // 38 - 2.5 = 35.5mm
    
    difference() {
        // Zewnetrzna bryla z zaokragleniami
        hull() {
            translate([r, r, 0]) cylinder(r=r, h=body_depth);
            translate([width-r, r, 0]) cylinder(r=r, h=body_depth);
            translate([r, height-r, 0]) cylinder(r=r, h=body_depth);
            translate([width-r, height-r, 0]) cylinder(r=r, h=body_depth);
        }
        
        // Otwor widoczny na LCD (na froncie, Z=0)
        translate([(width - lcd_visible_w)/2, (height - lcd_visible_h)/2, -1])
            cube([lcd_visible_w, lcd_visible_h, wall + 2]);
        
        // Polka/uskok na PCB LCD
        translate([(width - lcd_shelf_w)/2, (height - lcd_shelf_h)/2, wall])
            cube([lcd_shelf_w, lcd_shelf_h, lcd_glass + 1]);
        
        // Wnetrze - wyciecie na elektronike
        // Zaczyna sie od wall + lcd_glass, konczy przed edge_thin
        translate([wall, wall, wall + lcd_glass])
            cube([width - 2*wall, height - 2*wall, body_depth - wall - lcd_glass - edge_thin]);
        
        // Poszerzenie na ramke pokrywy (ciensza krawedz)
        // Na gorze korpusu, glebokosc cover_lip, scianka edge_thin
        translate([edge_thin, edge_thin, body_depth - cover_lip])
            cube([width - 2*edge_thin, height - 2*edge_thin, cover_lip + 1]);
        
        // Otwor USB-C na LEWEJ sciance, NA GORZE (patrzac na LCD od frontu)
        // Po odwroceniu do druku (ramka na stole), USB bedzie na gorze
        // usb_from_top - po obrocie 180 wokol X bedzie na gorze Y
        translate([-1, usb_from_top, body_depth - usb_from_back - usb_h])
            cube([wall + 2, usb_w, usb_h]);
    }
}

// Korpus obrocony do druku (ramka LCD na stole)
module body_v3_print() {
    translate([0, height, depth - wall])
        rotate([180, 0, 0])
            body_v3();
}

// =============================================================================
// TYLNA POKRYWA / DEKIEL (case_cover_v3)
// =============================================================================
// Drukowana plyta na stole (Z=0), ramka rosnie do gory

module cover_v3() {
    difference() {
        union() {
            // Glowna plyta pokrywy
            hull() {
                translate([r, r, 0]) cylinder(r=r, h=wall);
                translate([width-r, r, 0]) cylinder(r=r, h=wall);
                translate([r, height-r, 0]) cylinder(r=r, h=wall);
                translate([width-r, height-r, 0]) cylinder(r=r, h=wall);
            }
            
            // Ramka wchodzaca w korpus
            translate([edge_thin + tol, edge_thin + tol, wall])
                difference() {
                    cube([width - 2*edge_thin - 2*tol, height - 2*edge_thin - 2*tol, cover_lip]);
                    translate([cover_lip_thick, cover_lip_thick, -1])
                        cube([width - 2*edge_thin - 2*tol - 2*cover_lip_thick, 
                              height - 2*edge_thin - 2*tol - 2*cover_lip_thick, 
                              cover_lip + 2]);
                }
        }
    }
}

// =============================================================================
// RENDEROWANIE
// =============================================================================
if (part == "body") {
    body_v3_print();
} else if (part == "cover") {
    cover_v3();
} else if (part == "both") {
    // Podglad obu czesci obok siebie
    color("DimGray") body_v3_print();
    color("Gray") translate([width + 15, 0, 0]) cover_v3();
}

// =============================================================================
// INFO
// =============================================================================
echo(str(""));
echo(str("=== AIR QUALITY ENCLOSURE v3.0 ==="));
echo(str(""));
echo(str("WYMIARY ZEWNETRZNE: ", width, " x ", height, " x ", depth, " mm"));
echo(str("WYMIARY WEWNETRZNE: ", width - 2*wall, " x ", height - 2*wall, " x ", depth - 2*wall, " mm"));
echo(str(""));
echo(str("LCD OTWOR: ", lcd_visible_w, " x ", lcd_visible_h, " mm"));
echo(str("LCD POLKA: ", lcd_shelf_w, " x ", lcd_shelf_h, " mm"));
echo(str(""));
echo(str("USB-C: ", usb_w, " x ", usb_h, " mm (lewa scianka, gora)"));
echo(str(""));
echo(str("KONSTRUKCJA KLEJENA:"));
echo(str("  Korpus: ramka LCD + sciany (krawedz tylna ", edge_thin, "mm)"));
echo(str("  Pokrywa: plyta ", wall, "mm + ramka ", cover_lip_thick, "x", cover_lip, "mm"));
echo(str(""));
