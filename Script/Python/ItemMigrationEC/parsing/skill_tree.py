import os
import sys
import json
import pandas as pd


def export_worker_dependencies(obj, dic, dic2):
    if isinstance(obj, dict):
        obj.pop("$type", None)
        l = obj.values()
    else:
        l = obj

    for value in l:
        if value == "ECSkillLink":
            dic[obj["Value"][1]["Value"]] = obj["Value"][0]["Value"]
        elif value == "ECSkillAssetEntry":
            dic2[obj["Value"][2]["Value"]] = f'{obj["Value"][1]["Value"][0].get("X")},{obj["Value"][1]["Value"][0].get("Y")}'
        elif isinstance(value, dict) or isinstance(value, list):
            export_worker_dependencies(value, dic, dic2)


# If obj contains lists or dicts, it will call itself on them
# Will purge type lines so that it is less messy
def export_worker(obj, dic, imports):
    if isinstance(obj, dict):
        obj.pop("$type", None)
        l = obj.values()

        if "bIsAsset" in obj.keys():
            if obj["bIsAsset"] == True:
                dic["name"] = obj["ObjectName"]

        if "Name" in obj.keys():
            if obj["Name"] == "Name":
                dic["displayName"] = obj["CultureInvariantString"]
            elif obj["Name"] == "ItemReward":
                dic["itemReward"] += imports[abs(obj["Value"][0]["Value"]) - 1]["ObjectName"]

    else:
        l = obj

    for value in l:
        if value == "Amount":
            dic["progressionPointsCost"] = obj["Value"]

        elif value == "Rank":
            dic["rank"] = obj["Value"]

        elif isinstance(value, dict) or isinstance(value, list):
            export_worker(value, dic, imports)


def get_dependency_map(input_json):
    with open(input_json, "r") as f:
        data = json.load(f)

    link_dep_map = {}
    link_pos_map = {}
    node_map = {}
    dep_map = {}
    pos_map = {}

    imports = data["Imports"]
    i = -1
    for e in imports:
        node_map[i] = e["ObjectName"]
        i -= 1

    exports = data["Exports"]
    export_worker_dependencies(exports, link_dep_map, link_pos_map)

    for k, v in link_dep_map.items():
        dep_map[node_map[k]] = node_map[v]

    for k, v in link_pos_map.items():
        pos_map[node_map[k]] = v

    return dep_map, pos_map


def export_items(input_dir, output_csv, dep_map, pos_map, ignored_files):
    items = []

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file in ignored_files:
                continue

            item = {}

            with open((os.path.join(root, file)), 'r') as f:
                try:
                    item["name"] = ""
                    item["displayName"] = ""
                    item["progressionPointsCost"] = 0
                    item["rank"] = 0
                    item["dependency"] = ""
                    item["itemReward"] = ""

                    data = json.load(f)
                    exports = data["Exports"]
                    export_worker(exports, item, data["Imports"])

                    if item["name"] in dep_map.keys():
                        item["dependency"] = dep_map[item["name"]]

                    if item["name"] in pos_map.keys():
                        item["ui_position"] = pos_map[item["name"]]

                except Exception as e:
                    print(os.path.join(root, file))
                    print(e)

            items.append(item)

    df = pd.DataFrame(data=items)
    df = df.rename(
        {"name": "Progression", "displayName": "Name", "progressionPointsCost": "ProgressionPointsCost", "rank": "Rank",
         "dependency": "Dependency", "itemReward": "ItemReward"}, axis=1)
    print(df)
    df.to_csv(output_csv, index=False)


if __name__ == '__main__':
    input_dir = "../data/ec_raw/progression_trees/csm/"
    tree_json = "ChaosAdvancements.json"
    ignored_files = ["SpaceMarineAdvancements_NEW.json", "SpaceWolfAdvancements4.json", "ChaosAdvancements.json"]
    output_csv = "../data/ec/advancements/csm.csv"

    input_json = os.path.join(input_dir, tree_json)
    dep_map, pos_map = get_dependency_map(input_json)
    print(pos_map)
    export_items(input_dir, output_csv, dep_map, pos_map, ignored_files)
