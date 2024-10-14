import os
import subprocess

uasset_dir = "D:/MyProjects/eternal_crusade/ExtractedFull/EternalCrusade_uasset/Content/DataAssets/Characters/SpaceMarine/"
output_dir = "../data/ec_raw/data_assets_chars/lsm/"
test_mode = False

for root, dirs, files in os.walk(uasset_dir, topdown=False):
    for name in files:
        source_fp = os.path.join(root, name).replace("\\", "/")
        dest_fp = os.path.join(output_dir, os.path.relpath(root, uasset_dir),
                               os.path.splitext(name)[0] + ".json").replace("\\", "/")
        print(source_fp, "->", dest_fp)

        if not test_mode:
            os.makedirs(os.path.dirname(dest_fp), exist_ok=True)
            # UAssetGUI.exe tojson <source> <destination> <engine version>
            subprocess.run(
                ["C:/Users/JediKnight/Documents/UAssetGUI/UAssetGUI.exe", "tojson", source_fp, dest_fp, "VER_UE4_12"])
