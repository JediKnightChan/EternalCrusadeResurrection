import json
import re

import unreal

""" Should be used with internal UE Python API

Replace all actors of given asset in the level by another asset, saving coords, rotation and scale"""

level_actors = unreal.EditorLevelLibrary.get_all_level_actors()


def get_path_from_ref(string):
    path = re.search(r"'(?P<path>.+)'", string).group("path")
    return path


with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/MapUtils/assets_to_replace.json", "r") as f:
    data = json.load(f)

for asset_to_replace_from, asset_to_replace_to in data.items():
    # asset_to_replace_from = "/Script/Engine.StaticMesh'/Game/PROPS/Gameplay/SM_PROP_Cover_9x3x3_Promethium_Tank_01.SM_PROP_Cover_9x3x3_Promethium_Tank_01'"
    # asset_to_replace_to = "/Script/Engine.Blueprint'/Game/Blueprints/InWorldAssets/VaultOvers/High/B_WA_VaultOn_High_Cover_9x3x3_Promethium_Tank_01.B_WA_VaultOn_High_Cover_9x3x3_Promethium_Tank_01'"
    actors_to_replace = []

    asset_to_replace_from = get_path_from_ref(asset_to_replace_from)
    asset_to_replace_to = get_path_from_ref(asset_to_replace_to)

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
