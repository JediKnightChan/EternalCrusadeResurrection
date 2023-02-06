import os
from PropsTxtToJson import convert_props_txt_file_to_json

if __name__ == '__main__':
    """Bulk conversion of produced by umodel .props.txt material files to json"""

    root_dir = "D:/MyProjects/eternal_crusade/umodel_needed/exp2/Characters/SpaceMarine/"

    DO_DELETE_PROPS_TXT = True
    DO_DELETE_MAT = True

    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if filename.endswith(".mat"):
                os.remove(os.path.join(root, filename))
            if filename.endswith(".props.txt"):
                props_filepath = os.path.join(root, filename)
                print(props_filepath)
                json_content = convert_props_txt_file_to_json(props_filepath)
                json_filepath = os.path.join(root, filename.replace(".props.txt", ".json"))
                with open(json_filepath, "w") as f:
                    f.write(json_content)
                if DO_DELETE_PROPS_TXT:
                    os.remove(props_filepath)
