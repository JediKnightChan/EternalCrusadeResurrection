# Should be used with internal UE API in later versions of UE


import unreal
import os
import json

with open("materials_layout.json", "r") as f:
    materials_data = json.load(f)


def set_material_instance_texture(material_instance_asset, material_parameter, texture_path):
    if not unreal.editor_asset_library.does_asset_exist(texture_path):
        unreal.log_warning("Can't find texture: " + texture_path)
        return False
    tex_asset = unreal.editor_asset_library.find_asset_data(texture_path).get_asset()
    return unreal.material_editing_library.set_material_instance_texture_parameter_value(material_instance_asset,
                                                                                         material_parameter, tex_asset)


base_material = unreal.editor_asset_library.find_asset_data("/Game/Materials/LayoutSystem/MasterBuildingOpaque")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material_editing_library = unreal.material_editing_library
editor_asset_library = unreal.editor_asset_library


def create_material_instance(material_path, base_color_t, metallic_t, normal_t, roughness_t):
    material_instance_name = os.path.basename(material_path)
    material_instance_folder = os.path.dirname(material_path)
    if editor_asset_library.does_asset_exist(material_path):
        # material_instance_asset = editor_asset_library.find_asset_data(material_path).get_asset()
        editor_asset_library.delete_asset(material_path)

    material_instance_asset = asset_tools.create_asset(material_instance_name, material_instance_folder,
                                                       unreal.MaterialInstanceConstant,
                                                       unreal.MaterialInstanceConstantFactoryNew())
    material_editing_library.set_material_instance_parent(material_instance_asset, base_material.get_asset())
    set_material_instance_texture(material_instance_asset, "BaseColor", base_color_t)
    set_material_instance_texture(material_instance_asset, "Normal", normal_t)
    set_material_instance_texture(material_instance_asset, "Roughness", roughness_t)
    set_material_instance_texture(material_instance_asset, "Metallic", metallic_t)
    editor_asset_library.save_asset(material_path)


asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
materials_root_dir = '/Game/Materials/LayoutSystem/'
skip_counter = 0

for material_name, material_properties in materials_data.items():
    material_name = material_name.replace("/EternalCrusade/Content/", "/Game/")
    if material_properties["parent"] == "/EternalCrusade/Content/Materials/Templates/M_Opaque.M_Opaque":
        texture_data = material_properties["textures"]
        texture_data = {k: v.replace("/EternalCrusade/Content/", "/Game/") for k, v in texture_data.items()}
        base_color_t, metallic_t, normal_t, roughness_t = texture_data["Base Color"], texture_data["Metallic Map"], texture_data["Normal Map"], \
                                                          texture_data["Roughness Map"]
        create_material_instance(material_name, base_color_t, metallic_t, normal_t, roughness_t)
    else:
        print("Parent", material_properties["parent"])
        skip_counter += 1

print("Skipped", skip_counter, "of", len(materials_data))
