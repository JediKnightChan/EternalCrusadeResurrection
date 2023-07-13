import os
import json
import shutil

if __name__ == '__main__':
    game_4_12_dir = "D:/MyProjects/In His Name/3D/UE/Test12/Content/VFX/Particles"
    game_5_01_dir = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Content/VFX/Particles"

    with open("migrating_data.json", "r") as f:
        migrating_data = json.load(f)

    print(migrating_data)
    for k, v in migrating_data.items():
        orig_fp = os.path.join(game_4_12_dir, k)
        new_fp = os.path.join(game_5_01_dir, k)

        os.makedirs(os.path.dirname(new_fp), exist_ok=True)
        
        print(f"Copying {orig_fp} to {new_fp}")
        shutil.copy(orig_fp, new_fp)