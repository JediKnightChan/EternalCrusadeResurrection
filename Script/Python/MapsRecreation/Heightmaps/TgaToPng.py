import os

from PIL import Image

DO_DELETE_TGAS = True
root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/exp2/Maps/Levels/FortressAgnathio/Landscape/Territory05_x0_y2"

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
