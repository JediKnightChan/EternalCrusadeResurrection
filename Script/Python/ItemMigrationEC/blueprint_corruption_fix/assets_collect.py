"""This script collects blueprint in-game paths for further processing with clicker. For internal UE API"""

import json
import unreal

# mode = "weapon"
mode = "accessory_or_weapon_mod"

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = "/Game/Characters/SpaceMarine/Weapons/Blueprints/"
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)

package_paths = []
for asset in assets:
    if str(asset.asset_name).startswith("ID_SM") or str(asset.asset_name).startswith("ID_CSM"):
        if mode == "weapon":
                instance = unreal.EditorAssetLibrary.load_asset(asset.package_name)
                parent_class = unreal.ECRPythonHelpersLibrary.get_parent_class(instance)
                if "ECR.ECRInventoryItemDefinition" not in str(parent_class):
                    package_paths.append(str(asset.package_name))
        elif mode == "accessory_or_weapon_mod":
            if "Accessories" in str(asset.package_name) or "WeaponMods" in str(asset.package_name):
                package_paths.append(str(asset.package_name))
        else:
            raise NotImplementedError

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/ItemMigrationEC/temp.json", "w") as f:
    json.dump(package_paths, f, ensure_ascii=False, indent=4)
