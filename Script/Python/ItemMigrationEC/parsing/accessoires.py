import os
import json

import pandas as pd


# If obj contains lists or dicts, it will call itself on them
# Will purge type lines so that it is less messy
def export_worker(data_object, output_dict, ge_mods):
    if isinstance(data_object, dict):
        data_object.pop("$type", None)
        l = data_object.values()
    else:
        l = data_object

    for value in l:
        if value == "SetByCallerPair":
            output_dict[data_object["Value"][0]["Value"]] = json.dumps([round(data_object["Value"][1]["Value"], 4), ge_mods.get(data_object["Value"][0]["Value"])])

        elif isinstance(value, dict) or isinstance(value, list):
            export_worker(value, output_dict, ge_mods)


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

            with open((os.path.join(root, file)), 'r') as f:
                try:
                    item["name"] = file.removesuffix(".json")

                    data = json.load(f)

                    ge_mods = {}
                    name_map = data["NameMap"]
                    for s in name_map:
                        if s.startswith("ItemMod_"):
                            s = s.replace("ItemMod_", "")
                            attr, attr_change_type = s.split("_")[0], s.split("_")[1]
                            ge_mods[attr] = attr_change_type

                    exports = data["Exports"]
                    export_worker(exports, item, ge_mods)

                    items.append(item)
                except Exception as e:
                    print(os.path.join(root, file))
                    print(e)

    df = pd.DataFrame(data=items)
    print(df)
    df.to_csv(output_fp, index=False)


if __name__ == '__main__':
    input_dir = "../data/ec_raw/gameplay_items/csm/Accessories/"
    dirs_to_ignore = []
    output_fp = "../data/ec/gameplay_items/csm_accessories.csv"

    export_items(input_dir, dirs_to_ignore, output_fp)
