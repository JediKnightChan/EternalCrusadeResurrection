# Should be used with internal UE API in later versions of UE

"""
Auto assign materials to the corresponding meshes according to data from materials_props.json,
which was collected by ReferencesCollectorMaterial.py
"""

import unreal
import json

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/props_material_references.json", "r") as f:
    mesh_to_material_references = json.load(f)

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/materials_ec_to_ecr_renaming.json", "r") as f:
    material_rename_data = json.load(f)

print(len(mesh_to_material_references))

c = 0
# CHANGE ME: slice of dict items! (is meant for performance)
for static_path, slot_names_to_material_paths in list(mesh_to_material_references.items())[:300]:
    c += 1
    if c % 100 == 0:
        with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
                  "MaterialAutoAssign/BuildingsProps/temp.txt", "w") as f:
            f.write(f"Counter: {c}")

    print(static_path)
    static_path = static_path.replace("/Content/", "/Game/").replace(".uasset", "")

    # Lower-case slot names
    slot_names_to_material_paths = {k.lower(): v for k, v in slot_names_to_material_paths.items()}

    if unreal.EditorAssetLibrary.does_asset_exist(static_path):
        static_mesh_asset = unreal.EditorAssetLibrary.load_asset(static_path)
        static_mesh_component = unreal.StaticMeshComponent()
        static_mesh_component.set_static_mesh(static_mesh_asset)
        material_slot_names = unreal.StaticMeshComponent.get_material_slot_names(static_mesh_component)
        for slot_name in material_slot_names:
            slot_name = str(slot_name)
            material_index = static_mesh_component.get_material_index(slot_name)
            if slot_name.lower() in slot_names_to_material_paths:
                material_path = slot_names_to_material_paths[slot_name.lower()]
                material_path = material_rename_data.get(material_path, material_path)
            else:
                print(
                    f"Error: {static_path}: slot name {slot_name.lower()} not found among {slot_names_to_material_paths}")
                material_path = "/Engine/EngineMaterials/WorldGridMaterial"
            if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
                print(f"Error: {static_path}: Material {material_path} not found")
                material_path = "/Engine/EngineMaterials/WorldGridMaterial"
            material_asset = unreal.EditorAssetLibrary.load_asset(material_path)
            static_mesh_asset.set_material(material_index, material_asset)
    else:
        print(f"Warning: Static asset {static_path} not found")
