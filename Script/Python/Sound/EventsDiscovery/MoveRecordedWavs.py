import os
import re
import shutil

src_dir = "C:/Program Files (x86)/Audiokinetic/Wwise 2016.1.6.5926/SDK/samples/IntegrationDemo/" \
          "WwiseProject/GeneratedSoundBanks/Windows/"
dst_dir = "C:/Users/JediKnight/Documents/FMOD Studio/ECR/Assets/EC/WWise_Extracted/"

i = 0
files = os.listdir(src_dir)
for file in files:
    if file.lower().endswith(".wav"):
        i += 1

        filepath = os.path.join(src_dir, file)
        snb_name, wav_name = file.split("_bnk_")
        s = re.search(r"(?P<event>.+)_(?P<repeat>\d)\.wav", wav_name)
        repeat_name = s.group("repeat")
        event_name = s.group("event")
        # print(snb_name, event_name, repeat_name)
        if os.stat(filepath).st_size < 1024 * 50:
            continue
        new_path = os.path.join(dst_dir, snb_name, event_name, event_name + "_" + repeat_name + ".wav")
        new_dir = os.path.dirname(new_path)
        os.makedirs(new_dir, exist_ok=True)
        shutil.copy(filepath, new_path)
        print(new_path)
print(i)
