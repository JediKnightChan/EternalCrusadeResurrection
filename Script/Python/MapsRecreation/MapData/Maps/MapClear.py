# Should be used with internal UE API in later versions of UE

"""Delete all actors from the map"""

import unreal


level_library = unreal.EditorLevelLibrary
editor_asset_library = unreal.EditorAssetLibrary

all_actors = level_library.get_all_level_actors()
print("All actors length is", len(all_actors))

for actor in all_actors:
    if actor.get_actor_location().y > -30000:
        level_library.destroy_actor(actor)
    # if "rock" in actor.get_path_name().lower():
    #     level_library.destroy_actor(actor)
