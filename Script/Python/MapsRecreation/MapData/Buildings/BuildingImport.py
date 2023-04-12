import json
import os.path
import re

import unreal

"""Creates Actor blueprint with static mesh components from json. WARNING: Works only in UE 5.1 internal API
"""

# Change me!
DO_DELETE_IF_EXISTS = False
ASSET_DIR = "/Game/Buildings/Blueprints/"
# ASSET_DIR = "/Game/PROPS/GamePlay/"
# ASSET_DIR = "/Game/Graybox/Buildings/"
JSON_FILEPATH = "C:/Users/JediKnight/Documents/Unreal Projects/ECR/Script/Python/MapsRecreation/" \
                "MapData/Buildings/Files/BP_CaputurePoint_Building_48x48.json"
ASSET_NAME_OVERRIDE = ""
if ASSET_NAME_OVERRIDE:
    ASSET_NAME = ASSET_NAME_OVERRIDE
else:
    ASSET_NAME = os.path.basename(JSON_FILEPATH).replace(".json", "")


BFL = unreal.SubobjectDataBlueprintFunctionLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
ASSET_PATH = os.path.join(ASSET_DIR, ASSET_NAME)


def get_sub_handle_object(sub_handle):
    obj = BFL.get_object(BFL.get_data(sub_handle))
    return obj


def add_component(root_data_handle, subsystem, blueprint, new_class, name):
    sub_handle, fail_reason = subsystem.add_new_subobject(
        params=unreal.AddNewSubobjectParams(
            parent_handle=root_data_handle,
            new_class=new_class,
            blueprint_context=blueprint
        )
    )
    if not fail_reason.is_empty():
        raise Exception(f"ERROR from sub_object_subsystem.add_new_subobject: {fail_reason}")

    subsystem.rename_subobject(handle=sub_handle, new_name=unreal.Text(name))
    subsystem.attach_subobject(owner_handle=root_data_handle, child_to_add_handle=sub_handle)
    obj = BFL.get_object(BFL.get_data(sub_handle))
    return sub_handle, obj


def create_actor_blueprint(asset_name, asset_dir):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.Actor)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    blueprint = asset_tools.create_asset(asset_name, asset_dir, None, factory)
    return blueprint


def add_components_to_blueprint(blueprint):
    root_data_handle = SDS.k2_gather_subobject_data_for_blueprint(blueprint)[0]
    scene_handle, scene = add_component(root_data_handle, SDS, blueprint, unreal.SceneComponent, name="Scene")

    with open(JSON_FILEPATH, "rb") as f:
        data = json.load(f)

    for i, element in enumerate(data):
        path, transform = element["path"], element["transform"]
        path = re.search(r"\w+\s(?P<path>[\/\w]+).\w+", path).group("path")

        if path.startswith("/") and unreal.EditorAssetLibrary.does_asset_exist(path):
            sub_handle, static_mesh_comp = add_component(
                scene_handle,
                subsystem=SDS,
                blueprint=blueprint,
                new_class=unreal.StaticMeshComponent,
                name=f"StaticMesh{i + 1}")

            assert isinstance(static_mesh_comp, unreal.StaticMeshComponent)

            mesh = unreal.EditorAssetLibrary.find_asset_data(path).get_asset()
            static_mesh_comp.set_static_mesh(mesh)
            static_mesh_comp.set_editor_property(
                name="relative_location",
                value=unreal.Vector(transform["loc"][0],
                                    transform["loc"][1],
                                    transform["loc"][2])
            )
            static_mesh_comp.set_editor_property(
                name="relative_rotation",
                value=unreal.Rotator(pitch=transform["rot"][0],
                                     roll=transform["rot"][1],
                                     yaw=transform["rot"][2])
            )
            static_mesh_comp.set_editor_property(
                name="relative_scale3d",
                value=unreal.Vector(transform["scale"][0],
                                    transform["scale"][1],
                                    transform["scale"][2])
            )
        else:
            print(f"Error: Asset {path} doesn't exist")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
    if DO_DELETE_IF_EXISTS:
        unreal.EditorAssetLibrary.delete_asset(ASSET_PATH)
    else:
        raise FileExistsError(f"This file {ASSET_PATH} already exists")

blueprint = create_actor_blueprint(ASSET_NAME, ASSET_DIR)
add_components_to_blueprint(blueprint)
