# Should be used with internal UE API in later versions of UE

"""
Auto create materials of Layout System based on data from materials_layout.json
"""

import re

import unreal
import os
import json

DO_OVERWRITE_EXISTING_MATERIALS = True

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/"
          "MaterialAutoAssign/BuildingsProps/space_marines_all_materials_data.json", "r") as f:
    materials_data = json.load(f)

# CHANGE ME
EC_PARENT_MATERIAL = "/EternalCrusade/Content/Characters/Eldar/Base_Materials/M_Eldar01.M_Eldar01"
# CHANGE ME
MY_PARENT_MATERIAL = "/Game/Characters/SpaceMarine/Materials/EmissiveArmorMaterial"

MATERIAL_LOC_PREFIX = "D:/MyProjects/eternal_crusade/umodel_needed/exp2/"
MATERIAL_FILTER_PREFIX = "/Game/Characters/SpaceMarine/"
materials_data = {k: v for k, v in materials_data.items() if "Parent" in v and EC_PARENT_MATERIAL in v["Parent"]}


def set_material_instance_texture(material_instance_asset, material_parameter, texture_path):
    if texture_path is None:
        return
    if not unreal.EditorAssetLibrary.does_asset_exist(texture_path):
        unreal.log_warning("Can't find texture: " + texture_path)
        return False
    tex_asset = unreal.EditorAssetLibrary.find_asset_data(texture_path).get_asset()
    return unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(material_instance_asset,
                                                                                       material_parameter, tex_asset)


base_material = unreal.EditorAssetLibrary.find_asset_data(MY_PARENT_MATERIAL)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def create_material_instance(material_path, parameters_names_to_values):
    print(" ".join([str(x) for x in parameters_names_to_values.values()]))
    material_instance_name = os.path.basename(material_path)
    material_instance_folder = os.path.dirname(material_path)
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        print(f"Warning: {material_path} already exists")
        if DO_OVERWRITE_EXISTING_MATERIALS:
            # material_instance_asset = unreal.EditorAssetLibrary.find_asset_data(material_path).get_asset()
            unreal.EditorAssetLibrary.delete_asset(material_path)
        else:
            return

    material_instance_asset = asset_tools.create_asset(material_instance_name, material_instance_folder,
                                                       unreal.MaterialInstanceConstant,
                                                       unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance_asset, base_material.get_asset())
    for parameter_name, parameter_value in parameters_names_to_values.items():
        set_material_instance_texture(material_instance_asset, parameter_name, parameter_value)
    unreal.EditorAssetLibrary.save_asset(material_path)


asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()


def get_texture_data(material_data):
    result = {}
    texture_data_key = None
    for k, v in material_data.items():
        if k.startswith("TextureParameterValues"):
            texture_data_key = k
    for _, data in material_data[texture_data_key].items():
        result_key = data["ParameterName"]
        result_value = data["ParameterValue"]
        result_value = re.search(r"Texture2D'(?P<path>[/a-zA-Z0-9_-]+)\.[a-zA-Z0-9_-]+'", result_value)
        if not result_value:
            raise ValueError(f"{data['ParameterValue']} doesn't match regex")
        result_value = result_value.group("path")
        result_value = result_value.replace("/EternalCrusade/Content/", "/Game/")
        result[result_key] = result_value
    return result


for material_path, material_data in materials_data.items():
    material_game_path = material_path.replace(MATERIAL_LOC_PREFIX, "/Game/").replace(".json", "")
    if not material_path.startswith(MATERIAL_FILTER_PREFIX):
        continue
    texture_data = get_texture_data(material_data)
    texture_data = {k: v.replace("/EternalCrusade/Content/", "/Game/") for k, v in texture_data.items()}

    ec_opaque_parameter_data = {"BaseColor": texture_data.get("Base Color", None),
                                "Normal": texture_data.get("Normal Map", None),
                                "Roughness": texture_data.get("Roughness Map", None),
                                "Metal": texture_data.get("Metallic Map", None),
                                "Specular": None}

    ec_opaque_masked_parameter_data = {"BaseColor": texture_data.get("Base Color", None),
                                       "Normal": texture_data.get("Normal Map", None),
                                       "Roughness": texture_data.get("Roughness Map", None),
                                       "Metal": texture_data.get("Metallic Map", None),
                                       "OpacityMask": texture_data.get("Opacity Map", None)}

    vertex_parameter_data = {
        "Base Color 1": texture_data.get("Base Color_A", None),
        "Base Color 2": texture_data.get("Base Color_B", None),
        "Metallic 1": texture_data.get("Metallic_A", None),
        "Metallic 2": texture_data.get("Metallic_B", None),
        "Normal 1": texture_data.get("Normal_A", None),
        "Normal 2": texture_data.get("Normal_B", None),
        "Roughness 1": texture_data.get("R_A", None),
        "Roughness 2": texture_data.get("R_B", None)
    }

    ec_bodypart_parameter_data = {
        "BaseColor": None,
        "Mask": texture_data.get("Color_Mask", None),
        "Metal": texture_data.get("Metallic", None),
        "Roughness": texture_data.get("Roughness", None),
        "Normal": texture_data.get("Normal_Map", None)
    }

    ec_sm_unique_parameter_data = {
        "BaseColor": texture_data.get("Base_Color", None),
        "Mask": texture_data.get("Color_Mask", None),
        "Metal": texture_data.get("Metallic", None),
        "Roughness": texture_data.get("Roughness", None),
        "Normal": texture_data.get("Normal_Map", None)
    }

    create_material_instance(material_game_path, ec_sm_unique_parameter_data)
