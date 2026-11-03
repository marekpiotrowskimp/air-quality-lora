// =============================================================================
// AIR CONDITION (NADAJNIK) - ENCLOSURE v1.0
// =============================================================================
// Obudowa do druku 3D - KLEJONA (2 czesci)
// 
// Drukarka: Creality Ender-3 Pro
// Filament: Devil Design PLA 1.75mm Black
// Tolerancja druku FDM: +/- 0.3mm
//
// Data: 2026-01-06
// Wersja: 1.0
// =============================================================================
//
// KONSTRUKCJA:
// - Czesc 1: Korpus z oknem na OLED (case_body)
// - Czesc 2: Tylna pokrywa (case_cover)
// - Laczenie: klej na goraco + system pioro-wpust
//
// DRUK (bez supportow!):
// - Korpus: sciana z oknem plexi na stole, sciany rosna do gory
// - Pokrywa: plyta na stole, ramka 2mm rosnie do gory
//
// WENTYLACJA:
// - Lewa sciana: pelna wentylacja (czujnik pylu PMS7003)
// - Gorna/dolna sciana: wentylacja
// - Uklad otworow: honeycomb offset (lepszy rozklad naprezed)
// =============================================================================

// === WYMIARY WEWNETRZNE (podane przez uzytkownika) ===
inner_w = 57;      // szerokosc wewnetrzna (X)
inner_h = 82;      // wysokosc wewnetrzna (Y)
inner_d = 49;      // glebokosc wewnetrzna (Z)

// === SCIANKA ===
wall = 2;          // grubosc scianki (zredukowana z 2.5mm)

// === WYMIARY ZEWNETRZNE (obliczone) ===
// Szerokosc zewn: 57 + 2*2 = 61 mm
// Wysokosc zewn: 82 + 2*2 = 86 mm
// Glebokosc zewn: wymaga uwzglednienia plexi_depth i edge_thin
// Przestrzen wewn Z = body_depth - wall - plexi_depth - edge_thin
// Gdzie body_depth = outer_d - wall (bo pokrywa ma wall grubosci)
// Wiec: inner_d = (outer_d - wall) - wall - plexi_depth - edge_thin
// Przeksztalcenie: outer_d = inner_d + 2*wall + plexi_depth + edge_thin
// outer_d = 49 + 4 + 1 + 1 = 55 mm
outer_w = inner_w + 2 * wall;  // 61 mm
outer_h = inner_h + 2 * wall;  // 86 mm
outer_d = inner_d + 2 * wall + 1 + 1;  // 55 mm (uwzglednia plexi_depth=1 i edge_thin=1)

// === OKNO OLED (sciana frontowa) ===
// Pozycja: 1mm od LEWEJ krawedzi, 4mm od gory (patrzac z ZEWNATRZ na front)
oled_window_w = 55;           // szerokosc okna widocznego (1mm od kazdej krawedzi)
oled_window_h = 30;           // wysokosc okna widocznego
oled_from_left = 1;           // odstep od lewej krawedzi (z zewnatrz)
oled_from_top = 4;            // odstep od gornej krawedzi (z zewnatrz)

// Wglebienie na plexi (1mm glebokosci)
plexi_w = 57;                 // szerokosc plexi (cala szerokosc wewn)
plexi_h = 32;                 // wysokosc plexi (okno + 2mm margines)
plexi_depth = 1;              // glebokosc wglebienia = grubosc plexi
// Plexi jest wycentrowane wzgledem okna, wiec margines = (plexi - okno) / 2 = 1mm

// === USB (prawa sciana) ===
// Sciana prawa: wymiar wewn 49 x 82 mm (glebokosc x wysokosc)
// USB pionowe: 10mm szerokosc (wzdluz Z), 18mm wysokosc (wzdluz Y)
usb_w = 10;                   // szerokosc otworu (wzdluz glebokosci Z)
usb_h = 18;                   // wysokosc otworu (wzdluz wysokosci Y)
usb_from_top = 40;            // od gornej krawedzi wewn
usb_from_front = 4;           // od krawedzi z plexi (front)

// === MIKROSTYK (prawa sciana) ===
// Tact Switch 6x6mm / 5mm THT - 2pin (Botland)
// Wpust 7x7x1mm na korpus switcha, 2 otwory pod piny THT (raster 5mm)
button_recess_w = 7;          // szerokosc wpustu (wzdluz Z)
button_recess_h = 7;          // wysokosc wpustu (wzdluz Y)
button_recess_depth = 1;      // glebokosc wpustu
button_pin_d = 1.5;           // srednica otworu pod pin THT
button_pin_spacing = 5;       // rozstaw pinow (raster THT)
button_from_bottom = 10;      // od dolnej krawedzi wewn
button_from_front = 10;       // od krawedzi z plexi (front)

// === WENTYLACJA (honeycomb offset) ===
vent_hole_d = 1.5;            // srednica otworu
vent_spacing = 3.5;           // rozstaw miedzy otworami
vent_margin = 3;              // margines od krawedzi scianki (wewn)

// === POLACZENIE KORPUS-POKRYWA (system pioro-wpust) ===
edge_thin = wall / 2;         // 1mm - ciensza krawedz tylna korpusu
cover_lip = 2.0;              // wysokosc ramki pokrywy wchodzacej w korpus
cover_lip_thick = wall / 2;   // 1mm - grubosc ramki pokrywy
tol = 0.3;                    // tolerancja dopasowania

// === RENDER MODE ===
// Uzyj: openscad -D 'part="body"' -o case_body.stl air_condition_case.scad
// Uzyj: openscad -D 'part="cover"' -o case_cover.stl air_condition_case.scad
part = "both";  // "body", "cover", "both", "check"

$fn = 32;
$fn_vent = 12;  // mniejsza rozdzielczosc dla otworow wentylacyjnych (szybsze renderowanie)

// =============================================================================
// MODUL: Siatka wentylacyjna honeycomb dla sciany w plaszczyznie XY
// =============================================================================
// area_w, area_h - wymiary obszaru do pokrycia otworami
// hole_d - srednica otworu
// spacing - rozstaw
// margin - margines od krawedzi

module vent_holes_xy(area_w, area_h, hole_d, spacing, margin, thickness) {
    // Oblicz zakres
    start_x = margin;
    end_x = area_w - margin;
    start_y = margin;
    end_y = area_h - margin;
    
    row = 0;
    for (y = [start_y : spacing : end_y]) {
        // Przesuniecie co drugi rzad
        offset_x = ((floor((y - start_y) / spacing)) % 2 == 1) ? spacing / 2 : 0;
        
        for (x = [start_x + offset_x : spacing : end_x]) {
            if (x >= margin && x <= area_w - margin) {
                translate([x, y, -1])
                    cylinder(d = hole_d, h = thickness + 2);
            }
        }
    }
}

// =============================================================================
// KORPUS (case_body)
// =============================================================================
// Drukowany sciana z oknem plexi na stole (Z=0), sciany rosna do gory
// Krawedz tylna ciensza (edge_thin) - miejsce na ramke pokrywy

module body() {
    // Glebokosc korpusu (bez pokrywy)
    body_depth = outer_d - wall;  // 53 - 2 = 51mm
    
    difference() {
        // === ZEWNETRZNA BRYLA (prostopadloscian bez zaokraglen) ===
        cube([outer_w, outer_h, body_depth]);
        
        // === OKNO WIDOCZNE NA OLED ===
        // Pozycja okna od LEWEJ i od gory (patrzac z zewnatrz)
        oled_x = wall + oled_from_left;
        oled_y = outer_h - wall - oled_from_top - oled_window_h;
        
        translate([oled_x, oled_y, -1])
            cube([oled_window_w, oled_window_h, wall + 2]);
        
        // === WGLEBIENIE NA PLEXI (1mm glebokosci) ===
        // Plexi jest wycentrowane wzgledem okna
        plexi_x = oled_x - (plexi_w - oled_window_w) / 2;
        plexi_y = oled_y - (plexi_h - oled_window_h) / 2;
        
        translate([plexi_x, plexi_y, wall])
            cube([plexi_w, plexi_h, plexi_depth + 0.01]);
        
        // === WNETRZE - wyciecie na elektronike ===
        // Zaczyna sie od wall + plexi_depth, konczy przed edge_thin
        translate([wall, wall, wall + plexi_depth])
            cube([inner_w, inner_h, body_depth - wall - plexi_depth - edge_thin]);
        
        // === POSZERZENIE NA RAMKE POKRYWY (ciensza krawedz) ===
        translate([edge_thin, edge_thin, body_depth - cover_lip])
            cube([outer_w - 2 * edge_thin, outer_h - 2 * edge_thin, cover_lip + 1]);
        
        // === OTWOR USB (prawa sciana - patrzac z zewnatrz na front) ===
        // USB pionowe: 10mm (Z) x 18mm (Y)
        usb_y = outer_h - wall - usb_from_top - usb_h;
        usb_z = wall + plexi_depth + usb_from_front;
        
        translate([-1, usb_y, usb_z])
            cube([wall + 2, usb_h, usb_w]);
        
        // === MIKROSTYK - wpust (prawa sciana - patrzac z zewnatrz na front) ===
        // Tact Switch 6x6mm / 5mm THT - 2pin, lutowany od srodka
        button_y = wall + button_from_bottom;
        button_z = wall + plexi_depth + button_from_front;
        
        // Wpust 7x7x1mm (od zewnatrz) - korpus switcha 6x6mm
        translate([-0.01, button_y, button_z])
            cube([button_recess_depth + 0.01, button_recess_h, button_recess_w]);
        
        // 2 otwory pod piny THT (raster 5mm, wycentrowane we wpuscie)
        button_center_y = button_y + button_recess_h / 2;
        button_center_z = button_z + button_recess_w / 2;
        
        // Otwor 1 (Z - polowa rastra)
        translate([-1, button_center_y, button_center_z - button_pin_spacing / 2])
            rotate([0, 90, 0])
                cylinder(d = button_pin_d, h = wall + 2, $fn = 16);
        
        // Otwor 2 (Z + polowa rastra)
        translate([-1, button_center_y, button_center_z + button_pin_spacing / 2])
            rotate([0, 90, 0])
                cylinder(d = button_pin_d, h = wall + 2, $fn = 16);
        
        // === WENTYLACJA - LEWA SCIANA (patrzac z zewnatrz na front) ===
        // Wymiary obszaru wentylacji: wysokosc Y x glebokosc Z
        // Zakres Y: od wall + margin do outer_h - wall - margin
        // Zakres Z: od wall + plexi_depth + margin do body_depth - margin
        
        vent_area_h = outer_h - 2 * wall;  // 82mm
        vent_area_d = body_depth - wall - plexi_depth;  // ~48mm
        
        for (row = [0 : floor((vent_area_h - 2 * vent_margin) / vent_spacing)]) {
            offset_z = (row % 2 == 1) ? vent_spacing / 2 : 0;
            y = wall + vent_margin + row * vent_spacing;
            
            if (y <= outer_h - wall - vent_margin) {
                for (col = [0 : floor((vent_area_d - 2 * vent_margin) / vent_spacing)]) {
                    z = wall + plexi_depth + vent_margin + col * vent_spacing + offset_z;
                    
                    if (z <= body_depth - vent_margin && z >= wall + plexi_depth + vent_margin) {
                        translate([outer_w - wall - 1, y - vent_hole_d/2, z - vent_hole_d/2])
                            cube([wall + 2, vent_hole_d, vent_hole_d]);
                    }
                }
            }
        }
        
        // === WENTYLACJA - GORNA SCIANA ===
        // Wymiary: szerokosc X x glebokosc Z
        vent_area_w = outer_w - 2 * wall;  // 57mm
        
        for (row = [0 : floor((vent_area_w - 2 * vent_margin) / vent_spacing)]) {
            offset_z = (row % 2 == 1) ? vent_spacing / 2 : 0;
            x = wall + vent_margin + row * vent_spacing;
            
            if (x <= outer_w - wall - vent_margin) {
                for (col = [0 : floor((vent_area_d - 2 * vent_margin) / vent_spacing)]) {
                    z = wall + plexi_depth + vent_margin + col * vent_spacing + offset_z;
                    
                    if (z <= body_depth - vent_margin && z >= wall + plexi_depth + vent_margin) {
                        translate([x - vent_hole_d/2, outer_h - wall - 1, z - vent_hole_d/2])
                            cube([vent_hole_d, wall + 2, vent_hole_d]);
                    }
                }
            }
        }
        
        // === WENTYLACJA - DOLNA SCIANA ===
        for (row = [0 : floor((vent_area_w - 2 * vent_margin) / vent_spacing)]) {
            offset_z = (row % 2 == 1) ? vent_spacing / 2 : 0;
            x = wall + vent_margin + row * vent_spacing;
            
            if (x <= outer_w - wall - vent_margin) {
                for (col = [0 : floor((vent_area_d - 2 * vent_margin) / vent_spacing)]) {
                    z = wall + plexi_depth + vent_margin + col * vent_spacing + offset_z;
                    
                    if (z <= body_depth - vent_margin && z >= wall + plexi_depth + vent_margin) {
                        translate([x - vent_hole_d/2, -1, z - vent_hole_d/2])
                            cube([vent_hole_d, wall + 2, vent_hole_d]);
                    }
                }
            }
        }
    }
}

// Korpus obrocony do druku (sciana z plexi na stole)
module body_print() {
    body();
}

// =============================================================================
// TYLNA POKRYWA (case_cover)
// =============================================================================
// Drukowana plyta na stole (Z=0), ramka rosnie do gory

module cover() {
    union() {
        // === GLOWNA PLYTA POKRYWY ===
        cube([outer_w, outer_h, wall]);
        
        // === RAMKA WCHODZACA W KORPUS (pioro) ===
        translate([edge_thin + tol, edge_thin + tol, wall])
            difference() {
                cube([outer_w - 2 * edge_thin - 2 * tol, 
                      outer_h - 2 * edge_thin - 2 * tol, 
                      cover_lip]);
                      
                // Wyciecie wewnatrz ramki
                translate([cover_lip_thick, cover_lip_thick, -1])
                    cube([outer_w - 2 * edge_thin - 2 * tol - 2 * cover_lip_thick, 
                          outer_h - 2 * edge_thin - 2 * tol - 2 * cover_lip_thick, 
                          cover_lip + 2]);
            }
    }
}

// =============================================================================
// WERYFIKACJA WYMIAROW WEWNETRZNYCH
// =============================================================================
module inner_space_check() {
    // Przestrzen wewnetrzna - powinna byc dokladnie 57 x 82 x 49 mm
    // (z uwzglednieniem wglebienia na plexi 1mm)
    
    body_depth = outer_d - wall;
    actual_inner_d = body_depth - wall - plexi_depth - edge_thin;
    
    echo(str("=== WERYFIKACJA WYMIAROW WEWNETRZNYCH ==="));
    echo(str("Oczekiwane: ", inner_w, " x ", inner_h, " x ", inner_d, " mm"));
    echo(str("Rzeczywiste X (szer): ", inner_w, " mm (outer_w - 2*wall = ", outer_w, " - ", 2*wall, ")"));
    echo(str("Rzeczywiste Y (wys): ", inner_h, " mm (outer_h - 2*wall = ", outer_h, " - ", 2*wall, ")"));
    echo(str("Rzeczywiste Z (gleb): ", actual_inner_d, " mm"));
    echo(str("  body_depth = ", body_depth));
    echo(str("  - wall = ", wall));
    echo(str("  - plexi_depth = ", plexi_depth));
    echo(str("  - edge_thin = ", edge_thin));
    echo(str(""));
    echo(str("UWAGA: Glebokosc wewnetrzna = ", actual_inner_d, " mm"));
    echo(str("       Wymagane minimum: ", inner_d - plexi_depth, " mm (bo plexi zajmuje ", plexi_depth, "mm)"));
    
    // Wizualizacja przestrzeni
    color("red", 0.3)
        translate([wall, wall, wall + plexi_depth])
            cube([inner_w, inner_h, actual_inner_d]);
}

// =============================================================================
// RENDEROWANIE
// =============================================================================
if (part == "body") {
    body_print();
} else if (part == "cover") {
    cover();
} else if (part == "both") {
    // Podglad obu czesci obok siebie
    color("DimGray") body_print();
    color("Gray") translate([outer_w + 15, 0, 0]) cover();
} else if (part == "check") {
    // Tryb weryfikacji - pokazuje przestrzen wewnetrzna
    %body_print();  // polprzezroczysty korpus
    inner_space_check();
}

// =============================================================================
// INFO - wyswietlane przy renderowaniu
// =============================================================================
echo(str(""));
echo(str("=== AIR CONDITION ENCLOSURE v1.0 ==="));
echo(str(""));
echo(str("WYMIARY ZEWNETRZNE: ", outer_w, " x ", outer_h, " x ", outer_d, " mm"));
echo(str("WYMIARY WEWNETRZNE (zadane): ", inner_w, " x ", inner_h, " x ", inner_d, " mm"));
echo(str("GRUBOSC SCIANKI: ", wall, " mm"));
echo(str(""));
echo(str("OKNO OLED: ", oled_window_w, " x ", oled_window_h, " mm"));
echo(str("PLEXI: ", plexi_w, " x ", plexi_h, " x ", plexi_depth, " mm"));
echo(str(""));
echo(str("USB: ", usb_w, " x ", usb_h, " mm (", usb_from_top, "mm od gory, ", usb_from_front, "mm od frontu)"));
echo(str("MIKROSTYK: Tact Switch 6x6mm/5mm THT, wpust ", button_recess_w, "x", button_recess_h, "x", button_recess_depth, 
         "mm, 2x otwor fi", button_pin_d, "mm (raster ", button_pin_spacing, "mm)"));
echo(str(""));
echo(str("WENTYLACJA: otwory fi", vent_hole_d, "mm, rozstaw ", vent_spacing, "mm (honeycomb)"));
echo(str(""));
echo(str("KONSTRUKCJA KLEJONA:"));
echo(str("  Korpus: sciana OLED + sciany boczne (krawedz tylna ", edge_thin, "mm)"));
echo(str("  Pokrywa: plyta ", wall, "mm + ramka ", cover_lip_thick, "x", cover_lip, "mm"));
echo(str("  Tolerancja: ", tol, "mm"));
echo(str(""));
echo(str("EKSPORT STL:"));
echo(str("  openscad -D 'part=\"body\"' -o case_body.stl air_condition_case.scad"));
echo(str("  openscad -D 'part=\"cover\"' -o case_cover.stl air_condition_case.scad"));
echo(str(""));
