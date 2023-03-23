import unreal

""" Should be used with internal UE Python API

Auto generate LODs for static meshes"""

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
asset_root_dir = '/Game/Buildings/'
assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)


def generate_lods(static_mesh):
    number_of_vertices = unreal.EditorStaticMeshLibrary.get_number_verts(static_mesh, 0)
    if number_of_vertices < 10:
        return
    options = unreal.EditorScriptingMeshReductionOptions()
    options.reduction_settings = [
        unreal.EditorScriptingMeshReductionSettings(1.0, 1.0),
        unreal.EditorScriptingMeshReductionSettings(0.8, 0.75),
        unreal.EditorScriptingMeshReductionSettings(0.6, 0.5),
        unreal.EditorScriptingMeshReductionSettings(0.4, 0.25)
    ]
    options.auto_compute_lod_screen_size = True
    unreal.EditorStaticMeshLibrary.set_lods(static_mesh, options)
    unreal.EditorAssetLibrary.save_loaded_asset(static_mesh)


for asset in assets:
    if asset.asset_class == "StaticMesh":
        loaded_asset = unreal.EditorAssetLibrary.find_asset_data(asset.object_path).get_asset()
        generate_lods(loaded_asset)
