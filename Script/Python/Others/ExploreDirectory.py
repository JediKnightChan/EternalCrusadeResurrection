import os.path

dir_to_explore = "D:/MyProjects/eternal_crusade/ExtractedFull/EternalCrusade_uasset/Content/VFX/Particles/"
filename_part = "trail"

for root, dirs, files in os.walk(dir_to_explore):
    for file in files:
        filepath = os.path.join(root, file)
        if filename_part.lower() in filepath.lower():
            print(filepath)
