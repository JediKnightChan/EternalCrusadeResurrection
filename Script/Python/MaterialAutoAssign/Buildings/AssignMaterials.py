# Should be used with internal UE API in later versions of UE

"""
Auto assign materials to the corresponding meshes according to data from materials_props.json,
which was collected by MaterialReferencesCollector.py
"""

import unreal
import json
import os

with open("materials_props.json", "r") as f:
    materials_for_static = json.load(f)

editor_asset_library = unreal.editor_asset_library

for static_path, materials_list in materials_for_static.items():
    print(static_path)
    short_names_to_long = {os.path.basename(long): long for long in materials_list}
    static_path = static_path.replace("/Content/", "/Game/").replace(".uasset", "")
    if editor_asset_library.does_asset_exist(static_path):
        static_mesh_asset = unreal.editor_asset_library.load_asset(static_path)
        static_mesh_component = unreal.StaticMeshComponent()
        static_mesh_component.set_static_mesh(static_mesh_asset)
        material_slot_names = unreal.StaticMeshComponent.get_material_slot_names(static_mesh_component)
        for name in material_slot_names:
            name = str(name)
            material_index = static_mesh_component.get_material_index(name)
            if name in short_names_to_long:
                material_path = short_names_to_long[name]
            else:
                print(f"Could not find {name} among {short_names_to_long}")
                material_path = "/Engine/EngineMaterials/WorldGridMaterial"
            if not editor_asset_library.does_asset_exist(material_path):
                print(f"Material {material_path} not found")
                material_path = "/Engine/EngineMaterials/WorldGridMaterial"
            material_asset = unreal.editor_asset_library.load_asset(material_path)
            static_mesh_asset.set_material(material_index, material_asset)
    else:
        print(f"Static asset {static_path} not found")
