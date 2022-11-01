import os
import re
import json


def get_material_references_for_uasset(uasset_filepath):
    """For given uasset file, regex search references to /Game/Materials/...
    and return assumed slot names to this references"""

    with open(uasset_filepath, "rb") as f:
        content = f.read()
    material_refs = [r.group("ref").decode("utf8") for r in
                     re.finditer(rb"(?P<ref>/Game/Materials/[a-zA-Z0-9\/_]+)\x00", content)]
    slot_names_to_material_refs = {os.path.basename(material_ref): material_ref for material_ref in material_refs}
    return slot_names_to_material_refs


def collect_material_references_for_directory(root_dir):
    """Apply get_material_references_for_uasset to the files in root_dir in recursive walk"""

    total_material_data = {}
    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if filename.endswith(".uasset"):
                filepath = os.path.join(root, filename)
                print(filepath)
                material_data = get_material_references_for_uasset(filepath)
                in_game_filepath = re.sub(r".+/Content/", "/Game/", filepath).replace("\\", "/").replace(
                    ".uasset", "")
                total_material_data[in_game_filepath] = material_data
    return total_material_data


if __name__ == '__main__':
    """Collect material references from meshes in this directory and save them to json file"""

    root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/EternalCrusade_dup/Content/PROPS/"
    data = collect_material_references_for_directory(root_dir)
    with open("props_material_references.json", "w") as f:
        json.dump(data, f, indent=4)
