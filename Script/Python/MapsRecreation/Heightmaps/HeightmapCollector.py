import os

import cv2
import numpy as np

root_dir = "Medusa/Raw/"

x_values = [3, 4, 5]
y_values = [1, 2, 3]

dirname_template = "Territory02_x{x}_y{y}"
sub_dirs = os.listdir(root_dir)

for y_value in y_values:
    for x_value in x_values:
        dir_name = dirname_template.format(x=x_value, y=y_value)
        dir_path = os.path.join(root_dir, dir_name)
        dir_files = os.listdir(dir_path)
        tga_file = None
        for file in dir_files:
            if file.lower().endswith(".tga"):
                tga_file = os.path.abspath(os.path.join(dir_path, file))
        if not tga_file:
            raise FileNotFoundError("TGA file not found")
        new_tga_filepath = os.path.abspath(os.path.join(dir_path, "../..", "..", "All", dir_name + ".tga"))
        print(new_tga_filepath)
        os.rename(tga_file, new_tga_filepath)

