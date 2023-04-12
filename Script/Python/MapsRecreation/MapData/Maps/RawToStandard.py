import json
import os.path

raw_filename = "Raw/Medusa/medusa_background.raw.json"
new_filename = "Usual/Medusa/medusa_background.json"

with open(raw_filename, "rb") as f:
    data = json.load(f)

import_map = {}
for i, import_struc in enumerate(data["Imports"]):
    asset_path = import_struc["ObjectName"]
    import_map[i + 1] = asset_path

buildings_imports = {"Cube": "/Engine/BasicShapes/Cube.Cube"}
for i, import_struc in enumerate(data["Imports"]):
    asset_path = import_struc["ObjectName"]
    buildings_prefixes = ["/Game/Buildings/", "/Game/PROPS/", "/Game/Blueprints/", "/Game/Graybox/",
                          "/Game/VFX/Blueprints/", "/Game/Vehicles/"]
    for pref in buildings_prefixes:
        if asset_path.startswith(pref):
            buildings_imports[os.path.basename(asset_path)] = asset_path
            break


def merge_two_transforms(tr1, tr2):
    def sum_lists(l1, l2):
        return [x + y for x, y in zip(l1, l2)]

    def mult_lists(l1, l2):
        return [x * y for x, y in zip(l1, l2)]

    return {"loc": sum_lists(tr1["loc"], tr2["loc"]), "rot": sum_lists(tr1["rot"], tr2["rot"]),
            "scale": mult_lists(tr1["scale"], tr2["scale"])}


def get_default_transform():
    return {"loc": [0, 0, 0], "rot": [0, 0, 0], "scale": [1, 1, 1]}


def get_export_struc_transform(export_struc_):
    found_transform = False
    transform_ = {}
    for data_piece_ in export_struc_["Data"]:
        if data_piece_["Name"] == "RelativeLocation":
            found_transform = True
            value_struc = data_piece_["Value"][0]["Value"]
            transform_["loc"] = [value_struc["X"], value_struc["Y"], value_struc["Z"]]
        elif data_piece_["Name"] == "RelativeRotation":
            found_transform = True
            value_struc = data_piece_["Value"][0]["Value"]
            transform_["rot"] = [value_struc["Pitch"], value_struc["Roll"], value_struc["Yaw"]]
        elif data_piece_["Name"] == "RelativeScale3D":
            found_transform = True
            value_struc = data_piece_["Value"][0]["Value"]
            transform_["scale"] = [value_struc["X"], value_struc["Y"], value_struc["Z"]]
    for prop, fill_value in {"loc": 0, "rot": 0, "scale": 1}.items():
        if prop not in transform_:
            transform_[prop] = [fill_value] * 3
        transform_[prop] = [float(v) for v in transform_[prop]]

    if found_transform:
        return transform_
    else:
        return None


def get_attach_parent(struc):
    parent_struc = None
    for data_piece_ in struc["Data"]:
        if data_piece_["Name"] == "AttachParent":
            parent_ref = data_piece_["Value"]
            parent_struc = data["Exports"][parent_ref - 1]
    return parent_struc


def resolve_parent_transform(export_struc):
    # Try to find attach parent transform
    parent_struc = export_struc
    transform_ = get_default_transform()
    while True:
        parent_struc = get_attach_parent(parent_struc)
        if parent_struc is None:
            break
        parent_transform_ = get_export_struc_transform(parent_struc)
        if parent_transform_ is None:
            parent_transform_ = get_default_transform()
        transform_ = merge_two_transforms(parent_transform_, transform_)
    return transform_


result = []
blueprints_unique = []
for i, export_struc in enumerate(data["Exports"]):
    if len(export_struc["Data"]) == 0 or not isinstance(export_struc["Data"], list):
        continue

    # Standard static meshes placed in the level
    if export_struc["Data"][0]["Name"] == "StaticMesh":
        sm_ref = export_struc["Data"][0]["Value"]
        if sm_ref > 0:
            print("SM_REF > 0", sm_ref)
            continue
        sm_name = import_map[-sm_ref]
        sm_full_name = f"{buildings_imports.get(sm_name, sm_name)}.{sm_name}"

        # Try to find attach parent transform
        parent_transform = resolve_parent_transform(export_struc)

        # Get self rel transform
        self_rel_transform = get_export_struc_transform(export_struc)
        if self_rel_transform is None:
            self_rel_transform = get_default_transform()

        # Merge parent and self transform
        transform = merge_two_transforms(parent_transform, self_rel_transform)
        # if sm_name.startswith("SM_PROP_Magnetic_Elevator"):
        #     print(i+1, sm_name, self_rel_transform)
        sm_data = {"path": f"StaticMesh {sm_full_name}", "transform": transform}
        print("StaticMesh", sm_full_name)
        result.append(sm_data)

    # Blueprints placed in the level
    elif export_struc["ObjectName"].startswith("BP_"):
        sm_name = import_map[-export_struc["ClassIndex"]]

        transform = None
        # Try to get transform from root component reference
        for data_piece in export_struc["Data"]:
            if data_piece["Name"] == "RootComponent":
                root_ref = data_piece["Value"] - 1

                root_component_struc = data["Exports"][root_ref]

                # Try to find attach parent transform
                parent_transform = resolve_parent_transform(root_component_struc)
                # Get self rel transform
                self_rel_transform = get_export_struc_transform(root_component_struc)
                if self_rel_transform is None:
                    self_rel_transform = get_default_transform()
                transform = merge_two_transforms(parent_transform, self_rel_transform)

        if transform is None:
            transform = get_default_transform()

        if sm_name.lower().endswith("_c"):
            sm_name = sm_name[:-2]

        sm_full_name = f"{buildings_imports.get(sm_name, sm_name)}.{sm_name}"
        sm_data = {
            "path": f"Blueprint {sm_full_name}",
            "transform": transform
        }
        print("Blueprint", sm_full_name)
        if sm_full_name not in blueprints_unique:
            blueprints_unique.append(sm_full_name)
        result.append(sm_data)

with open(new_filename, "w") as f:
    json.dump(result, f, ensure_ascii=False, indent=4)

print("=" * 40 + " Unique Blueprints " + "=" * 40)
for el in blueprints_unique:
    print(el)
