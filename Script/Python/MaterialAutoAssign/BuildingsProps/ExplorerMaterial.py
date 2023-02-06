import os
import json
import re

"""Explore material data via parent materials"""


def save_material_data_to_json(root_dir, new_json_file):
    """Saves total data about materials in root_dir to json file"""

    all_materials_data = {}
    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if filename.endswith(".json"):
                filepath = os.path.join(root, filename).replace("\\", "/")
                print(filepath)
                with open(filepath, "r") as f:
                    material_data = json.load(f)
                    all_materials_data[filepath] = material_data
    with open(new_json_file, "w") as f:
        f.write(json.dumps(all_materials_data, indent=4))
    return all_materials_data


def load_materials_data_from_json(root_dir, new_json_file, references_file):
    """Loads total data about materials in root_dir to json file"""

    with open(new_json_file, "r") as f:
        total_material_data = json.load(f)
        total_material_data = {k.replace(root_dir, "/Game/").replace("//", "/").replace(".json", ""): v for k, v in
                               total_material_data.items()}

    with open(references_file, "r") as f:
        references_data = json.load(f)

    def get_parent_material(material_data_):
        if "Parent" in material_data_:
            parent_material_ = re.search(r"'(?P<path>[/a-zA-Z0-9_]+)\.[a-zA-Z0-9_]+'", material_data_["Parent"]).group(
                "path")
            parent_material_ = parent_material_.replace("/EternalCrusade/Content/", "/Game/")
            return parent_material_

    used_materials = []
    not_found_materials = []

    for _, slot_names_to_material_paths in references_data.items():
        for slot_name, material_path in slot_names_to_material_paths.items():
            if material_path in total_material_data:
                material_data = total_material_data[material_path]
            else:
                if material_path not in not_found_materials:
                    not_found_materials.append(material_path)
                continue

            if material_path not in used_materials:
                used_materials.append(material_path)

            # Adding parent materials to used materials
            while True:
                parent_material = get_parent_material(material_data)
                if parent_material is None:
                    break
                if parent_material in total_material_data:
                    material_data = total_material_data[parent_material]
                    if parent_material not in used_materials:
                        used_materials.append(parent_material)
                else:
                    # Didn't find parent material, break
                    if parent_material not in not_found_materials:
                        not_found_materials.append(parent_material)
                    break
    result = {k: v for k, v in total_material_data.items() if k in used_materials}

    return result, not_found_materials


def find_child_materials(all_materials_data, parent_material, replacer=None, recursive=False):
    """
    Finds child materials of a given parent material, using replacer for forming a search string
    from parent_material string. Can be recursive.
    """

    child_materials = []
    if replacer is None:
        search_string = parent_material
    else:
        search_string = replacer(parent_material)
    print("Search string is", search_string)

    for material_path, material_data in all_materials_data.items():
        if "Parent" in material_data:
            if search_string in material_data["Parent"]:
                child_materials.append(material_path)

    if recursive:
        if child_materials:
            grandchild_materials = []
            for child_material in child_materials:
                grandchild_search_res = find_child_materials(all_materials_data, child_material, replacer)
                grandchild_materials += grandchild_search_res
            child_materials += grandchild_materials

    return child_materials


def find_materials_with_other_parent(all_materials_data, parent_candidates, include_paret_materials=False):
    """Finds materials with other parent material aside from given"""

    results = []
    for material_path, material_data in all_materials_data.items():
        if "Parent" in material_data:
            has_known_parent = False
            for parent_candidate in parent_candidates:
                if parent_candidate in material_data["Parent"]:
                    has_known_parent = True
            if not has_known_parent:
                results.append((material_path, material_data["Parent"]))
        else:
            if include_paret_materials:
                results.append((material_path, "None"))
    return results


def find_material_parameters(all_materials_data, parent_material):
    results = {}
    for material_path, material_data in all_materials_data.items():
        if "Parent" in material_data:
            if parent_material in material_data["Parent"]:
                for key1, value1 in material_data.items():
                    if key1.startswith("ScalarParameterValues") or key1.startswith(
                            "TextureParameterValues") or key1.startswith("VectorParameterValues"):
                        for value2 in value1.values():
                            results[value2["ParameterName"]] = value2["ParameterValue"]
    return results


if __name__ == '__main__':
    root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/exp2"
    new_json_file = "space_marines_all_materials_data.json"
    references_file = "space_marines_material_references.json"
    # save_material_data_to_json(root_dir, new_json_file)

    all_materials_data, not_found_materials = load_materials_data_from_json(root_dir, new_json_file, references_file)

    # with open(new_json_file, "w") as f:
    #     f.write(json.dumps(all_materials_data, indent=4))

    print(len(all_materials_data), len(not_found_materials))
    # print("\n".join(not_found_materials))

    res = find_child_materials(all_materials_data, "M_Eldar01", recursive=False,
                               replacer=lambda string: os.path.basename(string).replace(".json", ""))
    # res = find_materials_with_other_parent(all_materials_data, ['M_BodyParts_01', 'M_SM_Unique01'])
    res = find_material_parameters(all_materials_data, 'M_Eldar01')
    print(len(res))
    for item in res:
        print(item)
