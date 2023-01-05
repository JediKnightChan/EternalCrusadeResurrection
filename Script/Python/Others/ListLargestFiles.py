import os
import tqdm


def get_file_size_in_mb(file_path):
    """ Get size of file at given path in bytes"""
    size = os.path.getsize(file_path)
    return size / 1000 / 1000


filepath_to_size = {}

root_dir = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Content/"
for root, dirs, files in os.walk(root_dir):
    for file in files:
        filepath = os.path.join(root, file)
        mb_size = get_file_size_in_mb(filepath)
        filepath_to_size[filepath.replace(root_dir, "").replace("\\", "/")] = mb_size

n = 20
for i, item in enumerate(sorted(filepath_to_size.items(), key=lambda p: p[1], reverse=True)):
    print(item)
    print(f"git lfs track '{item[0]}'")
    if i == n:
        break
