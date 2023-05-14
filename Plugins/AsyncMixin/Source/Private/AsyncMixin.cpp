// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncMixin.h"
#include "Containers/Ticker.h"
#include "Engine/AssetManager.h"
#include "Stats/Stats.h"

DEFINE_LOG_CATEGORY_STATIC(LogAsyncMixin, Log, All);

TMap<FAsyncMixin*, TSharedRef<FAsyncMixin::FLoadingState>> FAsyncMixin::Loading;

FAsyncMixin::FAsyncMixin()
{
}

FAsyncMixin::~FAsyncMixin()
{
	check(IsInGameThread());

	// Removing the loading state will cancel any pending loadings it was 
	// monitoring, and shouldn't receive any future callbacks for completion.
	Loading.Remove(this);
}

const FAsyncMixin::FLoadingState& FAsyncMixin::GetLoadingStateConst() const
{
	check(IsInGameThread());
	return Loading.FindChecked(this).Get();
}

FAsyncMixin::FLoadingState& FAsyncMixin::GetLoadingState()
{
	check(IsInGameThread());

	if (TSharedRef<FLoadingState>* LoadingState = Loading.Find(this))
	{
		return (*LoadingState).Get();
	}

	return Loading.Add(this, MakeShared<FLoadingState>(*this)).Get();
}

bool FAsyncMixin::HasLoadingState() const
{
	check(IsInGameThread());

	return Loading.Contains(this);
}

void FAsyncMixin::CancelAsyncLoading()
{
	// Don't create the loading state if we don't have anything pending.
	if (HasLoadingState())
	{
		GetLoadingState().CancelAndDestroy();
	}
}

bool FAsyncMixin::IsAsyncLoadingInProgress() const
{
	// Don't create the loading state if we don't have anything pending.
	if (HasLoadingState())
	{
		return GetLoadingStateConst().IsLoadingInProgress();
	}

	return false;
}

bool FAsyncMixin::IsLoadingInProgressOrPending() const
{
	if (HasLoadingState())
	{
		return GetLoadingStateConst().IsLoadingInProgressOrPending();
	}

	return false;
}

void FAsyncMixin::AsyncLoad(FSoftObjectPath SoftObjectPath, const FSimpleDelegate& DelegateToCall)
{
	GetLoadingState().AsyncLoad(SoftObjectPath, DelegateToCall);
}

void FAsyncMixin::AsyncLoad(const TArray<FSoftObjectPath>& SoftObjectPaths, const FSimpleDelegate& DelegateToCall)
{
	GetLoadingState().AsyncLoad(SoftObjectPaths, DelegateToCall);
}

void FAsyncMixin::AsyncPreloadPrimaryAssetsAndBundles(const TArray<FPrimaryAssetId>& AssetIds, const TArray<FName>& LoadBundles, const FSimpleDelegate& DelegateToCall)
{
	GetLoadingState().AsyncPreloadPrimaryAssetsAndBundles(AssetIds, LoadBundles, DelegateToCall);
}

void FAsyncMixin::AsyncCondition(TSharedRef<FAsyncCondition> Condition, const FSimpleDelegate& Callback)
{
	GetLoadingState().AsyncCondition(Condition, Callback);
}

void FAsyncMixin::AsyncEvent(const FSimpleDelegate& Callback)
{
	GetLoadingState().AsyncEvent(Callback);
}

void FAsyncMixin::StartAsyncLoading()
{
	// If we don't actually have any loading state because they've not queued anything to load,
	// just immediately start and finish the operation by calling the callbacks, no point in allocating
	// the memory just to de-allocate it.
	if (IsLoadingInProgressOrPending())
	{
		GetLoadingState().Start();
	}
	else
	{
		OnStartedLoading();
		OnFinishedLoading();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

FAsyncMixin::FLoadingState::FLoadingState(FAsyncMixin& InOwner)
	: OwnerRef(InOwner)
{
}

FAsyncMixin::FLoadingState::~FLoadingState()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FAsyncMixin_FLoadingState_DestroyThisMemoryDelegate);
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Destroy LoadingState (Done)"), this);

	// If we get destroyed, need to cancel whatever we're doing and cancel any
	// pending destruction - as we're already on the way out.
	CancelOnly(/*bDestroying*/true);
	CancelDestroyThisMemory(/*bDestroying*/true);
}

void FAsyncMixin::FLoadingState::CancelOnly(bool bDestroying)
{
	if (!bDestroying)
	{
		UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Cancel"), this);
	}

	CancelStartTimer();

	for (TUniquePtr<FAsyncStep>& Step : AsyncSteps)
	{
		Step->Cancel();
	}

	// Moving the memory to another array so we don't crash.
	// There was an issue where the Step would get corrupted because we were calling Reset() on the array.
	AsyncStepsPendingDestruction = MoveTemp(AsyncSteps);

	bPreloadedBundles = false;
	bHasStarted = false;
	CurrentAsyncStep = 0;
}

void FAsyncMixin::FLoadingState::CancelAndDestroy()
{
	CancelOnly(/*bDestroying*/false);
	RequestDestroyThisMemory();
}

void FAsyncMixin::FLoadingState::CancelDestroyThisMemory(bool bDestroying)
{
	// If we've schedule the memory to be deleted we need to abort that.
	if (IsPendingDestroy())
	{
		if (!bDestroying)
		{
			UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Destroy LoadingState (Canceled)"), this);
		}

		FTSTicker::GetCoreTicker().RemoveTicker(DestroyMemoryDelegate);
		DestroyMemoryDelegate.Reset();
	}
}

void FAsyncMixin::FLoadingState::RequestDestroyThisMemory()
{
	// If we're already pending to destroy this memory, just ignore.
	if (!IsPendingDestroy())
	{
		UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Destroy LoadingState (Requested)"), this);

		DestroyMemoryDelegate = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) {
			// Remove any memory we were using.
			FAsyncMixin::Loading.Remove(&OwnerRef);
			return false;
		}));
	}
}

void FAsyncMixin::FLoadingState::CancelStartTimer()
{
	if (StartTimerDelegate.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StartTimerDelegate);
		StartTimerDelegate.Reset();
	}
}

void FAsyncMixin::FLoadingState::Start()
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Start (Current Progress %d/%d)"), this, CurrentAsyncStep + 1, AsyncSteps.Num());

	// Cancel any pending kickoff load requests.
	CancelStartTimer();

	bool bStartingStepFound = false;

	if (!bHasStarted)
	{
		bHasStarted = true;
		OwnerRef.OnStartedLoading();
	}
	
	TryCompleteAsyncLoading();
}

void FAsyncMixin::FLoadingState::AsyncLoad(FSoftObjectPath SoftObjectPath, const FSimpleDelegate& DelegateToCall)
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] AsyncLoad '%s'"), this, *SoftObjectPath.ToString());

	AsyncSteps.Add(
		MakeUnique<FAsyncStep>(
			DelegateToCall,
			UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftObjectPath, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, false, false, TEXT("AsyncMixin"))
			)
	);

	TryScheduleStart();
}

void FAsyncMixin::FLoadingState::AsyncLoad(const TArray<FSoftObjectPath>& SoftObjectPaths, const FSimpleDelegate& DelegateToCall)
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] AsyncLoad [%s]"), this, *FString::JoinBy(SoftObjectPaths, TEXT(", "), [](const FSoftObjectPath& SoftObjectPath) { return FString::Printf(TEXT("'%s'"), *SoftObjectPath.ToString()); }));

	AsyncSteps.Add(
		MakeUnique<FAsyncStep>(
			DelegateToCall,
			UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftObjectPaths, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, false, false, TEXT("AsyncMixin"))
			)
	);

	TryScheduleStart();
}

void FAsyncMixin::FLoadingState::AsyncPreloadPrimaryAssetsAndBundles(const TArray<FPrimaryAssetId>& AssetIds, const TArray<FName>& LoadBundles, const FSimpleDelegate& DelegateToCall)
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X]  AsyncPreload Assets [%s], Bundles[%s]"),
		this,
		*FString::JoinBy(AssetIds, TEXT(", "), [](const FPrimaryAssetId& AssetId) { return *AssetId.ToString(); }),
		*FString::JoinBy(LoadBundles, TEXT(", "), [](const FName& LoadBundle) { return *LoadBundle.ToString(); })
	);

	TSharedPtr<FStreamableHandle> StreamingHandle;

	if (AssetIds.Num() > 0)
	{
		bPreloadedBundles = true;

		const bool bLoadRecursive = true;
		StreamingHandle = UAssetManager::Get().PreloadPrimaryAssets(AssetIds, LoadBundles, bLoadRecursive);
	}

	AsyncSteps.Add(MakeUnique<FAsyncStep>(DelegateToCall, StreamingHandle));

	TryScheduleStart();
}

void FAsyncMixin::FLoadingState::AsyncCondition(TSharedRef<FAsyncCondition> Condition, const FSimpleDelegate& DelegateToCall)
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] AsyncCondition '0x%X'"), this, &Condition.Get());

	AsyncSteps.Add(MakeUnique<FAsyncStep>(DelegateToCall, Condition));

	TryScheduleStart();
}

void FAsyncMixin::FLoadingState::AsyncEvent(const FSimpleDelegate& DelegateToCall)
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] AsyncEvent"), this);

	AsyncSteps.Add(MakeUnique<FAsyncStep>(DelegateToCall));

	TryScheduleStart();
}

void FAsyncMixin::FLoadingState::TryScheduleStart()
{
	CancelDestroyThisMemory(/*bDestroying*/false);

	// In the event the user forgets to start async loading, we'll begin doing it next frame.
	if (!StartTimerDelegate.IsValid())
	{
		StartTimerDelegate = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) {
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FAsyncMixin_FLoadingState_TryScheduleStartDelegate);
			Start();
			return false;
		}));
	}
}

bool FAsyncMixin::FLoadingState::IsLoadingInProgress() const
{
	if (AsyncSteps.Num() > 0)
	{
		if (CurrentAsyncStep < AsyncSteps.Num())
		{
			if (CurrentAsyncStep == (AsyncSteps.Num() - 1))
			{
				return AsyncSteps[CurrentAsyncStep]->IsLoadingInProgress();
			}

			// If we know it's a valid index, but not the last one, then we know we're still loading,
			// if it's not a valid index, we know there's no loading, or we're beyond any loading.
			return true;
		}
	}

	return false;
}

bool FAsyncMixin::FLoadingState::IsLoadingInProgressOrPending() const
{
	return StartTimerDelegate.IsValid() || IsLoadingInProgress();
}

bool FAsyncMixin::FLoadingState::IsPendingDestroy() const
{
	return DestroyMemoryDelegate.IsValid();
}

void FAsyncMixin::FLoadingState::TryCompleteAsyncLoading()
{
	// If we haven't started when we get this callback it means we've already completed
	// and this is some other callback finishing on the same frame/stack that we need to avoid
	// doing anything with until the memory is finished being deleted.
	if (!bHasStarted)
	{
		return;
	}

	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] TryCompleteAsyncLoading - (Current Progress %d/%d)"), this, CurrentAsyncStep + 1, AsyncSteps.Num());

	while (CurrentAsyncStep < AsyncSteps.Num())
	{
		FAsyncStep* Step = AsyncSteps[CurrentAsyncStep].Get();
		if (Step->IsLoadingInProgress())
		{
			if (!Step->IsCompleteDelegateBound())
			{
				UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Step %d - Still Loading (Listening)"), this, CurrentAsyncStep + 1);
				const bool bBound = Step->BindCompleteDelegate(FSimpleDelegate::CreateSP(this, &FLoadingState::TryCompleteAsyncLoading));
				ensureMsgf(bBound, TEXT("This is not intended to return false.  We're checking if it's loaded above, this should definitely return true."));
			}
			else
			{
				UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Step %d - Still Loading (Waiting)"), this, CurrentAsyncStep + 1);
			}

			break;
		}
		else
		{
			UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] Step %d - Completed (Calling User)"), this, CurrentAsyncStep + 1);

			// Always advance the CurrentAsyncStep, before calling the user callback, it's possible they might
			// add new work, and try and start again, so we need to be ready for the next bit.
			CurrentAsyncStep++;

			Step->ExecuteUserCallback();
		}
	}
	
	// If we're done loading, and bHasStarted is still true (meaning this is the first time we're encountering a request to complete)
	// try and complete.  It's entirely possible that a user callback might append new work, which they immediately start, which
	// immediately tries to complete, which might create a case where we're now inside of TryCompleteAsyncLoading, which then
	// calls Start, which then calls TryCompleteAsyncLoading, so when we come back out of the stack, we need to avoid trying to
	// complete the async loading N+ times.
	if (IsLoadingComplete() && bHasStarted)
	{
		CompleteAsyncLoading();
	}
}

void FAsyncMixin::FLoadingState::CompleteAsyncLoading()
{
	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] CompleteAsyncLoading"), this);

	// Mark that we've completed loading.
	if (bHasStarted)
	{
		bHasStarted = false;
		OwnerRef.OnFinishedLoading();
	}

	// It's unlikely but possible they started loading more stuff in the OnFinishedLoading callback,
	// so double check that we're still actually done.
	//
	// NOTE: We don't delete ourselves from memory in use.  Doing things like
	// pre-loading a bundle requires keeping the streaming handle alive.  So we're keeping
	// things alive.
	// 
	// We won't destroy the memory but we need to cleanup anything that may be hanging on to
	// captured scope, like completion handlers.
	if (IsLoadingComplete())
	{
		if (!bPreloadedBundles && !IsLoadingInProgressOrPending())
		{
			// If we're all done loading or pending loading, we should clean up the memory we're using.
			// go ahead and remove this loading state the owner mix-in allocated.
			RequestDestroyThisMemory();
			return;
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

FAsyncMixin::FLoadingState::FAsyncStep::FAsyncStep(const FSimpleDelegate& InUserCallback)
	: UserCallback(InUserCallback)
{
}

FAsyncMixin::FLoadingState::FAsyncStep::FAsyncStep(const FSimpleDelegate& InUserCallback, const TSharedPtr<FStreamableHandle>& InStreamingHandle)
	: UserCallback(InUserCallback)
	, StreamingHandle(InStreamingHandle)
{
}

FAsyncMixin::FLoadingState::FAsyncStep::FAsyncStep(const FSimpleDelegate& InUserCallback, const TSharedPtr<FAsyncCondition>& InCondition)
	: UserCallback(InUserCallback)
	, Condition(InCondition)
{
}

FAsyncMixin::FLoadingState::FAsyncStep::~FAsyncStep()
{

}

void FAsyncMixin::FLoadingState::FAsyncStep::ExecuteUserCallback()
{
	UserCallback.ExecuteIfBound();
	UserCallback.Unbind();
}

bool FAsyncMixin::FLoadingState::FAsyncStep::IsComplete() const
{
	if (StreamingHandle.IsValid())
	{
		return StreamingHandle->HasLoadCompleted();
	}
	else if (Condition.IsValid())
	{
		return Condition->IsComplete();
	}

	return true;
}

void FAsyncMixin::FLoadingState::FAsyncStep::Cancel()
{
	if (StreamingHandle.IsValid())
	{
		StreamingHandle->BindCompleteDelegate(FSimpleDelegate());
		StreamingHandle.Reset();
	}
	else if (Condition.IsValid())
	{
		Condition.Reset();
	}

	bIsCompletionDelegateBound = false;
}

bool FAsyncMixin::FLoadingState::FAsyncStep::BindCompleteDelegate(const FSimpleDelegate& NewDelegate)
{
	if (IsComplete())
	{
		// Too Late!
		return false;
	}

	if (StreamingHandle.IsValid())
	{
		StreamingHandle->BindCompleteDelegate(NewDelegate);
	}
	else if (Condition)
	{
		Condition->BindCompleteDelegate(NewDelegate);
	}

	bIsCompletionDelegateBound = true;

	return true;
}

bool FAsyncMixin::FLoadingState::FAsyncStep::IsCompleteDelegateBound() const
{
	return bIsCompletionDelegateBound;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

FAsyncCondition::FAsyncCondition(const FAsyncConditionDelegate& Condition)
	: UserCondition(Condition)
{
}

FAsyncCondition::FAsyncCondition(TFunction<EAsyncConditionResult()>&& Condition)
	: UserCondition(FAsyncConditionDelegate::CreateLambda([UserFunction = MoveTemp(Condition)]() mutable { return UserFunction(); }))
{
}

FAsyncCondition::~FAsyncCondition()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RepeatHandle);
}

bool FAsyncCondition::IsComplete() const
{
	if (UserCondition.IsBound())
	{
		const EAsyncConditionResult Result = UserCondition.Execute();
		return Result == EAsyncConditionResult::Complete;
	}

	return true;
}

bool FAsyncCondition::BindCompleteDelegate(const FSimpleDelegate& NewDelegate)
{
	if (IsComplete())
	{
		// Already Complete
		return false;
	}

	CompletionDelegate = NewDelegate;

	if (!RepeatHandle.IsValid())
	{
		RepeatHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FAsyncCondition::TryToContinue), 0.16);
	}

	return true;
}

bool FAsyncCondition::TryToContinue(float)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FAsyncCondition_TryToContinue);

	UE_LOG(LogAsyncMixin, Verbose, TEXT("[0x%X] AsyncCondition::TryToContinue"), this);

	if (UserCondition.IsBound())
	{
		const EAsyncConditionResult Result = UserCondition.Execute();

		switch (Result)
		{
		case EAsyncConditionResult::TryAgain:
			return true;
		case EAsyncConditionResult::Complete:
			RepeatHandle.Reset();
			UserCondition.Unbind();

			CompletionDelegate.ExecuteIfBound();
			CompletionDelegate.Unbind();
			break;
		}
	}

	return false;
}
