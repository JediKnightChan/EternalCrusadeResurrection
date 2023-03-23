import json
import os

latest_s3_key = "sound_sorting/root/SNB_Ambience/001.ogg"
met_latest_s3_key = False

data = {"sounds": []}
sounds = []
root_sound_directory = "./Files"
for root, subdirs, files in os.walk(root_sound_directory):
    for file in sorted(files):
        if file.endswith(".ogg"):
            filepath = os.path.join(root, file).replace("\\", "/")
            s3_key = filepath.replace("\\", "/").replace("./Files/Windows/", "sound_sorting/root/")
            data["sounds"].append(s3_key)

with open("Files/data.json", "w") as f:
    json.dump(data, f, ensure_ascii=False, indent=4)
