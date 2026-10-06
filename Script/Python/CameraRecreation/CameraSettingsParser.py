
import json
import os
import subprocess

import pandas as pd


ROOT_DIR = r"C:\Program Files (x86)\Steam\steamapps\common\Warhammer 40,000 - Eternal Crusade\extract\raw_uassets\EternalCrusade\Content\Blueprints\Camera"

# Change this to your UAssetGUI executable.
UASSET_GUI = r"C:\Users\JediKnight\Documents\UAssetGUI\UAssetGUI.exe"

OUTPUT_CSV = os.path.join(ROOT_DIR, "combined.csv")


def flatten_value(value):
    """Convert UAssetAPI JSON values into simple Python values."""

    if isinstance(value, (int, float, bool, str)):
        return value

    if isinstance(value, list):
        if len(value) == 1:
            return flatten_value(value[0])

        return [flatten_value(x) for x in value]

    if isinstance(value, dict):

        # UAssetAPI wrapper:
        # {
        #     "$type": "...",
        #     "Value": {...}
        # }
        if "Value" in value:
            return flatten_value(value["Value"])

        # FVector / FVector2D
        if all(k in value for k in ("X", "Y", "Z")):
            return (
                float(value["X"]),
                float(value["Y"]),
                float(value["Z"]),
            )

        if all(k in value for k in ("X", "Y")):
            return (
                float(value["X"]),
                float(value["Y"]),
            )

        # FRotator
        if all(k in value for k in ("Pitch", "Yaw", "Roll")):
            return (
                float(value["Pitch"]),
                float(value["Yaw"]),
                float(value["Roll"]),
            )

        # Generic dictionary
        return {
            k: flatten_value(v)
            for k, v in value.items()
        }

    return value


def export_uasset_to_json(uasset_path):
    """
    Export a .uasset to JSON using UAssetGUI.

    IMPORTANT:
    Adjust the command below to the command-line syntax of
    your UAssetGUI version.
    """

    json_path = uasset_path.replace(".uasset", ".json")

    command = [
        UASSET_GUI,
        "tojson",
        uasset_path,
        json_path,
        "VER_UE4_12"
    ]

    subprocess.run(
        command,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    return json_path


def parse_json(json_path, uasset_path):
    """Extract the Default_* object's properties into one dataframe row."""

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    row = {
        "File": os.path.relpath(uasset_path, ROOT_DIR),
    }

    for el in data.get("Exports", []):

        if "Default_" not in el.get("ObjectName", ""):
            continue

        for entry in el.get("Data", []):

            if "Value" not in entry:
                continue

            name = entry["Name"]
            value = flatten_value(entry["Value"])

            row[name] = value

    return row


rows = []

for current_dir, _, files in os.walk(ROOT_DIR):

    for filename in files:

        if not filename.lower().endswith(".uasset"):
            continue

        uasset_path = os.path.join(current_dir, filename)

        print(f"Processing: {uasset_path}")


        # 1. Generate JSON using UAssetGUI
        json_path = export_uasset_to_json(uasset_path)

        # 2. Parse generated JSON
        row = parse_json(json_path, uasset_path)

        # 3. Add to dataframe
        if len(row) > 1:
            rows.append(row)

        # 4. Remove temporary JSON
        os.remove(json_path)


df = pd.DataFrame(rows)

df.to_csv(
    OUTPUT_CSV,
    index=False,
    encoding="utf-8-sig",
)

print()
print(f"Processed {len(rows)} assets.")
print(f"Saved: {OUTPUT_CSV}")

