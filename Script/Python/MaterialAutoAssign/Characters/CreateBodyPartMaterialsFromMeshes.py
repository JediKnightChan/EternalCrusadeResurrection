# Should be used with internal UE API in later versions of UE

"""
Automatically create material instances based on mesh names
(as they are similar to material names and texture names)
"""

import unreal
import os

OVERWRITE_EXISTING = False


def set_material_instance_texture(material_instance_asset, material_parameter, texture_path):
    if not unreal.editor_asset_library.does_asset_exist(texture_path):
        unreal.log_warning("Can't find texture: " + texture_path)
        return False
    tex_asset = unreal.editor_asset_library.find_asset_data(texture_path).get_asset()
    return unreal.material_editing_library.set_material_instance_texture_parameter_value(material_instance_asset,
                                                                                         material_parameter, tex_asset)


base_material = unreal.editor_asset_library.find_asset_data("/Game/Materials/ECMasterOpaque")
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()
MaterialEditingLibrary = unreal.material_editing_library
EditorAssetLibrary = unreal.editor_asset_library


# noinspection DuplicatedCode
def create_material_instance(material_instance_folder, mesh_name):
    material_instance_name = "MI_" + mesh_name.replace("RT", "").replace("LT", "")
    material_instance_path = os.path.join(material_instance_folder, material_instance_name)

    if EditorAssetLibrary.does_asset_exist(material_instance_path):
        # material_instance_asset = EditorAssetLibrary.find_asset_data(material_instance_path).get_asset()
        unreal.log("Asset already exists")
        if OVERWRITE_EXISTING:
            EditorAssetLibrary.delete_asset(material_instance_path)
        else:
            return

    material_instance_asset = AssetTools.create_asset(material_instance_name, material_instance_folder,
                                                      unreal.MaterialInstanceConstant,
                                                      unreal.MaterialInstanceConstantFactoryNew())

    # Set parent material
    MaterialEditingLibrary.set_material_instance_parent(material_instance_asset, base_material.get_asset())

    base_color_texture = os.path.join(material_instance_folder,
                                      "T_" + mesh_name.replace("RT", "").replace("LT", "") + "_BC")
    if EditorAssetLibrary.does_asset_exist(base_color_texture):
        set_material_instance_texture(material_instance_asset, "Base Color", base_color_texture)

    mask_texture = os.path.join(material_instance_folder, "T_" + mesh_name.replace("RT", "").replace("LT", "") + "_Mk")
    if EditorAssetLibrary.does_asset_exist(mask_texture):
        set_material_instance_texture(material_instance_asset, "Mask", mask_texture)

    normal_texture = os.path.join(material_instance_folder, "T_" + mesh_name.replace("RT", "").replace("LT", "") + "_N")
    if EditorAssetLibrary.does_asset_exist(normal_texture):
        set_material_instance_texture(material_instance_asset, "Normal", normal_texture)

    roughness_texture = os.path.join(material_instance_folder,
                                     "T_" + mesh_name.replace("RT", "").replace("LT", "") + "_R")
    if EditorAssetLibrary.does_asset_exist(roughness_texture):
        set_material_instance_texture(material_instance_asset, "Roughness", roughness_texture)

    metal_texture = os.path.join(material_instance_folder, "T_" + mesh_name.replace("RT", "").replace("LT", "") + "_M")
    if EditorAssetLibrary.does_asset_exist(metal_texture):
        set_material_instance_texture(material_instance_asset, "Metal", metal_texture)
    EditorAssetLibrary.save_asset(material_instance_path)


asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = '/Game/Characters/SpaceMarine/BodyPart/Torso/'
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)
for asset in assets:
    if asset.asset_class == "SkeletalMesh":
        create_material_instance(str(asset.package_path), str(asset.asset_name))
