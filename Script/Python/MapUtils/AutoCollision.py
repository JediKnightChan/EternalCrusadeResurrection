import unreal

""" Should be used with internal UE Python API

Auto generate collision for static meshes"""

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = '/Game/PROPS/'
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)


def set_convex_collision(static_mesh):
    unreal.EditorStaticMeshLibrary.set_convex_decomposition_collisions(static_mesh, 4, 16, 100000)
    unreal.EditorAssetLibrary.save_loaded_asset(static_mesh)


for asset in assets:
    if asset.asset_class == "StaticMesh":
        static_mesh = unreal.EditorAssetLibrary.find_asset_data(asset.object_path).get_asset()
        set_convex_collision(static_mesh)
