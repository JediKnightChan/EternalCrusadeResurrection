from S3Utils import upload_file_to_s3, check_file_exists
import os

latest_s3_key = None
met_latest_s3_key = False

root_sound_directory = "./Files/Windows/English(US)/"
for root, subdirs, files in os.walk(root_sound_directory):
    for file in sorted(files):
        if file.endswith(".ogg"):
            filepath = os.path.join(root, file).replace("\\", "/")
            s3_key = filepath.replace("\\", "/").replace("./Files/Windows/", "sound_sorting/root/")

            if latest_s3_key:
                if s3_key == latest_s3_key:
                    met_latest_s3_key = True
                if not met_latest_s3_key:
                    print("Skipping", s3_key)
                    continue

            if check_file_exists(s3_key, 'ecr-sounds'):
                print(s3_key, "Exists")
                continue

            print(s3_key, "uploading")

            with open(filepath, "rb") as f:
                content = f.read()
                upload_file_to_s3(content, s3_key, 'ecr-sounds', 'audio/ogg')
