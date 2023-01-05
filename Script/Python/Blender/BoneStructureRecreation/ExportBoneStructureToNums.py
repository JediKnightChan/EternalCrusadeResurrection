# Should be used inside Blender

"""
Script to export mapping of bones from original skeleton bones names (joint_Pelvis_01) to
indices names (Bone.002), needed because UE 4.12 can export animation with indices names
"""

import bpy
import json

# Object should be selected in Blender
context = bpy.context
obj = context.object

original_bone_names_to_indices_names = {}

for i, bone in enumerate(obj.pose.bones):
    print(bone.name)
    index_name = f"Bone.{i:03d}" if i != 0 else "Bone"
    original_bone_names_to_indices_names[bone.name] = index_name

root_dir = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/Blender/BoneStructureRecreation"
with open(f"{root_dir}/space_marine_bone_mapping.json", "w") as f:
    json.dump(original_bone_names_to_indices_names, f, indent=4)
