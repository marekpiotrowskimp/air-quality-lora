# Setup Blender for millimeters + fix imported STL
# Blender 3.3.1
#
# Użycie w Blenderze:
# 1. Najpierw zaimportuj STL (File -> Import -> STL)
# 2. Zakładka Scripting (góra ekranu)
# 3. Open -> wybierz ten plik
# 4. Run Script (przycisk ▶ lub Alt+P)

import bpy

# === 1. Ustaw jednostki na milimetry ===
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 0.001  # 1 jednostka Blendera = 1mm
scene.unit_settings.length_unit = 'MILLIMETERS'

# === 2. Przeskaluj wszystkie obiekty MESH x1000 ===
# (bo STL z OpenSCAD jest w mm, a Blender importuje jako metry)
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        obj.scale = (1000, 1000, 1000)
        # Zastosuj skalę (Apply Scale)
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        obj.select_set(False)

# === 3. Wycentruj widok na obiektach ===
# Zaznacz wszystkie obiekty mesh
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        obj.select_set(True)

# Dopasuj widok do zaznaczonych obiektów
for area in bpy.context.screen.areas:
    if area.type == 'VIEW_3D':
        for region in area.regions:
            if region.type == 'WINDOW':
                override = {'area': area, 'region': region}
                bpy.ops.view3d.view_selected(override)
                break

print("Gotowe! Jednostki: mm, obiekty przeskalowane i wycentrowane.")
