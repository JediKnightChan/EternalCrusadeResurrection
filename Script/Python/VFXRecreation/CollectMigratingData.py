# I manually duplicated all particle systems in
# 4.12 with Ctrl+W, now let's collect dictionary new file -> old_file
import json
import os


def get_relative_path(path, root_dir):
    return path.replace(root_dir, "").strip("/\\")


if __name__ == '__main__':
    original_dir = "D:/MyProjects/eternal_crusade/umodel_needed/EternalCrusade_dup/Content/VFX/Particles"
    game_4_12_dir = "D:/MyProjects/In His Name/3D/UE/Test12/Content/VFX/Particles"

    new_files_to_old_names = {}

    for root, _, files in os.walk(game_4_12_dir):
        orig_root = root.replace(game_4_12_dir, original_dir)

        for file in files:
            fp = os.path.join(root, file)
            original_fp = fp.replace(game_4_12_dir, original_dir)

            if not os.path.exists(original_fp):
                # New file
                basename = file.split(".")[0]

                # First try: remove _2
                basename_orig = basename.replace("_2", "")
                if os.path.exists(os.path.join(orig_root, basename_orig + ".uasset")):
                    new_files_to_old_names[get_relative_path(fp, game_4_12_dir)] = \
                        get_relative_path(os.path.join(orig_root, basename_orig + ".uasset"), original_dir)
                    continue

                if basename.endswith("2"):
                    # Second_try: remove 2
                    basename_orig = basename[:-1]
                    if os.path.exists(os.path.join(orig_root, basename_orig + ".uasset")):
                        new_files_to_old_names[get_relative_path(fp, game_4_12_dir)] = \
                            get_relative_path(os.path.join(orig_root, basename_orig + ".uasset"), original_dir)
                        continue

                    # Third try: replace 2 by 1
                    basename_orig = basename[:-1] + "1"
                    if os.path.exists(os.path.join(orig_root, basename_orig + ".uasset")):
                        new_files_to_old_names[get_relative_path(fp, game_4_12_dir)] = \
                            get_relative_path(os.path.join(orig_root, basename_orig + ".uasset"), original_dir)

                        continue

                    # Fourth try: remove _02
                    if basename.endswith("_02"):
                        basename_orig = basename[:-3]
                        if os.path.exists(os.path.join(orig_root, basename_orig + ".uasset")):
                            new_files_to_old_names[get_relative_path(fp, game_4_12_dir)] = \
                                get_relative_path(os.path.join(orig_root, basename_orig + ".uasset"), original_dir)

                            continue
                print(f"Couldn't find orig for {basename}")

    with open("migrating_data.json", "w") as f:
        json.dump(new_files_to_old_names, f, indent=4)
