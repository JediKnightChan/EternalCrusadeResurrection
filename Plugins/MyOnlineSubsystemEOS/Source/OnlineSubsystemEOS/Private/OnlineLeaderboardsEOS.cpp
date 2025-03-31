// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineLeaderboardsEOS.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemEOS.h"
#include "OnlineSubsystemEOSPrivate.h"
#include "OnlineSubsystemEOSTypes.h"
#include "UserManagerEOS.h"
#include "OnlineStatsEOS.h"

#if WITH_EOS_SDK
#include "eos_leaderboards.h"
#include "eos_userinfo.h"

struct FQueryLeaderboardForUserOptions :
	public EOS_Leaderboards_QueryLeaderboardUserScoresOptions
{
	TArray<char*> PointerArray;
	TArray<EOS_Leaderboards_UserScoresQueryStatInfo> StatInfoArray;
	TArray<EOS_ProductUserId> ProductUserIds;

	FQueryLeaderboardForUserOptions(const uint32 InNumColumns, TArray<EOS_ProductUserId>& InProductUserIds)
		: EOS_Leaderboards_QueryLeaderboardUserScoresOptions()
		, ProductUserIds(MoveTemp(InProductUserIds))
	{
		ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDUSERSCORES_API_LATEST;

		StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
		EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;

		PointerArray.AddZeroed(InNumColumns);
		for (int32 Index = 0; Index < PointerArray.Num(); Index++)
		{
			PointerArray[Index] = new char[EOS_OSS_STRING_BUFFER_LENGTH];
		}

		StatInfoArray.AddZeroed(InNumColumns);
		StatInfo = StatInfoArray.GetData();
		StatInfoCount = InNumColumns;

		UserIds = ProductUserIds.GetData();
		UserIdsCount = ProductUserIds.Num();
	}

	~FQueryLeaderboardForUserOptions()
	{
		for (int32 Index = 0; Index < PointerArray.Num(); Index++)
		{
			delete[] PointerArray[Index];
		}
	}
};

struct FQueryLeaderboardForUsersContext
{
	TArray<FUniqueNetIdRef> Players;
	FOnlineLeaderboardReadRef ReadObject;

	FQueryLeaderboardForUsersContext(const TArray<FUniqueNetIdRef>& InPlayers, FOnlineLeaderboardReadRef& InReadObject)
		: Players(InPlayers)
		, ReadObject(InReadObject)
	{
	}

	void AddEmptyRowForPlayer(FUniqueNetIdRef NetId, const FString& Nickname)
	{
		FOnlineStatsRow Row(Nickname, NetId);
		for (const FColumnMetaData& Column : ReadObject->ColumnMetadata)
		{
			Row.Columns.Add(Column.ColumnName, FVariantData());
		}
		Row.Rank = -1;
		ReadObject->Rows.Add(Row);
	}
};

typedef TEOSCallback<EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallback, EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo, FOnlineLeaderboardsEOS> FQueryLeaderboardForUsersCallback;

bool FOnlineLeaderboardsEOS::ReadLeaderboards(const TArray<FUniqueNetIdRef>& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	if (Players.Num() == 0)
	{
		EOSSubsystem->ExecuteNextTick([this]()
			{
				TriggerOnLeaderboardReadCompleteDelegates(false);
			});
		return true;
	}

	TArray<EOS_ProductUserId> ProductUserIds;
	ProductUserIds.Empty(Players.Num());
	// Validate the number of known users
	for (const FUniqueNetIdRef& NetId : Players)
	{
		const FUniqueNetIdEOS& EOSId = FUniqueNetIdEOS::Cast(*NetId);
		const EOS_ProductUserId UserId = EOSId.GetProductUserId();
		if (UserId != nullptr)
		{
			ProductUserIds.Add(UserId);
		}
	}

	FQueryLeaderboardForUserOptions Options(ReadObject->ColumnMetadata.Num(), ProductUserIds);
	Options.LocalUserId = EOSSubsystem->UserManager->GetLocalProductUserId(0);
	// Convert the column names to stats
	int32 Index = 0;
	for (const FColumnMetaData& Column : ReadObject->ColumnMetadata)
	{
		FCStringAnsi::Strncpy(Options.PointerArray[Index], TCHAR_TO_UTF8(*Column.ColumnName.ToString()), EOS_OSS_STRING_BUFFER_LENGTH);
		EOS_Leaderboards_UserScoresQueryStatInfo& StatInfo = Options.StatInfoArray[Index];
		StatInfo.ApiVersion = EOS_LEADERBOARDS_USERSCORESQUERYSTATINFO_API_LATEST;
		StatInfo.StatName = Options.PointerArray[Index];
		StatInfo.Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Latest;
		Index++;
	}

	TSharedPtr<FQueryLeaderboardForUsersContext> QueryContext = MakeShared<FQueryLeaderboardForUsersContext>(Players, ReadObject);

	FQueryLeaderboardForUsersCallback* CallbackObj = new FQueryLeaderboardForUsersCallback(FOnlineLeaderboardsEOSWeakPtr(AsShared()));
	CallbackObj->CallbackLambda = [this, QueryContext](const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* Data)
	{
		bool bWasSuccessful = Data->ResultCode == EOS_EResult::EOS_Success;
		if (!bWasSuccessful)
		{
	
			UE_LOG_ONLINE_LEADERBOARD(Error, TEXT("EOS_Leaderboards_QueryLeaderboardUserScores() failed with EOS result code (%s)"), ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			QueryContext->ReadObject->ReadState = EOnlineAsyncTaskState::Failed;
			TriggerOnLeaderboardReadCompleteDelegates(false);
			return;
		}

		char StatName[EOS_OSS_STRING_BUFFER_LENGTH];

		for (FUniqueNetIdRef NetId : QueryContext->Players)
		{
			FString Nickname = EOSSubsystem->UserManager->GetPlayerNickname(*NetId);
			const FUniqueNetIdEOS& EOSId = FUniqueNetIdEOS::Cast(*NetId);
			const EOS_ProductUserId UserId = EOSId.GetProductUserId();
			if (UserId == nullptr)
			{
				QueryContext->AddEmptyRowForPlayer(NetId, Nickname);
				continue;
			}

			FOnlineStatsRow Row(Nickname, NetId);

			EOS_Leaderboards_CopyLeaderboardUserScoreByUserIdOptions UserCopyOptions = { };
			UserCopyOptions.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDUSERSCOREBYUSERID_API_LATEST;
			UserCopyOptions.UserId = UserId;
			UserCopyOptions.StatName = StatName;

			// Read each stat from the leaderboard
			for (const FColumnMetaData& Column : QueryContext->ReadObject->ColumnMetadata)
			{
				// Update which stat we are requesting
				FCStringAnsi::Strncpy(StatName, TCHAR_TO_UTF8(*Column.ColumnName.ToString()), EOS_OSS_STRING_BUFFER_LENGTH);

				EOS_Leaderboards_LeaderboardUserScore* LeaderboardUserScore = nullptr;
				EOS_EResult UserCopyResult = EOS_Leaderboards_CopyLeaderboardUserScoreByUserId(EOSSubsystem->LeaderboardsHandle, &UserCopyOptions, &LeaderboardUserScore);
				if (UserCopyResult != EOS_EResult::EOS_Success)
				{
					Row.Columns.Add(Column.ColumnName, FVariantData());
					continue;
				}

				Row.Columns.Add(Column.ColumnName, FVariantData(LeaderboardUserScore->Score));

				EOS_Leaderboards_LeaderboardUserScore_Release(LeaderboardUserScore);
			}
			QueryContext->ReadObject->Rows.Add(Row);
		}

		// Manually build the ranks by sorting and then assigning rank values
		FName SortedColumn = QueryContext->ReadObject->SortedColumn;
		QueryContext->ReadObject->Rows.Sort([SortedColumn](const FOnlineStatsRow& RowA, const FOnlineStatsRow& RowB)
		{
			const FVariantData& ValueA = RowA.Columns[SortedColumn];
			const FVariantData& ValueB = RowB.Columns[SortedColumn];
			if (ValueA.GetType() == ValueB.GetType())
			{
				int32 ScoreA = 0;
				int32 ScoreB = 0;
				ValueA.GetValue(ScoreA);
				ValueB.GetValue(ScoreB);
				return ScoreA >= ScoreB;
			}
			return true;
		});
		int32 Rank = 1;
		for (FOnlineStatsRow& Row : QueryContext->ReadObject->Rows)
		{
			Row.Rank = Rank++;
		}

		QueryContext->ReadObject->ReadState = EOnlineAsyncTaskState::Done;

		TriggerOnLeaderboardReadCompleteDelegates(true);
	};

	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	EOS_Leaderboards_QueryLeaderboardUserScores(EOSSubsystem->LeaderboardsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}

bool FOnlineLeaderboardsEOS::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	TArray<TSharedRef<FOnlineFriend>> Friends;
	EOSSubsystem->UserManager->GetFriendsList(LocalUserNum, FString(), Friends);
	if (Friends.Num() == 0)
	{
		EOSSubsystem->ExecuteNextTick([this]()
			{
				TriggerOnLeaderboardReadCompleteDelegates(false);
			});
		return true;
	}

	TArray<FUniqueNetIdRef> Players;
	Players.Reset(Friends.Num());

	for (TSharedRef<const FOnlineFriend> Friend : Friends)
	{
		Players.Add(Friend->GetUserId());
	}

	return ReadLeaderboards(Players, ReadObject);
}

typedef TEOSCallback<EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallback, EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo, FOnlineLeaderboardsEOS> FQueryLeaderboardCallback;

bool FOnlineLeaderboardsEOS::ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	if (Rank > EOS_MAX_NUM_RANKINGS)
	{
		UE_LOG_ONLINE_LEADERBOARD(Warning, TEXT("ReadLeaderboardsAroundRank() - Rank (%d) must be <= 1000"), Rank);
		EOSSubsystem->ExecuteNextTick([this]()
			{
				TriggerOnLeaderboardReadCompleteDelegates(false);
			});
		return true;
	}

	// Calculate the range that we'll copy out of the full results
	uint32 StartIndex = (uint32)FMath::Clamp<int32>(Rank - (int32)Range, 0, EOS_MAX_NUM_RANKINGS);
	uint32 EndIndex = FMath::Clamp<uint32>(Rank + (int32)Range, 0, EOS_MAX_NUM_RANKINGS - 1);

	char LeaderboardId[EOS_OSS_STRING_BUFFER_LENGTH];
	EOS_Leaderboards_QueryLeaderboardRanksOptions Options = { };
	Options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDRANKS_API_LATEST;
	Options.LeaderboardId = LeaderboardId;
	Options.LocalUserId = EOSSubsystem->UserManager->GetLocalProductUserId(0);
	FCStringAnsi::Strncpy(LeaderboardId, TCHAR_TO_UTF8(*ReadObject->LeaderboardName.ToString()), EOS_OSS_STRING_BUFFER_LENGTH);

	FQueryLeaderboardCallback* CallbackObj = new FQueryLeaderboardCallback(FOnlineLeaderboardsEOSWeakPtr(AsShared()));
	FOnlineLeaderboardReadRef LambdaReadObject = ReadObject;
	CallbackObj->CallbackLambda = [this, LambdaReadObject, StartIndex, EndIndex](const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* Data)
	{
		bool bWasSuccessful = Data->ResultCode == EOS_EResult::EOS_Success;
		if (!bWasSuccessful)
		{
			UE_LOG_ONLINE_LEADERBOARD(Error, TEXT("EOS_Leaderboards_QueryLeaderboardRanks() failed with EOS result code (%s)"), ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			LambdaReadObject->ReadState = EOnlineAsyncTaskState::Failed;
			TriggerOnLeaderboardReadCompleteDelegates(false);
			return;
		}
		EOS_Leaderboards_GetLeaderboardRecordCountOptions CountOptions = { };
		CountOptions.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDRECORDCOUNT_API_LATEST;

		uint32 LeaderboardCount = EOS_Leaderboards_GetLeaderboardRecordCount(EOSSubsystem->LeaderboardsHandle, &CountOptions);
		// Handle fewer entries than our start index
		if (LeaderboardCount <= StartIndex)
		{
			LambdaReadObject->ReadState = EOnlineAsyncTaskState::Done;
			TriggerOnLeaderboardReadCompleteDelegates(false);
			return;
		}
		// Handle fewer entries than our ending page index
		uint32 NewEndIndex = FMath::Clamp<uint32>(EndIndex, 0, LeaderboardCount - 1);

		EOS_Leaderboards_CopyLeaderboardRecordByIndexOptions CopyOptions = { };
		CopyOptions.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDRECORDBYINDEX_API_LATEST;

		EOS_Connect_GetProductUserIdMappingOptions Options = { };
		Options.ApiVersion = EOS_CONNECT_GETPRODUCTUSERIDMAPPING_API_LATEST;
		Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
		Options.LocalUserId = EOSSubsystem->UserManager->GetLocalProductUserId(0);

		// Allow for us reading a page out of the whole stats range returned
		for (uint32 Index = StartIndex; Index <= NewEndIndex; Index++)
		{
			CopyOptions.LeaderboardRecordIndex = Index;

			EOS_Leaderboards_LeaderboardRecord* Record = nullptr;
			EOS_EResult Result = EOS_Leaderboards_CopyLeaderboardRecordByIndex(EOSSubsystem->LeaderboardsHandle, &CopyOptions, &Record);
			if (Result == EOS_EResult::EOS_Success)
			{
				char EpicIdStr[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH+1];
				int32 EpicIdStrSize = sizeof(EpicIdStr);
				Options.TargetProductUserId = Record->UserId;

				Result = EOS_Connect_GetProductUserIdMapping(EOSSubsystem->ConnectHandle, &Options, EpicIdStr, &EpicIdStrSize);
				if (Result == EOS_EResult::EOS_Success)
				{
					FString Nickname = UTF8_TO_TCHAR(Record->UserDisplayName);
					if(Nickname.IsEmpty())
					{
						Nickname = TEXT("Unknown Player");
					}
					const EOS_EpicAccountId AccountId = EOS_EpicAccountId_FromString(EpicIdStr);
					const FUniqueNetIdEOSRef NetId = FUniqueNetIdEOSRegistry::FindOrAdd(AccountId, Record->UserId).ToSharedRef();

					FOnlineStatsRow* Row = new(LambdaReadObject->Rows) FOnlineStatsRow(Nickname, NetId);
					Row->Rank = Record->Rank;
					Row->Columns.Add(LambdaReadObject->SortedColumn, FVariantData(Record->Score));
				}
			}
		}

		LambdaReadObject->ReadState = EOnlineAsyncTaskState::Done;
		TriggerOnLeaderboardReadCompleteDelegates(true);
	};

	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	EOS_Leaderboards_QueryLeaderboardRanks(EOSSubsystem->LeaderboardsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}

bool FOnlineLeaderboardsEOS::ReadLeaderboardsAroundUser(FUniqueNetIdRef Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	TArray<FUniqueNetIdRef> Players;
	Players.Add(Player);

	return ReadLeaderboards(Players, ReadObject);
}

void FOnlineLeaderboardsEOS::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	// Nothing needed to be done here
}

bool FOnlineLeaderboardsEOS::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	TArray<FOnlineStatsUserUpdatedStats> StatsToWrite;

	FOnlineStatsUserUpdatedStats& UpdatedStats = StatsToWrite.Emplace_GetRef(Player.AsShared());
	for (const TPair<FName, FVariantData>& Stat : WriteObject.Properties)
	{
		UpdatedStats.Stats.Add(Stat.Key.ToString(), FOnlineStatUpdate(Stat.Value, FOnlineStatUpdate::EOnlineStatModificationType::Unknown));
	}

	EOSSubsystem->StatsInterfacePtr->UpdateStats(Player.AsShared(), StatsToWrite, FOnlineStatsUpdateStatsComplete());
	return true;
}

bool FOnlineLeaderboardsEOS::FlushLeaderboards(const FName& SessionName)
{
	UE_LOG_ONLINE_LEADERBOARD(Warning, TEXT("FlushLeaderboards() is not supported"));

	EOSSubsystem->ExecuteNextTick([this, SessionName]()
		{
			TriggerOnLeaderboardFlushCompleteDelegates(SessionName, false);
		});
	
	return true;
}

bool FOnlineLeaderboardsEOS::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	return false;
}

#endif
