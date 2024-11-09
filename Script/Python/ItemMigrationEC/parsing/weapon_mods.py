import os
import json
import traceback

import pandas as pd


def clean_struc(struc):
    if isinstance(struc, list):
        result = {}
        for el in struc:
            result = {**result, **clean_struc(el)}
        return result
    elif isinstance(struc, dict):
        return {struc["Name"]: clean_struc(struc["Value"])}
    else:
        return struc

# If obj contains lists or dicts, it will call itself on them
# Will purge type lines so that it is less messy
def export_worker(data_object, output_dict, mod_names):
    if isinstance(data_object, dict):
        data_object.pop("$type", None)
        l = data_object.values()
    else:
        l = data_object

    for value in l:
        if isinstance(value, str):
            for mod_name in mod_names:
                if value.startswith(mod_name):
                    if "Data" in data_object:
                        for struc in data_object["Data"]:
                            if mod_name in output_dict:
                                output_dict[mod_name][struc["Name"]] = clean_struc(struc.get("Value"))
                            else:
                                output_dict[mod_name] = {struc["Name"]: clean_struc(struc.get("Value"))}

        elif isinstance(value, dict) or isinstance(value, list):
            export_worker(value, output_dict, mod_names)


def export_items(input_dir, dirs_to_ignore, output_fp):
    items = []

    for root, dirs, files in os.walk(input_dir, topdown=False):
        do_ignore = False
        for ignore_dir in dirs_to_ignore:
            if ignore_dir in root:
                do_ignore = True

        if do_ignore:
            print("Ignoring", root)
            continue

        for file in files:
            item = {}
            print(file)
            with open((os.path.join(root, file)), 'r') as f:
                try:
                    item["name"] = file.removesuffix(".json")

                    data = json.load(f)

                    mod_names = ["GameplayEffectMetadata"]
                    name_map = data["NameMap"]
                    for s in name_map:
                        if s.endswith("Mod"):
                            mod_names.append(s)

                    exports = data["Exports"]
                    export_worker(exports, item, mod_names)

                    items.append(item)
                except Exception as e:
                    print(os.path.join(root, file), traceback.format_exc())

    df = pd.DataFrame(data=items)
    # print(df)
    df.to_csv(output_fp, index=False)


if __name__ == '__main__':
    input_dir = "../data/ec_raw/gameplay_items/csm/WeaponMods/"
    dirs_to_ignore = []
    output_fp = "../data/ec/gameplay_items/csm_weapon_mods.csv"

    export_items(input_dir, dirs_to_ignore, output_fp)
