import json
import os

import pandas as pd

df = pd.read_csv("../data/ecr/gameplay_items/csm.csv", encoding="utf-16")
df = df.fillna("")

item_names = list(df["---"])


def get_new_id(row):
    item_def = row["Item Definition"]
    if not item_def:
        item_name = row["---"]
        item_supposed_parent = "_".join(item_name.split("_")[:2]) + "_Standard"
        if item_supposed_parent not in item_names:
            return None

        separate_id_needed = False
        category = "Grey"
        separate_id_substrs = {"_StandardV": "Green", "_T2": "Blue", "_Unique": "Purple"}
        for k, v in separate_id_substrs.items():
            if k in item_name:
                category = v
                break

        new_id = ""
        parent_id = df[df["---"] == item_supposed_parent].iloc[0]["Item Definition"]
        parent_id = parent_id.replace("/Script/Engine.BlueprintGeneratedClass", "").strip("'")
        parent_id_dir = os.path.dirname(parent_id)
        parent_id_filename = os.path.basename(parent_id).split(".")[0]

        if category == "Grey":
            new_id = parent_id_filename
        elif category == "Green":
            new_id = parent_id_filename + "_Alt"
        elif category == "Blue":
            new_id = parent_id_filename + "_MasterCrafted"
        else:
            new_id = parent_id_filename + "_Unique"

        return parent_id_dir, new_id, parent_id_filename


result = {}
for i, row in df.iterrows():
    new_id = get_new_id(row)
    if new_id:
        result[row["---"]] = {
            "dir": new_id[0],
            "parent_file": new_id[2],
            "file": new_id[1],
        }

with open("сsm_id_map.json", "w") as f:
    json.dump(result, f, indent=4)
