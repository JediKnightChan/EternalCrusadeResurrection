// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ECRAssetManager.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/ECRGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Stats/StatsMisc.h"
#include "Engine/Engine.h"
#include "Gameplay/GAS/ECRGameplayCueManager.h"
#include "Misc/ScopedSlowTask.h"

const FName FECRBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("ECR.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(UECRAssetManager::DumpLoadedAssets)
);

//////////////////////////////////////////////////////////////////////

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FECRAssetManagerStartupJob(#JobFunc, [this](const FECRAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

//////////////////////////////////////////////////////////////////////

UECRAssetManager::UECRAssetManager()
{
	DefaultPawnData = nullptr;
}

UECRAssetManager& UECRAssetManager::Get()
{
	check(GEngine);

	if (UECRAssetManager* Singleton = Cast<UECRAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogECR, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to ECRAssetManager!"));

	// Fatal error above prevents this from being called.
	return *NewObject<UECRAssetManager>();
}

UObject* UECRAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::IsValid())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool UECRAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UECRAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void UECRAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogECR, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : Get().LoadedAssets)
	{
		UE_LOG(LogECR, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogECR, Log, TEXT("... %d assets in loaded pool"), Get().LoadedAssets.Num());
	UE_LOG(LogECR, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

void UECRAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UECRAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	STARTUP_JOB(InitializeAbilitySystem());
	STARTUP_JOB(InitializeGameplayCueManager());

	// Run all the queued up startup jobs
	DoAllStartupJobs();
}

void UECRAssetManager::InitializeAbilitySystem()
{
	SCOPED_BOOT_TIMING("UECRAssetManager::InitializeAbilitySystem");

	FECRGameplayTags::InitializeNativeTags();

	UAbilitySystemGlobals::Get().InitGlobalData();
}

void UECRAssetManager::InitializeGameplayCueManager()
{
	SCOPED_BOOT_TIMING("UECRAssetManager::InitializeGameplayCueManager");

	UECRGameplayCueManager* GCM = UECRGameplayCueManager::Get();
	check(GCM);
	GCM->LoadAlwaysLoadedCues();
}


const UECRPawnData* UECRAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}


void UECRAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("UECRAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		// No need for periodic progress updates, just run the jobs
		for (const FECRAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			float TotalJobValue = 0.0f;
			for (const FECRAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}

			float AccumulatedJobValue = 0.0f;
			for (FECRAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				const float JobValue = StartupJob.JobWeight;
				StartupJob.SubstepProgressDelegate.BindLambda([This = this, AccumulatedJobValue, JobValue, TotalJobValue](float NewProgress)
					{
						const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.0f, 1.0f) * JobValue;
						const float OverallPercentWithSubstep = (AccumulatedJobValue + SubstepAdjustment) / TotalJobValue;

						This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
					});

				StartupJob.DoJob();

				StartupJob.SubstepProgressDelegate.Unbind();

				AccumulatedJobValue += JobValue;

				UpdateInitialGameContentLoadPercent(AccumulatedJobValue / TotalJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.0f);
		}
	}

	StartupJobs.Empty();

	UE_LOG(LogECR, Display, TEXT("All startup jobs took %.2f seconds to complete"), FPlatformTime::Seconds() - AllStartupJobsStartTime);
}

void UECRAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// Could route this to the early startup loading screen
}

#if WITH_EDITOR
void UECRAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("ECREditor", "BeginLoadingPIEData", "Loading PIE Data"));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		// You could add preloading of anything else needed for the experience we'll be using here
		// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	}
}
#endif