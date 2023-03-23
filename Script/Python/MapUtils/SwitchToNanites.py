import unreal

""" Should be used with internal UE Python API

Switch to/from nanites on static meshes"""

static_mesh_editor_subsystem = unreal.get_editor_subsystem(
    unreal.StaticMeshEditorSubsystem)

# Change me!
USE_NANITES = False
asset_root_dir = '/Game/Buildings/'

asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()

assets = asset_reg.get_assets_by_path(asset_root_dir, recursive=True)

static_mesh_data = {}
for asset in assets:
    if asset.asset_class == "StaticMesh":
        asset_path = asset.object_path
        asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path).get_asset()
        static_mesh_data[asset_path] = asset_data

for static_mesh_path, static_mesh in static_mesh_data.items():
    nanite_setting = static_mesh_editor_subsystem.get_nanite_settings(static_mesh)
    if nanite_setting.enabled == USE_NANITES:
        print("Already done", static_mesh)
        continue
    print("Enabing", static_mesh)

    nanite_setting.enabled = USE_NANITES
    static_mesh_editor_subsystem.set_nanite_settings(static_mesh, nanite_setting, True)

    unreal.EditorAssetLibrary.save_asset(static_mesh_path, False)
