import os

from PIL import Image

DO_DELETE_TGAS = True
root_dir = "./Ronan/"

walk_iterator = list(os.walk(root_dir))
for root, subdirs, files in walk_iterator:
    for file in files:
        if file.lower().endswith(".tga"):
            tga_filepath = os.path.join(root, file)
            img = Image.open(tga_filepath)
            png_filepath = os.path.join(root, file.replace(".tga", ".png").replace(".TGA", ".png"))
            img.save(png_filepath)

            if DO_DELETE_TGAS:
                os.remove(tga_filepath)
