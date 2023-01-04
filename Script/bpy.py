"""
Dummy Blender library (bpy) simulator for running scripts outside blender with testing purposes
"""


class BlenderPose:
    bones = []


class BlenderObject:
    pose = BlenderPose()


class BlenderContext:
    object = BlenderObject()


context = BlenderContext()
