import json


def go_through_json(x):
    if isinstance(x, list):
        return [go_through_json(v) for v in x if True or
                not isinstance(v, dict) or v.get("SerializingPropertyType", "") != "LazyObjectProperty"]
    elif isinstance(x, dict):
        if x.get("SerializingPropertyType", "") == "LazyObjectProperty":
            x["Value"]["$value"] = "Y7tGN2c48kCYPX0bLAfQMA=="
        return {k: go_through_json(v) for k, v in x.items()}
    else:
        return x


def string_contains_any(string, substring_list):
    for s in substring_list:
        if s.lower() in string.lower():
            return True
    return False


with open("Files/t_x3_y1.json", "r") as f:
    data = json.load(f)
print(len(data["Exports"]))

new_data = go_through_json(data)
with open("Files/t_x3_y1_filtered.json", "w") as f:
    json.dump(new_data, f, indent=4, ensure_ascii=False)
