# Should be used with UE 4.12 version of EC with the plugin: https://github.com/20tab/UnrealEnginePython

import json
import unreal_engine as ue


def get_socket_data(socket):
    return {
        "name": socket.SocketName,
        "bone": socket.BoneName,
        "loc": [socket.RelativeLocation.x, socket.RelativeLocation.y, socket.RelativeLocation.z],
        "rot": [socket.RelativeRotation.roll, socket.RelativeRotation.pitch, socket.RelativeRotation.yaw],
        "scale": [socket.RelativeScale.x, socket.RelativeScale.y, socket.RelativeScale.z]
    }


global_socket_data = {}

"""
skeleton = ue.get_selected_assets()[0]
"""

for asset in ue.get_assets_by_class("Skeleton"):
    asset_path = asset.get_path_name()
    skeleton = asset

    current_sockets = skeleton.Sockets
    sockets_info = [get_socket_data(socket) for socket in current_sockets]
    if sockets_info:
        print(sockets_info)
        global_socket_data[asset_path] = sockets_info

    # TODO: add socket export for Skeletal meshes too

with open("sockets.json", "w") as f:
    json.dump(global_socket_data, f)
