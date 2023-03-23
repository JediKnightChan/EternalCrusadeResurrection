import unreal

""" Should be used with internal UE Python API

Replace all actors of given asset in the level by another asset, saving coords, rotation and scale"""

level_actors = unreal.EditorLevelLibrary.get_all_level_actors()

asset_to_replace_from = "/Game/PROPS/Ambiance/" \
                        "SM_PROP_AMB_EnergyCell_01_a.SM_PROP_AMB_EnergyCell_01_a"
asset_to_replace_to = "/Game/Blueprints/InWorldAssets/VaultOvers/OverLows/" \
                      "B_WA_VaultOver_Low_AmbEnergyCell_1A.B_WA_VaultOver_Low_AmbEnergyCell_1A"
actors_to_replace = []

for actor in level_actors:
    if (isinstance(actor, unreal.StaticMeshActor)):
        if not actor.static_mesh_component or not actor.static_mesh_component.static_mesh:
            continue
        asset_path = actor.static_mesh_component.static_mesh.get_path_name()
        if asset_path == asset_to_replace_from:
            actors_to_replace.append(
                (actor, actor.get_actor_location(), actor.get_actor_rotation(), actor.get_actor_scale3d()))

new_asset = unreal.EditorAssetLibrary.find_asset_data(asset_to_replace_to).get_asset()
for i in reversed(range(len(actors_to_replace))):
    actor, loc, rot, scale = actors_to_replace[i]
    unreal.EditorLevelLibrary.destroy_actor(actor)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_object(new_asset, loc, rot)
    actor.set_actor_scale3d(scale)
