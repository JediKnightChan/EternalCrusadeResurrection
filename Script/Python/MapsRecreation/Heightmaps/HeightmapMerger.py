# Should be used outside UE

"""Script to merge two landscapes into one by selecting the highest point (pixel) between two height maps"""

import cv2
import numpy as np

height_map_filepath1 = "C:/Users/JediKnight/Documents/arkainl.png"
height_map_filepath2 = "C:/Users/JediKnight/Documents/mountainsl.png"
destination_filepath = "C:/Users/JediKnight/Documents/mergedl.png"

l1 = cv2.imread(height_map_filepath1, -1)
l2 = cv2.imread(height_map_filepath2, -1)
merged_l = np.maximum(l1, l2)

print("Merged shape", merged_l.shape)
cv2.imwrite(destination_filepath, merged_l)
