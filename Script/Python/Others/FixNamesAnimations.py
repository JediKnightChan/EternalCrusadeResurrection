""" Should be used with internal UE Python API

Fixes animations names that are bad because of import naming mistake"""

import os

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = '/Game/Characters/SpaceMarine/Animations/AnimSequences/'
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)

rename_data = []
for asset in assets:
    base_name = os.path.basename(str(asset.package_name))
    if base_name.startswith("AO_"):
        print(str(asset.package_name))
    else:
        continue

    new_base_name = "AS" + base_name[2:]
    print(new_base_name)
    rnd = unreal.AssetRenameData(asset=asset.get_asset(),
                                 new_package_path=asset.package_path,
                                 new_name=new_base_name)
    rename_data.append(rnd)

    new_path = os.path.join(os.path.dirname(str(asset.package_name)), new_base_name)
    if unreal.EditorAssetLibrary.does_asset_exist(new_path):
        unreal.EditorAssetLibrary.delete_asset(new_path)

at = unreal.AssetToolsHelpers().get_asset_tools()
at.rename_assets(rename_data)
