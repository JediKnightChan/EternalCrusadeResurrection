# Should be used inside Blender

"""
Script to import normal skeleton bones names (joint_Pelvis_01) instead of
indices names (Bone.002), needed because UE 4.12 can export animation with indices names.

ATTENTION!: Armature should be renamed manually to joint_Char along with Object!
"""

import bpy
import json

root_dir = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Blender/BoneStructureRecreation"
with open(f"{root_dir}/space_marine_bone_mapping.json", "r") as f:
    original_bone_names_to_indices_names = json.load(f)

indices_names_to_original_bone_names = {v: k for k, v in original_bone_names_to_indices_names.items()}

# Object should be selected in Blender
context = bpy.context
obj = context.object

for i, bone in enumerate(obj.pose.bones):
    if bone.name in indices_names_to_original_bone_names:
        bone.name = indices_names_to_original_bone_names[bone.name]
    else:
        print(f"Warning: bone {bone.name} not found in mapping!")
