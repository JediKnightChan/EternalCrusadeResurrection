import pandas as pd
import unreal

"""Creates Blueprint Classes for accessoires
"""

# Change me!
ASSET_DIR = "/Game/Characters/SpaceMarine/Weapons/Blueprints/AbilityItems/Accessories/LSM/"
TABLE_PATH = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/data/ec/gameplay_items/" \
             "lsm_accessories.csv"

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


def create_item(asset_name, asset_dir):
    # Load parent blueprint class
    ge_class = unreal.EditorAssetLibrary.load_blueprint_class(
        "/Game/Blueprints/ECR/GAS/GameplayEffects/Attributes/Accessories/GE_Accessory")
    # Create blueprint asset based on this class
    ge_instance = create_blueprint("GE_" + asset_name, asset_dir, ge_class)
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(ge_instance)
    # Get class of this blueprint to mention it in TSubClassOf<...> property
    ge_final_class = unreal.EditorAssetLibrary.load_blueprint_class(
        asset_dir + "/" + "GE_" + asset_name)

    # Load custom DataAsset class
    as_class = unreal.load_class(None, "/Script/ECR.ECRAbilitySet")
    # Create data asset
    as_instance = create_data_asset("AS_" + asset_name, asset_dir, as_class)
    # Create C++ struct
    s = unreal.ECRAbilitySet_GameplayEffect()
    # Set its property, which should be EditAnywhere
    s.set_editor_property("GameplayEffect", ge_final_class)
    # Set property in data asset to this struct
    as_instance.set_editor_property("GrantedGameplayEffects", [s])
    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(as_instance)

    # Load Equipment Definition C++ class
    ed_class = unreal.load_class(None, "/Script/ECR.ECREquipmentDefinition")
    # Create blueprint based on this parent class
    ed_instance = create_blueprint("ED_" + asset_name, asset_dir, ed_class)
    # Now we want to modify property in that blueprint asset, have to load generated class
    ed_instance_gen_class = unreal.load_object(None, ed_instance.generated_class().get_path_name())
    # Now load CDO (class default object) for generated class
    ed_instance_cdo = unreal.get_default_object(ed_instance_gen_class)
    # Now can set this property for blueprint asset
    ed_instance_cdo.set_editor_property("AbilitySetsToGrant", [as_instance])
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
items = list(df.reset_index()["name"])

for item in items:
    folder = item.split("_")[1]
    print(folder, item)
    create_item(item, ASSET_DIR + folder)
