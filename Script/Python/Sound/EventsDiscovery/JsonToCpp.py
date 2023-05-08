import json

with open("soundbank_events_temp.json", "r") as f:
    data = json.load(f)

for k, v in data.items():
    for el in v:
        if str(el['id']) == el['name']:
            continue
        print(f"{{ \"{k}\", {el['id']}, \"{el['name']}\" }},")
