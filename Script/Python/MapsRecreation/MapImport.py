# Should be used with internal UE API in later versions of UE

import unreal
import json
import re


map_data_filepath = "zedek1_main.json"
level_library = unreal.EditorLevelLibrary
editor_asset_library = unreal.editor_asset_library


with open(map_data_filepath, "rb") as f:
    data = json.load(f)


for element in data:
    path, transform = element["path"], element["transform"]
    path = re.search(r"\w+\s(?P<path>[\/\w]+).\w+", path).group("path")
    print(path)
    if editor_asset_library.does_asset_exist(path):
        if "rock" in path.lower():
            continue
        asset = editor_asset_library.find_asset_data(path).get_asset()
        level_library.spawn_actor_from_object(asset, transform["loc"], transform["rot"])
    else:
        print(f"Asset {path} doesn't exist")
