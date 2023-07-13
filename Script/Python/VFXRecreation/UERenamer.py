import os.path

import unreal
import json

with open("C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/VFXRecreation/migrating_data.json", "r") as f:
    migration_data = json.load(f)

game_folder = "/Game/VFX/Particles/"
for k, v in migration_data.items():
    src = os.path.join(game_folder, k.replace(".uasset", ""))
    target = os.path.join(game_folder, v.replace(".uasset", ""))
    unreal.EditorAssetLibrary.rename_asset(src, target)