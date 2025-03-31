// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineUserCloudInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for encapsulating the platform user cloud interface
 */
class FOnlineUserCloudEOSPlus :
	public IOnlineUserCloud,
	public TSharedFromThis<FOnlineUserCloudEOSPlus, ESPMode::ThreadSafe>
{
public:
	FOnlineUserCloudEOSPlus() = delete;
	virtual ~FOnlineUserCloudEOSPlus();

	void Initialize();

	//~ Begin IOnlineUserCloud Interface
	virtual bool GetFileContents(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents) override;
	virtual bool ClearFiles(const FUniqueNetId& UserId) override;
	virtual bool ClearFile(const FUniqueNetId& UserId, const FString& FileName) override;
	virtual void EnumerateUserFiles(const FUniqueNetId& UserId) override;
	virtual void GetUserFileList(const FUniqueNetId& UserId, TArray<FCloudFileHeader>& UserFiles) override;
	virtual bool ReadUserFile(const FUniqueNetId& UserId, const FString& FileName) override;
	virtual bool WriteUserFile(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents, bool bCompressBeforeUpload = false) override;
	virtual void CancelWriteUserFile(const FUniqueNetId& UserId, const FString& FileName) override;
	virtual bool DeleteUserFile(const FUniqueNetId& UserId, const FString& FileName, bool bShouldCloudDelete, bool bShouldLocallyDelete) override;
	virtual bool RequestUsageInfo(const FUniqueNetId& UserId) override;
	virtual void DumpCloudState(const FUniqueNetId& UserId) override;
	virtual void DumpCloudFileState(const FUniqueNetId& UserId, const FString& FileName) override;
	//~ End IOnlineUserCloud Interface

PACKAGE_SCOPE:
	FOnlineUserCloudEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	void OnEnumerateUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& UserId);
	void OnWriteUserFileProgress(int32 BytesWritten, const FUniqueNetId& UserId, const FString& FileName);
	void OnWriteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);
	void OnWriteUserFileCanceled(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);
	void OnReadUserFileComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);
	void OnDeleteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);
	void OnRequestUsageInfoComplete(bool bWasSuccessful, const FUniqueNetId& UserId, int64 BytesUsed, const TOptional<int64>& TotalQuota);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId) const;

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;
	
	// We don't support EOS mirroring yet
	IOnlineUserCloudPtr BaseUserCloudInterface;
};

typedef TSharedPtr<FOnlineUserCloudEOSPlus, ESPMode::ThreadSafe> FOnlineUserCloudEOSPlusPtr;
