import boto3
import os
from botocore.exceptions import ClientError


def generate_s3_client():
    s3_session = boto3.session.Session()
    s3_storage_client = s3_session.client(
        service_name='s3',
        endpoint_url='https://storage.yandexcloud.net',
        aws_access_key_id=os.getenv("AWS_S3_ACCESS_KEY_ID"),
        aws_secret_access_key=os.getenv("AWS_S3_SECRET_ACCESS_KEY"),
        region_name="ru-central1"
    )
    return s3_storage_client


def upload_file_to_s3(content, s3_key, bucket='myownbucket', content_type=None):
    s3_storage_client = generate_s3_client()
    s3_storage_client.put_object(Bucket=bucket, Key=s3_key, Body=content, ContentType=content_type)


def get_file_from_s3(s3_key, bucket):
    """Get file content from S3 Object Storage by key"""
    s3_storage_client = generate_s3_client()
    obj_response = s3_storage_client.get_object(Bucket=bucket, Key=s3_key)
    content = obj_response['Body'].read()
    return content


def check_file_exists(s3_key, bucket):
    s3_storage_client = generate_s3_client()
    try:
        s3_storage_client.head_object(Bucket=bucket, Key=s3_key)
        return True
    except ClientError:
        return False
