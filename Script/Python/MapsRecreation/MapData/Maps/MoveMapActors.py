import unreal

DO_FOR_ALL = False

if DO_FOR_ALL:
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
else:
    actors = unreal.EditorLevelLibrary.get_selected_level_actors()

relative_offset = unreal.Vector(30000, 30000, -50000)
for actor in actors:
    current_location = actor.get_actor_location()
    actor.set_actor_location(current_location + relative_offset, False, False)
