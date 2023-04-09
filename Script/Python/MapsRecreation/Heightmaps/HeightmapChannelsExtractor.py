import os
import cv2
import numpy as np

from PIL import Image


def extract_heightmap(png_filepath, heightmap_filepath):
    im = Image.open(png_filepath)
    im = np.asarray(im).astype("uint16")

    first_channel = 0
    second_channel = 1
    g = im[:, :, first_channel] << 8
    g = g + im[:, :, second_channel].astype("uint16")
    cv2.imwrite(heightmap_filepath, g.astype(np.uint16))


if __name__ == '__main__':
    one_file = True
    root_dir = "Ronan/"

    if one_file:
        filename = "Merged.png"
        new_filename = "Heightmap.png"
        filepaths_to_heightmap_filepaths = {os.path.join(root_dir, filename): os.path.join(root_dir, new_filename)}
    else:
        png_dir = os.path.join(root_dir, "PNG")
        heightmap_dir = os.path.join(root_dir, "Extracted")
        filenames = os.listdir(png_dir)

        filepaths_to_heightmap_filepaths = {
            os.path.join(png_dir, filename): os.path.join(heightmap_dir, filename) for
            filename in filenames}

    for png_filepath, heightmap_filepath in filepaths_to_heightmap_filepaths.items():
        extract_heightmap(png_filepath, heightmap_filepath)
