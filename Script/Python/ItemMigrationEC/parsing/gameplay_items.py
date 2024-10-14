import os
import json

import pandas as pd


# If obj contains lists or dicts, it will call itself on them
# Will purge type lines so that it is less messy
def export_worker(data_object, output_dict):
    if isinstance(data_object, dict):
        data_object.pop("$type", None)
        l = data_object.values()
    else:
        l = data_object

    for value in l:
        if value == "DisplayName":
            output_dict["display_name"] = data_object["Value"]

        elif value == "Description":
            output_dict["description"] = data_object["Value"]

        elif value == "ItemImageUrl":
            output_dict["image"] = data_object["Value"]

        elif value == "PointValue":
            output_dict["lp_cost"] = data_object["Value"]

        elif value == "EItemTier":
            output_dict["tier"] = data_object["EnumValue"]

        elif value == "TagName" or value == "Tag":
            output_dict["tags"].append(data_object["Value"])

        elif value == "TagContainer" or value == "GrantedTags" or value == "BroadcastChannels":
            for tag in data_object["Value"]:
                if isinstance(tag, str):
                    output_dict["tags"].append(tag)

        elif isinstance(value, dict) or isinstance(value, list):
            export_worker(value, output_dict)


def export_items(input_dir, dirs_to_ignore, output_fp):
    items = []

    with open("../data/ec_raw/localization/ru.json", "r", encoding="utf-8") as f:
        ru_loc = json.load(f)

    with open("../data/ec_raw/localization/en.json", "r", encoding="utf-8") as f:
        en_loc = json.load(f)

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
                    item["display_name"] = ""
                    item["description"] = ""
                    item["image"] = ""
                    item["lp_cost"] = 0
                    item["tier"] = ""
                    item["tags"] = []

                    data = json.load(f)
                    exports = data["Exports"]
                    export_worker(exports, item)

                    item["display_name_ru"] = ru_loc.get(item["display_name"])
                    item["display_name_en"] = en_loc.get(item["display_name"])

                    item["description_ru"] = ru_loc.get(item["description"])
                    item["description_en"] = en_loc.get(item["description"])

                    item["tags"] = " ".join(item["tags"])
                    items.append(item)
                except Exception as e:
                    print(os.path.join(root, file))
                    print(e)

    df = pd.DataFrame(data=items)
    df.to_csv(output_fp, index=False)


if __name__ == '__main__':
    input_dir = "../data/ec_raw/gameplay_items/lsm/"
    dirs_to_ignore = ['Appearance']
    output_fp = "../data/ec/gameplay_items/lsm.csv"

    export_items(input_dir, dirs_to_ignore, output_fp)
