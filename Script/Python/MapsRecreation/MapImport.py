# Should be used with internal UE API in later versions of UE

"""Spawn actors on the map based on map export data produced by MapExport.py"""

import unreal
import json
import re


map_data_filepath = "C:/Users/JediKnight/Documents/zedek1_main.json"
level_library = unreal.EditorLevelLibrary
editor_asset_library = unreal.EditorAssetLibrary
relative_offset = unreal.Vector(-167598.703125, -1021464.375, +96750.0)

with open(map_data_filepath, "rb") as f:
    data = json.load(f)


for element in data:
    path, transform = element["path"], element["transform"]
    path = re.search(r"\w+\s(?P<path>[\/\w]+).\w+", path).group("path")
    print(path)
    if editor_asset_library.does_asset_exist(path):
        if "rock" not in path.lower():
            continue
        asset = editor_asset_library.find_asset_data(path).get_asset()
        actor = level_library.spawn_actor_from_object(asset, transform["loc"], transform["rot"])
        current_location = actor.get_actor_location()
        actor.set_actor_location(current_location + relative_offset, False, False)
    else:
        print(f"Asset {path} doesn't exist")
