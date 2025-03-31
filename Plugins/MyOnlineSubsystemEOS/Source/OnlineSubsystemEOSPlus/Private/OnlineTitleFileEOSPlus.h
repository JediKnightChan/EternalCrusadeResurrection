// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineTitleFileInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for encapsulating the platform title file interface
 */
class FOnlineTitleFileEOSPlus :
	public IOnlineTitleFile,
	public TSharedFromThis<FOnlineTitleFileEOSPlus, ESPMode::ThreadSafe>
{
public:
	FOnlineTitleFileEOSPlus() = delete;
	virtual ~FOnlineTitleFileEOSPlus();

	void Initialize();

	//~ Begin IOnlineTitleFile Interface
	virtual bool GetFileContents(const FString& FileName, TArray<uint8>& FileContents) override;
	virtual bool ClearFiles() override;
	virtual bool ClearFile(const FString& FileName) override;
	virtual void DeleteCachedFiles(bool bSkipEnumerated) override;
	virtual bool EnumerateFiles(const FPagedQuery& Page = FPagedQuery()) override;
	virtual void GetFileList(TArray<FCloudFileHeader>& Files) override;
	virtual bool ReadFile(const FString& FileName) override;
	//~ End IOnlineTitleFile Interface

PACKAGE_SCOPE:
	FOnlineTitleFileEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	void OnEnumerateFilesComplete(bool bWasSuccessful, const FString& ErrorStr);
	void OnReadFileProgress(const FString& FileName, uint64 NumBytes);
	void OnReadFileComplete(bool bWasSuccessful, const FString& FileName);
	void OnTitleFileAnalyticsEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId) const;

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;
	
	// We don't support EOS mirroring yet
	IOnlineTitleFilePtr BaseTitleFileInterface;
};

typedef TSharedPtr<FOnlineTitleFileEOSPlus, ESPMode::ThreadSafe> FOnlineTitleFileEOSPlusPtr;
