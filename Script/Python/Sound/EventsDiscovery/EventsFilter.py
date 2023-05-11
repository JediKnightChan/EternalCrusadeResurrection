import json

with open("soundbank_events.json", "r") as f:
    data = json.load(f)

new_data = {}

for k, v in data.items():
    if not k.startswith("SNB_"):
        continue
    new_data[k] = v


with open("soundbank_events_temp.json", "w") as f:
    json.dump(new_data, f, indent=4)

print(len(new_data.keys()))
