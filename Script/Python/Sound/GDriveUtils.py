import os
import io
import logging

from googleapiclient import discovery
from httplib2 import Http
from oauth2client import file, client, tools
from googleapiclient.http import MediaFileUpload, MediaIoBaseDownload

from abc import ABC, abstractmethod


class DriveWorker(ABC):

    def recursively_get_files_in_folder_by_url(self, folder_link, file_attributes, file_filter=None):
        pass

    def _get_filter_function_from_items_filter(self, items_filter):
        if items_filter is None:
            return self.create_filter_function_from_dict({})
        elif isinstance(items_filter, dict):
            return self.create_filter_function_from_dict(items_filter)
        elif callable(items_filter):
            return items_filter
        else:
            raise ValueError("Items filter must be either None or dict or function")

    @staticmethod
    def create_filter_function_from_dict(filter_dict):
        pass


logger = logging.getLogger('multiprocess-queue-logger')


class GoogleDriveWorker(DriveWorker):
    def __init__(self, credentials_file):
        SCOPES = 'https://www.googleapis.com/auth/drive'
        store = file.Storage('storage.json')
        creds = store.get()
        if not creds or creds.invalid:
            flow = client.flow_from_clientsecrets(credentials_file, SCOPES)
            creds = tools.run_flow(flow, store)
        self.DRIVE = discovery.build('drive', 'v3', http=creds.authorize(Http()))

    def get_file_content(self, gdrive_id):
        """Get content of file from Google Drive by its ID"""
        request = self.DRIVE.files().get_media(fileId=gdrive_id)
        fh = io.BytesIO()
        downloader = MediaIoBaseDownload(fh, request)
        done = False
        while done is False:
            status, done = downloader.next_chunk()
            logger.debug("Download %d%%." % int(status.progress() * 100))
        fh.seek(0)
        return fh.read()

    def download_file(self, local_dir, gdrive_path, item):
        content = self.get_file_content(item["id"])
        local_filepath = os.path.join(local_dir, gdrive_path[1:])
        logger.debug(local_filepath)
        os.makedirs(os.path.dirname(local_filepath), exist_ok=True)
        with open(local_filepath, 'wb') as f:
            f.write(content)

    def _get_all_items_in_folder(self, google_drive_id):
        query = f"'{google_drive_id}' in parents"
        page_token = None

        all_items = []
        while True:
            results = self.DRIVE.files().list(fields="nextPageToken, files(id, name, kind, mimeType)", q=query,
                                              pageToken=page_token).execute()
            items = results.get('files', [])
            all_items += items

            page_token = results.get('nextPageToken', None)
            if page_token is None:
                break

        return all_items

    def retain_folder_structure(self, gdrive_id, gdrive_path="/", local_dir=None, items_filter=None):
        """Either lists all files in gdrive_id directory or downloads them to local_dir based on download"""

        items_filter = self._get_filter_function_from_items_filter(items_filter)

        items_on_this_level = self._get_all_items_in_folder(gdrive_id)
        all_good_files = []
        for item in items_on_this_level:
            print(item)
            gdrive_filepath = os.path.join(gdrive_path, item["name"])
            item["filepath"] = gdrive_filepath
            if item['mimeType'] == 'application/vnd.google-apps.folder':
                gdrive_subdir_path = os.path.join(gdrive_path, item["name"])
                good_dir_items = self.retain_folder_structure(item["id"], gdrive_path=gdrive_subdir_path,
                                                              local_dir=local_dir, items_filter=items_filter)
                all_good_files += good_dir_items
            else:
                if local_dir and items_filter(item):
                    logger.debug("Downloading", item)
                    self.download_file(local_dir, gdrive_filepath, item)

        all_good_files += filter(items_filter, items_on_this_level)
        return all_good_files

    def recursively_get_files_in_folder_by_url(self, folder_url, item_attributes, items_filter=None):
        folder_id = folder_url.split("/")[-1]
        items = self.retain_folder_structure(folder_id, items_filter=items_filter)
        items = sorted(items, key=lambda x: x["filepath"])
        return [tuple(item[attribute] for attribute in item_attributes) for item in items]


if __name__ == "__main__":
    w = GoogleDriveWorker("./Files/creentials.json")

    items = w.retain_folder_structure("1jLHwfI0NEwoWAKsZqzgoV0ZxDu0B3G7Q", "Sounds/root")
    print(items)
