import os.path
import subprocess

DO_REVORB = False
DO_DELETE_WEM = False
USE_AOTUV_CODEC = False

root_sound_directory = "C:/Users/JediKnight/Downloads/smvoiceunpacker/pck_outputs/sfx/player1_vanguard/0x21c8885c/"
# tools_directory = "C:/Users/JediKnight/Downloads/Wwise-Audio-Unpacker-master/Tools/"
tools_directory = "C:/Users/JediKnight/Downloads/smvoiceunpacker/ww2ogg024/"
wem_to_ogg_exe = os.path.join(tools_directory, "ww2ogg.exe")
codec_aotuv = os.path.join(tools_directory, "packed_codebooks_aoTuV_603.bin")
codec_standard = os.path.join(tools_directory, "packed_codebooks.bin")
revorb_exe = os.path.join(tools_directory, "revorb.exe")

for root, subdirs, files in os.walk(root_sound_directory):
    for file in files:
        if file.endswith(".wem") or file.endswith(".Wwise"):
            wem_filepath = os.path.join(root, file)
            if USE_AOTUV_CODEC:
                codec_param = f"--pcb {codec_aotuv}"
            else:
                codec_param = f"--pcb {codec_standard}"
            subprocess.run(f"{wem_to_ogg_exe} {wem_filepath} {codec_param}")
            if DO_REVORB:
                ogg_filepath = os.path.join(root, file.replace(".wem", ".ogg").replace(".Wwise", ".ogg"))
                subprocess.run(f"{revorb_exe} {ogg_filepath}")
            if DO_DELETE_WEM:
                os.remove(wem_filepath)
