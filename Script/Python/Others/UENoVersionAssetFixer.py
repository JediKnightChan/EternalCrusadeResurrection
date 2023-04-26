import os

import unreal
import json

assets_to_ignore = [
    "VFX/Plasma/EnergyBallVFX/Texture/T_FireSwirlUV_8X8.uasset",
    "VFX/Sky/UltraDynamicSky/Textures/Sky/Real_Stars.uasset",
    "Maps/Levels/MedusaRelay.umap",
    "StarterContent/HDRI/HDRI_Epic_Courtyard_Daylight.uasset",
    "Maps/Levels/FortressHarkus.umap",
    "Maps/Levels/Zedek.umap"
]
asset_root_dir = "/Game/"
json_progress_path = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Others/temp.json"

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()


def update_progress_json():
    with open(json_progress_path, "w") as f:
        json.dump({"processed": processed_paths}, f, ensure_ascii=False, indent=4)


def should_ignore_asset(asset_path):
    asset_path = str(asset_path).lower()
    for ignore_asset in assets_to_ignore:
        ignore_asset = ignore_asset.lower().replace(".uasset", "").replace(".umap", "")
        if asset_path.endswith(ignore_asset):
            return True
    return False


processed_paths = []

if os.path.exists(json_progress_path):
    with open(json_progress_path, "r") as f:
        data = json.load(f)
        processed_paths = data["processed"]
        assets_to_ignore += processed_paths

assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)
for asset in assets:
    print("Got", asset.package_name)
    if should_ignore_asset(asset.package_name):
        print("Ignoring", asset.package_name)
        continue
    else:
        print("Not ignoring", asset.package_name)
        unreal.EditorAssetLibrary.save_asset(asset.package_name, False)
        print("Saved", asset.package_name)
        processed_paths.append(str(asset.package_name))
        update_progress_json()
