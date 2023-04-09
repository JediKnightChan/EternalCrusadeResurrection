import os

import cv2
import numpy as np

root_dir = "./Ronan/Raw/"

# (x1, y1) is top left corner of image
x_values = [6, 7, 8]
y_values = [0, 1]

filename_template = "x{x}_y{y}.png"

final_result_filename = "merged.png"

hlines = []
for y_value in y_values:
    horizontal_images = []
    for x_value in x_values:
        filename = filename_template.format(x=x_value, y=y_value)
        png_file = os.path.join(root_dir, filename)
        img = cv2.imread(png_file, -1)
        horizontal_images.append(img)
    hline = cv2.hconcat(horizontal_images)
    hlines.append(hline)

res = cv2.vconcat(hlines)
print(res.shape)
cv2.imwrite(os.path.join(root_dir, final_result_filename), res)
