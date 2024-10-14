import pandas as pd

import unreal

"""Creates Blueprint Classes for accessoires
"""

# Change me!
ASSET_DIR = "/Game/Characters/SpaceMarine/Weapons/Blueprints/AbilityItems/WeaponMods/LSM/"
TABLE_PATH = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/data/ec/gameplay_items/" \
             "lsm_weapon_mods.csv"
GENERAL_TABLE_PATH = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/data/ec/gameplay_items/" \
                     "lsm.csv"


BFL = unreal.SubobjectDataBlueprintFunctionLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def create_blueprint(asset_name, asset_dir, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    blueprint = asset_tools.create_asset(asset_name, asset_dir, None, factory)
    return blueprint


def create_data_asset(asset_name, asset_dir, parent_class):
    my_data_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, asset_dir, parent_class,
                                                                            unreal.DataAssetFactory())
    unreal.EditorAssetLibrary.save_loaded_asset(my_data_asset)
    return my_data_asset


def create_item(asset_name, asset_dir, item_tags):
    # Load parent blueprint class
    mod_class = unreal.EditorAssetLibrary.load_blueprint_class(
        "/Game/Blueprints/ECR/Weapons/AbilityItems/WeaponMods/ECRWeaponMod_BP")
    # Create blueprint asset based on this class
    mod_instance = create_blueprint("EI_" + asset_name, asset_dir, mod_class)

    # Now we want to modify property in that blueprint asset, have to load generated class
    mod_instance_gen_class = unreal.load_object(None, mod_instance.generated_class().get_path_name())
    # Now load CDO (class default object) for generated class
    mod_instance_cdo = unreal.get_default_object(mod_instance_gen_class)
    # Convert strings to gameplay tags
    tag_container = unreal.GameplayTagContainer()
    for tag_str in item_tags:
        gameplay_tag = unreal.ECRPythonHelpersLibrary.convert_string_to_gameplay_tag(tag_str)
        print("TAGS VALUE", unreal.GameplayTagLibrary.get_debug_string_from_gameplay_tag(gameplay_tag))
        tag_container = unreal.GameplayTagLibrary.add_gameplay_tag_to_container(tag_container, gameplay_tag)

    print("TAGS CONTAINER VALUE", unreal.GameplayTagLibrary.get_debug_string_from_gameplay_tag_container(tag_container))

    # Now can set this property for blueprint asset
    mod_instance_cdo.set_editor_property("ModifierTags", tag_container)
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(mod_instance)
    # Get class of this blueprint to mention it in TSubClassOf<...> property
    mod_final_class = unreal.EditorAssetLibrary.load_blueprint_class(
        asset_dir + "/" + "EI_" + asset_name)

    # Load Equipment Definition C++ class
    ed_class = unreal.load_class(None, "/Script/ECR.ECREquipmentDefinition")
    # Create blueprint based on this parent class
    ed_instance = create_blueprint("ED_" + asset_name, asset_dir, ed_class)
    # Now we want to modify property in that blueprint asset, have to load generated class
    ed_instance_gen_class = unreal.load_object(None, ed_instance.generated_class().get_path_name())
    # Now load CDO (class default object) for generated class
    ed_instance_cdo = unreal.get_default_object(ed_instance_gen_class)
    # Now can set this property for blueprint asset
    ed_instance_cdo.set_editor_property("InstanceType", mod_final_class)
    # Loading created blueprint asset class to mention it in TSubClassOf<...>
    ed_instance_final_class = unreal.EditorAssetLibrary.load_blueprint_class(
        asset_dir + "/" + "ED_" + asset_name)
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(ed_instance)

    # Load C++ class of Item Definition
    id_class = unreal.load_class(None, "/Script/ECR.ECRInventoryItemDefinition")
    # Create blueprint asset of that parent class
    id_instance = create_blueprint("ID_" + asset_name, asset_dir, id_class)
    # Doing same as in code for Equipment Definition above to edit a property on it
    id_instance_gen_class = unreal.load_object(None, id_instance.generated_class().get_path_name())
    id_instance_cdo = unreal.get_default_object(id_instance_gen_class)

    # Creating an EditInlineNew object instance (Inventory Fragment: Equippable item)
    id_fragment_class = unreal.load_class(None, "/Script/ECR.InventoryFragment_EquippableItem")
    id_fragment_instance = unreal.new_object(id_fragment_class, id_instance)
    id_fragment_instance.set_editor_property("EquipmentDefinition", ed_instance_final_class)

    id_instance_cdo.set_editor_property("Fragments", [id_fragment_instance])
    unreal.EditorAssetLibrary.save_loaded_asset(id_instance)


df = pd.read_csv(TABLE_PATH)
df = df.set_index("name")
df = df.dropna(axis=0, how='all')

df_general = pd.read_csv(GENERAL_TABLE_PATH)
df_general = df_general.set_index("name")

df = pd.merge(df, df_general, left_index=True, right_index=True, how="left")

items = list(df.reset_index()["name"])
items_tags = list(df.reset_index()["tags"])

met_tags = []
for item, item_tags in zip(items, items_tags):
    item_tags = item_tags.split(" ")
    item_tags = [tag for tag in item_tags if tag.startswith("Mod.")]

    print(item, item_tags)
    create_item(item, ASSET_DIR, item_tags)

    # for tag in item_tags:
    #     if tag not in met_tags:
    #         print(f'+GameplayTagList=(Tag="{tag}",DevComment="")')
    #         met_tags.append(tag)
