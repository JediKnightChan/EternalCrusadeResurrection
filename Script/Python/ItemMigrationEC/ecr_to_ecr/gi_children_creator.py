import json
import os

import pandas as pd

import unreal

"""Creates Blueprint Classes for weapon skins
"""

MAPPING_PATH = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/ecr_to_ecr/temp.json"

BFL = unreal.SubobjectDataBlueprintFunctionLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def create_blueprint(asset_name, asset_dir, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    blueprint = asset_tools.create_asset(asset_name, asset_dir, None, factory)
    return blueprint


def create_item(child_id_name, parent_id_name, id_dir, item_row_name):
    if not id_dir or not child_id_name or not parent_id_name:
        return

    child_id_fp = os.path.join(id_dir, child_id_name).replace("\\", "/")
    parent_id_fp = os.path.join(id_dir, parent_id_name).replace("\\", "/")

    # Load parent blueprint class
    parent_id_class = unreal.EditorAssetLibrary.load_blueprint_class(parent_id_fp)
    # Create blueprint asset based on this class
    parent_id_instance = unreal.EditorAssetLibrary.load_asset(parent_id_fp)

    # Now we want to modify property in that blueprint asset, have to load generated class
    parent_id_instance_gen_class = unreal.load_object(None, parent_id_instance.generated_class().get_path_name())
    # Now load CDO (class default object) for generated class
    parent_id_instance_cdo = unreal.get_default_object(parent_id_instance_gen_class)

    parent_ed_gen_class = None
    for fragment in parent_id_instance_cdo.get_editor_property("Fragments"):
        try:
            if fragment.get_editor_property("EquipmentDefinition"):
                parent_ed_gen_class = fragment.get_editor_property("EquipmentDefinition")
        except:
            continue

    if parent_ed_gen_class is None:
        return

    parent_ed_gen_class_cdo = unreal.get_default_object(parent_ed_gen_class)
    parent_wi_gen_class = parent_ed_gen_class_cdo.get_editor_property("InstanceType")
    parent_default_weapon_actor_gen_class = parent_ed_gen_class_cdo.get_editor_property("ActorsToSpawn")[
        0].get_editor_property("ActorSelectionSet").get_editor_property("default_actor_class")

    parent_default_weapon_actor_gen_class = unreal.EditorAssetLibrary.load_blueprint_class(
        parent_default_weapon_actor_gen_class.get_path_name()[:-2].replace("_SM_", "_CSM_"))

    new_weapon_actor_suffix = "_" + "_".join(item_row_name.split("_")[2:])
    new_weapon_actor_suffix = new_weapon_actor_suffix.replace("_T2", "_MasterCrafted")
    new_weapon_actor_suffix = new_weapon_actor_suffix.replace("_StandardV2", "_Alt")

    new_weapon_actor_class_name = parent_default_weapon_actor_gen_class.get_name()[:-2] + new_weapon_actor_suffix
    new_ed_class_name = parent_ed_gen_class.get_name()[:-2] + child_id_name.replace(
        parent_id_name, "")
    new_wi_class_name = parent_wi_gen_class.get_name()[:-2] + child_id_name.replace(
        parent_id_name, "")

    print("Creating", item_row_name, new_weapon_actor_class_name, new_ed_class_name, new_wi_class_name)

    if child_id_name == parent_id_name:
        if unreal.EditorAssetLibrary.does_asset_exist(
                os.path.join(id_dir, new_weapon_actor_class_name).replace("\\", "/")):
            return
        # No separate ID, ED, WI are needed, just need child weapon actor
        create_blueprint(new_weapon_actor_class_name, id_dir, unreal.EditorAssetLibrary.load_blueprint_class(
            parent_default_weapon_actor_gen_class.get_path_name()[:-2]))
        pass
    else:
        if unreal.EditorAssetLibrary.does_asset_exist(
                os.path.join(id_dir, new_weapon_actor_class_name).replace("\\", "/")):
            return

        # Need to create ID, ED, WI, actor
        # Creating weapon actor
        new_weapon_actor_instance = create_blueprint(new_weapon_actor_class_name, id_dir,
                                                     unreal.EditorAssetLibrary.load_blueprint_class(
                                                         parent_default_weapon_actor_gen_class.get_path_name()[:-2]))
        new_weapon_actor_final_class = unreal.EditorAssetLibrary.load_blueprint_class(
            new_weapon_actor_instance.get_path_name())

        if unreal.EditorAssetLibrary.does_asset_exist(
                os.path.join(id_dir, new_wi_class_name).replace("\\", "/")):
            return

        # Create WI
        new_wi_instance = create_blueprint(new_wi_class_name, id_dir, unreal.EditorAssetLibrary.load_blueprint_class(
            parent_wi_gen_class.get_path_name()[:-2]))

        if unreal.EditorAssetLibrary.does_asset_exist(
                os.path.join(id_dir, new_ed_class_name).replace("\\",
                                                                "/")) or unreal.EditorAssetLibrary.does_asset_exist(
            os.path.join(id_dir, child_id_name).replace("\\", "/")):
            return

        # Create ED
        new_ed_instance = create_blueprint(new_ed_class_name, id_dir, unreal.EditorAssetLibrary.load_blueprint_class(
            parent_ed_gen_class.get_path_name()[:-2]))
        new_ed_instance_gen_class = unreal.load_object(None, new_ed_instance.generated_class().get_path_name())
        new_ed_instance_cdo = unreal.get_default_object(new_ed_instance_gen_class)
        # Now can set this property for blueprint asset
        new_ed_instance_cdo.set_editor_property("InstanceType", unreal.EditorAssetLibrary.load_blueprint_class(
            new_wi_instance.get_path_name()))

        socket = new_ed_instance_cdo.get_editor_property("ActorsToSpawn")[0].get_editor_property("AttachSocket")
        a = unreal.ECRActorSelectionSet()
        a.set_editor_property("DefaultActorClass", new_weapon_actor_final_class)

        s = unreal.ECREquipmentActorToSpawn()
        s.set_editor_property("ActorSelectionSet", a)
        s.set_editor_property("AttachSocket", socket)
        new_ed_instance_cdo.set_editor_property("ActorsToSpawn", [s])

        # Create ID
        new_id_instance = create_blueprint(child_id_name, id_dir, unreal.EditorAssetLibrary.load_blueprint_class(
            parent_id_instance_gen_class.get_path_name()[:-2]))
        new_id_instance_gen_class = unreal.load_object(None, new_id_instance.generated_class().get_path_name())
        new_id_instance_cdo = unreal.get_default_object(new_id_instance_gen_class)
        # Now can set this property for blueprint asset
        fragments = new_id_instance_cdo.get_editor_property("Fragments")
        new_fragments = []
        for f in fragments:
            if "InventoryFragment_EquippableItem" not in str(f):
                new_fragments.append(f)
            else:
                id_fragment_class = unreal.load_class(None, "/Script/ECR.InventoryFragment_EquippableItem")
                id_fragment_instance = unreal.new_object(id_fragment_class, new_id_instance)
                id_fragment_instance.set_editor_property("EquipmentDefinition",
                                                         unreal.EditorAssetLibrary.load_blueprint_class(
                                                             new_ed_instance.get_path_name()))
                new_fragments.append(id_fragment_instance)

        new_id_instance_cdo.set_editor_property("Fragments", new_fragments)


with open(MAPPING_PATH, "r") as f:
    mapping = json.load(f)

for item_name, item_data in mapping.items():
    create_item(item_data["file"], item_data["parent_file"], item_data["dir"], item_name)
