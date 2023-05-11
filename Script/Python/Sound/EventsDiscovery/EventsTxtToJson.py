import json
import pandas as pd

df = pd.read_csv("all_events.txt", sep=" ", header=None)
keys = list(df[0])
values = list(df[1])
events_to_ids = dict(zip(keys, values))

with open("all_events.json", "w") as f:
    json.dump(events_to_ids, f, indent=4)
