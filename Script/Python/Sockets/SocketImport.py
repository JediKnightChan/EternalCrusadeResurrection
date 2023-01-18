# Should be used with custom version of custom EC project with the plugin: https://github.com/20tab/UnrealEnginePython

import unreal_engine as ue
import json

with open("sockets.json", "r") as f:
    global_socket_data = json.load(f)

DEBUG_MODE = True

for asset in ue.get_assets_by_class("Skeleton"):
    asset_path = asset.get_path_name()
    if asset_path.startswith("/Game/Characters/SpaceMarine/Weapons/") and asset_path in global_socket_data:
        print(asset_path)

        skeleton = asset
        new_sockets = []

        ground_truth_sockets = global_socket_data[asset_path]
        # iterate each bone and add a random socket to it
        for socket_dict in ground_truth_sockets:
            # SkeletalMeshSocket outer must be set to the related Skeleton
            new_socket = ue.classes.SkeletalMeshSocket('', skeleton)
            new_socket.SocketName = socket_dict["name"]
            new_socket.BoneName = socket_dict["bone"]
            new_socket.RelativeLocation = ue.FVector(*socket_dict["loc"])
            new_socket.RelativeRotation = ue.FRotator(*socket_dict["rot"])
            new_socket.RelativeScale = ue.FVector(*socket_dict["scale"])

            new_sockets.append(new_socket)

        # set the new sockets list
        skeleton.Sockets = new_sockets
        skeleton.save_package()
        ue.open_editor_for_asset(skeleton)
        if DEBUG_MODE:
            break
