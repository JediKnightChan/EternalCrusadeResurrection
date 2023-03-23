import os.path
import subprocess

root_sound_directory = "./Files"
tools_directory = "C:/Users/JediKnight/Downloads/Wwise-Audio-Unpacker-master/Tools/"
wem_to_ogg_exe = os.path.join(tools_directory, "ww2ogg.exe")
codec = os.path.join(tools_directory, "packed_codebooks_aoTuV_603.bin")
revorb_exe = os.path.join(tools_directory, "revorb.exe")

for root, subdirs, files in os.walk(root_sound_directory):
    for file in files:
        if file.endswith(".wem"):
            wem_filepath = os.path.join(root, file)
            subprocess.run(f"{wem_to_ogg_exe} {wem_filepath} --pcb {codec}")
            ogg_filepath = os.path.join(root, file.replace(".wem", ".ogg"))
            subprocess.run(f"{revorb_exe} {ogg_filepath}")
            os.remove(wem_filepath)
