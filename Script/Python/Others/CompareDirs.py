import json
import os
import shutil

full_dir = "C:/Program Files (x86)/Steam/steamapps/common/Warhammer 40,000 - Eternal Crusade/extract/project/EternalCrusade/Content/VFX/Particles"
part_dir = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Content/VFX/Particles"
new_dir = "C:/Users/JediKnight/Documents/Unreal Projects/Test12/Content/VFX/Particles"

def get_modified_name(original_name, all_original_names, new_mapping):
    while True:
        if original_name[-1].isdigit():
            digit = int(original_name[-1])
            new_name = original_name[:-1] + str(digit + 1)
        else:
            digit = 2
            new_name = original_name + str(digit)
        if new_name not in all_original_names and new_name not in list(new_mapping.values()):
            return new_name
        original_name = new_name


original_names = []
original_names_to_modified = {}
original_names_to_modified_moved = {}

existing = 0
missing = 0
for root, subdirs, files in os.walk(full_dir):
    for file in files:
        first_fp = os.path.join(root, file).replace("\\", "/")
        original_name = first_fp.replace(full_dir, "").strip("/").replace(".uasset", "")
        original_names.append(original_name)
# print(original_names)

for root, subdirs, files in os.walk(full_dir):
    for file in files:
        first_fp = os.path.join(root, file).replace("\\", "/")
        second_fp = first_fp.replace(full_dir, part_dir)

        if os.path.exists(second_fp):
            existing += 1
        else:
            missing += 1
            # print(second_fp, "missing")
            original_name = first_fp.replace(full_dir, "").strip("/").replace(".uasset", "")
            new_name = get_modified_name(original_name, original_names, original_names_to_modified)
            original_names_to_modified[original_name] = new_name

            third_path = first_fp.replace(full_dir, new_dir).replace(original_name, new_name)
            if os.path.exists(third_path):
                print("Exists", third_path, original_name)
                os.makedirs(os.path.dirname(second_fp), exist_ok=True)
                shutil.copy(third_path, second_fp.replace(original_name, new_name))
                original_names_to_modified_moved[original_name] = new_name
                pass
            else:
                # print("Not exist", third_path, original_name)
                pass
# print(original_names_to_modified)
with open("temp.json", "w") as f:
    json.dump(original_names_to_modified_moved, f)
