import json
import os

root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/EternalCrusade_dup/Content/Audio/Events/"

play_events = []
stop_events = []
release_events = []
on_events = []

for root, subdirs, files in os.walk(root_dir):
    for file in files:
        # filepath = os.path.join(root, file)

        file = file.replace(".uasset", "")
        if file.startswith("Play_"):
            play_events.append(file)
        elif file.startswith("Stop_"):
            stop_events.append(file)
        elif file.startswith("Release_"):
            release_events.append(file)
        elif file.startswith("On_"):
            on_events.append(file)
        else:
            print(file)

data = {
    "Play": sorted(play_events),
    "Stop": sorted(stop_events),
    "Release": sorted(release_events),
    "On": sorted(on_events)
}

with open("events.json", "w") as f:
    json.dump(data, f, indent=4)
