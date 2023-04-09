import os.path
import re
import subprocess
import time

DO_DELETE_BNK = False
root_sound_directory = "C:/Users/JediKnight/Downloads/smvoiceunpacker/pck_outputs/sfx/player1_vanguard/"
tools_directory = "C:/Users/JediKnight/Downloads/Wwise-Audio-Unpacker-master/Tools/"
bnk_extractor_exe = os.path.join(tools_directory, "bnkextr.exe")

for root, subdirs, files in os.walk(root_sound_directory):
    for file in files:
        if file.lower().endswith(".bnk"):
            bnk_filepath = os.path.join(root, file)
            try:
                subprocess.run(f"del *.wav")
            except FileNotFoundError:
                pass
            subprocess.run(f"{bnk_extractor_exe} {bnk_filepath}", capture_output=False)
            time.sleep(1)

            new_bnk_dir = os.path.join(root, file.replace(".bnk", ""))
            # print(file, new_bnk_dir)

            for this_dir_file in os.listdir("."):
                if this_dir_file.endswith(".wav"):
                    new_wem_file = re.search(r"(?P<number>\d+).wav", this_dir_file).group("number") + ".wem"

                    os.makedirs(new_bnk_dir, exist_ok=True)
                    new_wem_filepath = os.path.join(new_bnk_dir, new_wem_file)
                    if os.path.exists(new_wem_filepath):
                        os.remove(new_wem_filepath)

                    os.rename(os.path.join(".", this_dir_file), new_wem_filepath)
            if DO_DELETE_BNK:
                os.remove(bnk_filepath)
