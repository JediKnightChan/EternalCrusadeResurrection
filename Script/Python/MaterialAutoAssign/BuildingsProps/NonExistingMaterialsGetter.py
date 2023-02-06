import json
import unreal

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/space_marines_material_references.json", "r") as f:
    material_references_data = json.load(f)

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/materials_ec_to_ecr_renaming.json", "r") as f:
    material_rename_data = json.load(f)

missing_material_paths = []

for mesh_path, mesh_material_data in material_references_data.items():
    for slot_name, material_path in mesh_material_data.items():
        material_path = material_rename_data.get(material_path, material_path)
        if not unreal.EditorAssetLibrary.does_asset_exist(
                material_path) and material_path not in missing_material_paths:
            missing_material_paths.append(material_path)

for missing_material_path in missing_material_paths:
    print(missing_material_path)
