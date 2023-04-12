import json
import os.path

raw_filename = "Files/BP_CaputurePoint_Building_48x48.raw.json"
filename = raw_filename.replace(".raw.json", ".json")

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
        sm_full_name = f"{buildings_imports.get(sm_name, sm_name)}.{sm_name}"
        sm_data = {"path": f"StaticMesh {sm_full_name}", "transform": {}}

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
            print(sm_data)
        result.append(sm_data)

with open(filename, "w") as f:
    json.dump(result, f, ensure_ascii=False, indent=4)
