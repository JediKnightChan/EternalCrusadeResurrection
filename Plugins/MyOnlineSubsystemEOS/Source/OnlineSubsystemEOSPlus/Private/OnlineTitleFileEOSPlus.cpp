// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineTitleFileEOSPlus.h"
#include "OnlineSubsystemEOSPlus.h"
#include "OnlineError.h"

FOnlineTitleFileEOSPlus::FOnlineTitleFileEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	BaseTitleFileInterface = EOSPlus->BaseOSS->GetTitleFileInterface();
}

FOnlineTitleFileEOSPlus::~FOnlineTitleFileEOSPlus()
{
	if (BaseTitleFileInterface.IsValid())
	{
		BaseTitleFileInterface->ClearOnEnumerateFilesCompleteDelegates(this);
		BaseTitleFileInterface->ClearOnReadFileProgressDelegates(this);
		BaseTitleFileInterface->ClearOnReadFileCompleteDelegates(this);
		BaseTitleFileInterface->ClearOnTitleFileAnalyticsEventDelegates(this);
	}
}

FUniqueNetIdEOSPlusPtr FOnlineTitleFileEOSPlus::GetNetIdPlus(const FString& SourceId) const
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineTitleFileEOSPlus::Initialize()
{
	if (BaseTitleFileInterface.IsValid())
	{
		BaseTitleFileInterface->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateThreadSafeSP(this, &FOnlineTitleFileEOSPlus::OnEnumerateFilesComplete));
		BaseTitleFileInterface->AddOnReadFileProgressDelegate_Handle(FOnReadFileProgressDelegate::CreateThreadSafeSP(this, &FOnlineTitleFileEOSPlus::OnReadFileProgress));
		BaseTitleFileInterface->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateThreadSafeSP(this, &FOnlineTitleFileEOSPlus::OnReadFileComplete));
		BaseTitleFileInterface->AddOnTitleFileAnalyticsEventDelegate_Handle(FOnTitleFileAnalyticsEventDelegate::CreateThreadSafeSP(this, &FOnlineTitleFileEOSPlus::OnTitleFileAnalyticsEvent));
	}
	else
	{
		UE_LOG_ONLINE(VeryVerbose, TEXT("[FOnlineTitleFileEOSPlus::Initialize] BaseTitleFileInterface delegates not bound. Base interface not valid"));
	}
}

//~ Begin IOnlineTitleFile Interface

bool FOnlineTitleFileEOSPlus::GetFileContents(const FString& FileName, TArray<uint8>& FileContents)
{
	bool bResult = false;

	if (BaseTitleFileInterface.IsValid())
	{
		bResult = BaseTitleFileInterface->GetFileContents(FileName, FileContents);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::GetFileContents] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineTitleFileEOSPlus::ClearFiles()
{
	bool bResult = false;

	if (BaseTitleFileInterface.IsValid())
	{
		bResult = BaseTitleFileInterface->ClearFiles();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::ClearFiles] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineTitleFileEOSPlus::ClearFile(const FString& FileName)
{
	bool bResult = false;

	if (BaseTitleFileInterface.IsValid())
	{
		bResult = BaseTitleFileInterface->ClearFile(FileName);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::ClearFile] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineTitleFileEOSPlus::DeleteCachedFiles(bool bSkipEnumerated)
{
	if (BaseTitleFileInterface.IsValid())
	{
		BaseTitleFileInterface->DeleteCachedFiles(bSkipEnumerated);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::DeleteCachedFiles] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineTitleFileEOSPlus::EnumerateFiles(const FPagedQuery& Page)
{
	bool bResult = false;

	if (BaseTitleFileInterface.IsValid())
	{
		bResult = BaseTitleFileInterface->EnumerateFiles(Page);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::EnumerateFiles] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineTitleFileEOSPlus::GetFileList(TArray<FCloudFileHeader>& Files)
{
	if (BaseTitleFileInterface.IsValid())
	{
		BaseTitleFileInterface->GetFileList(Files);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::GetFileList] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineTitleFileEOSPlus::ReadFile(const FString& FileName)
{
	bool bResult = false;

	if (BaseTitleFileInterface.IsValid())
	{
		bResult = BaseTitleFileInterface->ReadFile(FileName);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineTitleFileEOSPlus::ReadFile] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineTitleFileEOSPlus::OnEnumerateFilesComplete(bool bWasSuccessful, const FString& ErrorStr)
{
	TriggerOnEnumerateFilesCompleteDelegates(bWasSuccessful, ErrorStr);
}

void FOnlineTitleFileEOSPlus::OnReadFileProgress(const FString& FileName, uint64 NumBytes)
{
	TriggerOnReadFileProgressDelegates(FileName, NumBytes);
}

void FOnlineTitleFileEOSPlus::OnReadFileComplete(bool bWasSuccessful, const FString& FileName)
{
	TriggerOnReadFileCompleteDelegates(bWasSuccessful, FileName);
}

void FOnlineTitleFileEOSPlus::OnTitleFileAnalyticsEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	TriggerOnTitleFileAnalyticsEventDelegates(EventName, Attributes);
}

//~ End IOnlineTitleFile Interface