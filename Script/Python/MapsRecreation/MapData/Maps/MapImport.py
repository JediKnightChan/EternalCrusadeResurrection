"""Spawn actors on the map based on map export data produced by MapExport.py. Should be used with internal UE API"""

import os.path

import unreal
import json
import re

# Change me!
map_data_filepath = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/MapsRecreation/" \
                    "MapData/Maps/Usual/HiveComplex01/x01_static.json"
PATH_TO_FILTER = None
EXCLUDE_ENGINE_ASSETS = False
level_library = unreal.EditorLevelLibrary
editor_asset_library = unreal.EditorAssetLibrary
relative_offset_loc = unreal.Vector(0, 0, 0)  # + unreal.Vector(25400,
#              -76200,
#             0)
# relative_offset_loc = unreal.Vector(0, 0, 0)

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/MapsRecreation/"
          "MapData/Maps/path_replacing_map.json", "r") as f:
    path_replacing_map = json.load(f)

with open(map_data_filepath, "rb") as f:
    data = json.load(f)

for element in data:
    path, transform = element["path"], element["transform"]
    path = re.search(r"\w+\s(?P<path>[\/\w]+).\w+", path).group("path")
    path = path_replacing_map.get(path, path)

    if PATH_TO_FILTER and path != PATH_TO_FILTER:
        continue

    print(path)
    if path.startswith("/") and editor_asset_library.does_asset_exist(path):
        if path.startswith("/Engine/") and EXCLUDE_ENGINE_ASSETS:
            continue
        asset = editor_asset_library.find_asset_data(path).get_asset()
        actor = level_library.spawn_actor_from_object(asset, transform["loc"])
        if actor:
            actor.set_actor_rotation(
                unreal.Rotator(pitch=transform["rot"][0], roll=transform["rot"][1], yaw=transform["rot"][2]), True)
            actor.set_actor_scale3d(transform["scale"])
            current_location = actor.get_actor_location()
            actor.set_actor_location(current_location + relative_offset_loc, False, False)
        else:
            print(f"Error: Couldn't spawn actor for {path}")
    else:
        print(f"Error: Asset {path} doesn't exist")
