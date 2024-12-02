import os
import subprocess

uasset_dir = "C:/Program Files (x86)\Steam/steamapps/common/Warhammer 40,000 - Eternal Crusade/extract/unpacked/EternalCrusade/Content/ProgressionTrees/ChaosAdvancements/"
output_dir = "../data/ec_raw/progression_trees/csm/"
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
