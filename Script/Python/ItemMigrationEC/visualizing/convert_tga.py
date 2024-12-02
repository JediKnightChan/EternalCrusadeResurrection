from PIL import Image
import os


def convert_tga_to_png(directory):
    for filename in os.listdir(directory):
        if filename.endswith(".tga"):
            img_path = os.path.join(directory, filename)
            img = Image.open(img_path)
            png_path = os.path.join(directory, filename.replace(".tga", ".png"))
            img.save(png_path, "PNG")
            print(f"Converted {filename} to {png_path}")


# Example usage
convert_tga_to_png("C:/Users/JediKnight/Downloads/ChaosAdvancementsIcons/ChaosAdvancements/")
