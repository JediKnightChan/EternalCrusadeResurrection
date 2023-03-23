"""
Dummy Unreal library simulator for running scripts outside Unreal with testing purposes
"""


def log_warning(text):
    print(text)


class Asset:
    @staticmethod
    def get_asset():
        return None

    @staticmethod
    def set_material(index, material):
        pass


class Array:
    def __init__(self, Type):
        pass

    def append(self, value):
        pass


class EditorFilterLibrary:
    @staticmethod
    def by_class(array, class_):
        return array


class EditorAssetLibrary:
    @staticmethod
    def does_asset_exist(path):
        return True

    @staticmethod
    def find_asset_data(path):
        return Asset()

    @staticmethod
    def delete_asset(path):
        return

    @staticmethod
    def save_asset(path, only_if_is_dirty=True):
        return

    @staticmethod
    def load_asset(path):
        return Asset()


class AssetTools:
    def create_asset(self, *args):
        pass


class AssetToolsHelpers:
    @staticmethod
    def get_asset_tools():
        return AssetTools()


class MaterialEditingLibrary:
    @staticmethod
    def set_material_instance_texture_parameter_value(material, key, value):
        return

    @staticmethod
    def set_material_instance_parent(material, parent):
        return


class AssetRegistryHelpers:
    @staticmethod
    def get_asset_registry():
        return


class MaterialInstanceConstant:
    pass


class MaterialInstanceConstantFactoryNew:
    pass


class StaticMesh:
    pass


class SkeletalMesh:
    pass


class SkeletalMaterial:
    def set_editor_property(self, name, value):
        pass


class MeshComponent:
    def __init__(self):
        self.material_slot_names = ["SlotName1", "SlotName2"]

    def get_material_slot_names(self):
        return self.material_slot_names

    def get_material_index(self, slot_name):
        return self.material_slot_names.index(slot_name)


class StaticMeshComponent(MeshComponent):

    def set_static_mesh(self, static_mesh_asset):
        pass


class SkeletalMeshComponent(MeshComponent):

    def set_skeletal_mesh(self, static_mesh_asset):
        pass


class StaticMeshEditorSubsystem:
    pass


def get_editor_subsystem(subsystem):
    pass
