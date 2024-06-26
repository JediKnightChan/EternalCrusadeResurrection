import unreal

""" Should be used with internal UE Python API

Auto generate LODs for static meshes. 5.3 API"""

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = '/Game/Vehicles/SpaceMarine/'
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)


def generate_lods(skeletal_mesh):
    unreal.SkeletalMeshEditorSubsystem.regenerate_lod(skeletal_mesh, 3)
    unreal.EditorAssetLibrary.save_loaded_asset(skeletal_mesh)


print(len(assets))
sk_count = 0
for i, asset in enumerate(assets):
    if asset.asset_class_path.asset_name == "SkeletalMesh":
        loaded_asset = unreal.EditorAssetLibrary.find_asset_data(asset.package_name).get_asset()
        sk_count += 1

        if sk_count < 10:
            print(loaded_asset)
        generate_lods(loaded_asset)
