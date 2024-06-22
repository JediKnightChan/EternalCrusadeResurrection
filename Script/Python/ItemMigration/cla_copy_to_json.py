import os
import json


def parse_cla_copy(content):
    cea_assets = []
    rows = content.split("\n")
    for row in rows:
        row = row.strip()
        if row.startswith("ElementaryAssets"):
            value = row.split("=")[1]
            if value.startswith('/Script/ECRCommon.CustomizationElementaryAsset'):
                cea_assets.append(value)
    return cea_assets


src_dir = "./data/old/cla/raw/"
target_dir = "./data/old/cla/json/"
for filename in os.listdir(src_dir):
    if filename.endswith(".COPY"):
        with open(os.path.join(src_dir, filename), "r") as f:
            content = f.read()
            cea_assets = parse_cla_copy(content)
        with open(os.path.join(target_dir, filename.replace(".COPY", ".json")), "w") as f:
            json.dump({"cea": cea_assets}, f, indent=4)
