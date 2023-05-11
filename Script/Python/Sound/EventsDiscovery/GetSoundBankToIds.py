import json
import os

with open("all_events.json", "r") as f:
    events_to_ids = json.load(f)
ids_to_events = {v: k for k, v in events_to_ids.items()}


def read_bnk(bnk_path):
    rack = []
    with open(bnk_path, "rb") as f:
        # Section reading
        while bytes := f.read(4):
            if bytes == b'HIRC':
                # Remove length bytes
                f.read(4)
                # Read the number of objects in the HIRC section
                nb = int.from_bytes(f.read(4), 'little')
                # Read the objects' data and store it
                for obj in range(0, nb):
                    object_type = f.read(1)
                    object_length = int.from_bytes(f.read(4), 'little') - 4
                    object_id = int.from_bytes(f.read(4), 'little')
                    object_data = f.read(object_length)
                    rack.append([object_id, object_type, object_length])
            else:
                # Skip sections that don't interest us

                # Read section length (length without what is already read)
                length = f.read(4)
                # Skip section
                f.read(int.from_bytes(length, 'little'))

    event_ids = []
    for i in range(0, len(rack)):
        if rack[i][1] == b'\x04':
            event_id = rack[i][0]
            event_ids.append(event_id)
    return event_ids


if __name__ == '__main__':
    root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/EternalCrusade_dup/Content/WwiseAudio/Windows/"

    all_data = {}
    for file in os.listdir(root_dir):
        if file.endswith("bnk"):
            print(file)
            event_ids = read_bnk(root_dir + file)
            data = [{"id": event_id, "name": ids_to_events.get(event_id, str(event_id))} for event_id in event_ids]
            all_data[file] = data

    with open("soundbank_events.json", "w") as f:
        json.dump(all_data, f, indent=4)
