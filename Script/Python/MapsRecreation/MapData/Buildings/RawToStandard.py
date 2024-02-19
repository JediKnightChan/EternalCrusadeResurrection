import json
import os.path
import re


raw_filename = "Files/BP_SIG_PromethiumPump_01_Int.raw.json"
filename = raw_filename.replace(".raw.json", ".json")
PATH_REPLACING_JSON = "../../../MapUtils/assets_to_replace.json"

with open(PATH_REPLACING_JSON, "rb") as f:
    path_replacing_map = json.load(f)

    def pp(string):
        t = re.search(r"\w+'(?P<path>[\/\w]+).\w+", string)
        return t.group("path") if t else ""

    path_replacing_map = {pp(k): pp(v) for k, v in path_replacing_map.items()}

with open(raw_filename, "rb") as f:
    data = json.load(f)

import_map = {}
for i, import_struc in enumerate(data["Imports"]):
    asset_path = import_struc["ObjectName"]
    import_map[i + 1] = asset_path

buildings_imports = {"Cube": "/Engine/BasicShapes/Cube.Cube"}
for i, import_struc in enumerate(data["Imports"]):
    asset_path = import_struc["ObjectName"]
    if asset_path.startswith("/Game/Buildings/") or asset_path.startswith("/Game/PROPS/"):
        buildings_imports[os.path.basename(asset_path)] = asset_path

result = []
for i, export_struc in enumerate(data["Exports"]):
    if len(export_struc["Data"]) == 0:
        continue
    if export_struc["Data"][0]["Name"] == "StaticMesh":
        sm_ref = export_struc["Data"][0]["Value"]
        sm_name = import_map[-sm_ref]
        sm_full_name = f"{buildings_imports.get(sm_name, sm_name)}"
        sm_data = {"transform": {}}

        sm_data["want_child_actor"] = sm_full_name in path_replacing_map
        sm_data["path"] = path_replacing_map.get(sm_full_name, sm_full_name)


        for data_piece in export_struc["Data"]:
            if data_piece["Name"] == "RelativeLocation":
                value_struc = data_piece["Value"][0]["Value"]
                sm_data["transform"]["loc"] = [value_struc["X"], value_struc["Y"], value_struc["Z"]]
            elif data_piece["Name"] == "RelativeRotation":
                value_struc = data_piece["Value"][0]["Value"]
                sm_data["transform"]["rot"] = [value_struc["Pitch"], value_struc["Roll"], value_struc["Yaw"]]
            elif data_piece["Name"] == "RelativeScale3D":
                value_struc = data_piece["Value"][0]["Value"]
                sm_data["transform"]["scale"] = [value_struc["X"], value_struc["Y"], value_struc["Z"]]
        for prop, fill_value in {"loc": 0, "rot": 0, "scale": 1}.items():
            if prop not in sm_data["transform"]:
                sm_data["transform"][prop] = [fill_value] * 3
            sm_data["transform"][prop] = [float(v) for v in sm_data["transform"][prop]]
        result.append(sm_data)

with open(filename, "w") as f:
    json.dump(result, f, ensure_ascii=False, indent=4)
