"""
Script to compare 2 gameplay tags configs
"""

import os

current_file = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Config/DefaultGameplayTags.ini"
new_file = "C:/Users/JediKnight/Downloads/DefaultGameplayTags.ini"

with open(current_file, "r") as f:
    current_lines = [l for l in f.read().split("\n") if l.startswith("+GameplayTagList=")]

with open(new_file, "r") as f:
    new_lines = [l for l in f.read().split("\n") if l.startswith("+GameplayTagList=")]

for line in new_lines:
    if line not in current_lines:
        print(line)
