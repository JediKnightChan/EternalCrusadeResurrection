import unreal
import json

""" Should be used with internal UE API in later versions of UE

Auto assign materials to the corresponding meshes according to data from materials_props.json,
which was collected by ReferencesCollectorMaterial.py"""


with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/buildings_material_references.json", "r") as f:
    mesh_to_material_references = json.load(f)

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/materials_ec_to_ecr_renaming.json", "r") as f:
    material_rename_data = json.load(f)


def set_static_mesh_material(static_mesh, material_path_, material_index_):
    material_asset = unreal.EditorAssetLibrary.load_asset(material_path_)
    static_mesh.set_material(material_index_, material_asset)


def set_skeletal_mesh_material(skeletal_mesh, material_path_, slot_name_):
    skeletal_mesh_materials = skeletal_mesh.materials

    new_material_array = unreal.Array(unreal.SkeletalMaterial)

    for material_iter in skeletal_mesh_materials:
        slot_name_iter = material_iter.get_editor_property("material_slot_name")

        if str(slot_name_iter).lower() == slot_name_.lower():
            new_material_iter = unreal.SkeletalMaterial()
            new_material_asset = unreal.EditorAssetLibrary.load_asset(material_path_)
            new_material_iter.set_editor_property("material_slot_name", slot_name_iter)
            new_material_iter.set_editor_property("material_interface", new_material_asset)
            new_material_array.append(new_material_iter)
        else:
            new_material_array.append(material_iter)

    skeletal_mesh.set_editor_property("materials", new_material_array)


# CHANGE ME
STATIC_MESH = True
filter_path = "/Game/Buildings/Structures/STRUC_Ruins_01/"
mesh_to_material_references = {k: v for k, v in mesh_to_material_references.items() if k.startswith(filter_path)}

print(len(mesh_to_material_references))

c = 0
# CHANGE ME: slice of dict items! (is meant for performance)
for asset_path, slot_names_to_material_paths in list(mesh_to_material_references.items()):
    c += 1
    if c % 100 == 0:
        with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
                  "MaterialAutoAssign/BuildingsProps/temp.txt", "w") as f:
            f.write(f"Counter: {c}")

    print(asset_path)
    asset_path = asset_path.replace("/Content/", "/Game/").replace(".uasset", "")

    # Lower-case slot names
    slot_names_to_material_paths = {k.lower(): v for k, v in slot_names_to_material_paths.items()}

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if STATIC_MESH:
            filter_class = unreal.StaticMesh
        else:
            filter_class = unreal.SkeletalMesh
        if not unreal.EditorFilterLibrary.by_class([asset], filter_class):
            continue

        print(f"Got {'static' if STATIC_MESH else 'skeletal'} mesh", asset_path)

        if STATIC_MESH:
            static_mesh_component = unreal.StaticMeshComponent()
            static_mesh_component.set_static_mesh(asset)
            material_slot_names = unreal.StaticMeshComponent.get_material_slot_names(static_mesh_component)
            for slot_name in material_slot_names:
                slot_name = str(slot_name)
                material_index = static_mesh_component.get_material_index(slot_name)
                if slot_name.lower() in slot_names_to_material_paths:
                    material_path = slot_names_to_material_paths[slot_name.lower()]
                    material_path = material_rename_data.get(material_path, material_path)
                else:
                    print(
                        f"WarningSevere: {asset_path}: slot name {slot_name.lower()} not found among "
                        f"{slot_names_to_material_paths}")
                    material_path = "/Engine/EngineMaterials/WorldGridMaterial"
                if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
                    print(f"WarningSevere: {asset_path}: Material {material_path} not found")
                    material_path = "/Engine/EngineMaterials/WorldGridMaterial"
                set_static_mesh_material(asset, material_path, material_index)
        else:
            for slot_name, material_path in slot_names_to_material_paths.items():
                if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
                    print(f"WarningSevere: {asset_path}: Material {material_path} not found")
                    material_path = "/Engine/EngineMaterials/WorldGridMaterial"
                set_skeletal_mesh_material(asset, material_path, slot_name)
    else:
        print(f"Warning: Asset {asset_path} not found")
