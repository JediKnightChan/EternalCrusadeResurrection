import json
import os

import unreal

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Others/temp.json") as f:
    mapping = json.load(f)

base_dir = "/Game/VFX/Particles/"

rename_data = []
for k, v in mapping.items():
    unreal.EditorAssetLibrary.rename_asset(base_dir + v, base_dir + k)

    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    asset = ar.get_asset_by_object_path(base_dir + v)

    new_package_path = os.path.dirname(base_dir + k)
    new_name = k.split("/")[-1]
    rnd = unreal.AssetRenameData(asset=asset.get_asset(),
                                 new_package_path=new_package_path,
                                 new_name=new_name)
    print("new path", new_package_path, "new name", new_name)
    rename_data.append(rnd)

at = unreal.AssetToolsHelpers().get_asset_tools()
at.rename_assets(rename_data)
