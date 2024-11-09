import os
import json

import pandas as pd


# If obj contains lists or dicts, it will call itself on them
# Will purge type lines so that it is less messy
def export_worker(data_object, output_dict, keys_to_save):
    if isinstance(data_object, dict):
        data_object.pop("$type", None)
        l = data_object.values()
    else:
        l = data_object

    for value in l:
        if value in keys_to_save:
            print(data_object)
            output_dict[value] = data_object["Value"]

        elif isinstance(value, dict) or isinstance(value, list):
            export_worker(value, output_dict, keys_to_save)


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
                    item["name"] = os.path.relpath((os.path.join(root, file)), input_dir)

                    data = json.load(f)

                    keys_to_save = ["HealthMax", "ShieldMax", "ShieldRegenRatePercent", "ToughnessRating", "MaxSprintSpeed", "MaxCrawlSpeed", "MaxPeekOutCoverSpeed", "MaxRunSpeed", "MaxRunBackwardSpeed", "MaxStrafeSpeed", "MaxWalkSpeedCrouched", "MaxAcceleration", "MaxStamina", "RegenRate", "RegenDelayNormal"]

                    exports = data["Exports"]
                    export_worker(exports, item, keys_to_save)

                    items.append(item)
                except Exception as e:
                    print(os.path.join(root, file))
                    print(e)

    df = pd.DataFrame(data=items)
    print(df)
    df.to_csv(output_fp, index=False)


if __name__ == '__main__':
    input_dir = "../data/ec_raw/data_assets_chars/lsm/"
    dirs_to_ignore = []
    output_fp = "../data/ec/gameplay_items/csm_initial_stats.csv"

    export_items(input_dir, dirs_to_ignore, output_fp)
