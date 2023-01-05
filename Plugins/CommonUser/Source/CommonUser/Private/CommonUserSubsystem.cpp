// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonUserSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "UObject/UObjectHash.h"

#if COMMONUSER_OSSV1
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#else
#include "Online/Auth.h"
#include "Online/ExternalUI.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"
#include "Online/Privileges.h"

using namespace UE::Online;
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogCommonUser, Log, All);
DEFINE_LOG_CATEGORY(LogCommonUser);

UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Error, "SystemMessage.Error");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Warning, "SystemMessage.Warning");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Display, "SystemMessage.Display");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::SystemMessage_Error_InitializeLocalPlayerFailed, "SystemMessage.Error.InitializeLocalPlayerFailed");

UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::Platform_Trait_RequiresStrictControllerMapping, "Platform.Trait.RequiresStrictControllerMapping");
UE_DEFINE_GAMEPLAY_TAG(FCommonUserTags::Platform_Trait_SingleOnlineUser, "Platform.Trait.SingleOnlineUser");


//////////////////////////////////////////////////////////////////////
// UCommonUserInfo

UCommonUserInfo::FCachedData* UCommonUserInfo::GetCachedData(ECommonUserOnlineContext Context)
{
	// Look up directly, game has a separate cache than default
	FCachedData* FoundData = CachedDataMap.Find(Context);
	if (FoundData)
	{
		return FoundData;
	}

	// Now try system resolution
	UCommonUserSubsystem* Subsystem = GetSubsystem();

	ECommonUserOnlineContext ResolvedContext = Subsystem->ResolveOnlineContext(Context);
	return CachedDataMap.Find(ResolvedContext);
}

const UCommonUserInfo::FCachedData* UCommonUserInfo::GetCachedData(ECommonUserOnlineContext Context) const
{
	return const_cast<UCommonUserInfo*>(this)->GetCachedData(Context);
}

void UCommonUserInfo::UpdateCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context)
{
	// This should only be called with a resolved and valid type
	FCachedData* GameCache = GetCachedData(ECommonUserOnlineContext::Game);
	FCachedData* ContextCache = GetCachedData(Context);

	if (!ensure(GameCache && ContextCache))
	{
		// Should always be valid
		return;
	}

	// Update direct cache first
	ContextCache->CachedPrivileges.Add(Privilege, Result);

	if (GameCache != ContextCache)
	{
		// Look for another context to merge into game
		ECommonUserPrivilegeResult GameContextResult = Result;
		ECommonUserPrivilegeResult OtherContextResult = ECommonUserPrivilegeResult::Available;
		for (TPair<ECommonUserOnlineContext, FCachedData>& Pair : CachedDataMap)
		{
			if (&Pair.Value != ContextCache && &Pair.Value != GameCache)
			{
				ECommonUserPrivilegeResult* FoundResult = Pair.Value.CachedPrivileges.Find(Privilege);
				if (FoundResult)
				{
					OtherContextResult = *FoundResult;
				}
				else
				{
					OtherContextResult = ECommonUserPrivilegeResult::Unknown;
				}
				break;
			}
		}

		if (GameContextResult == ECommonUserPrivilegeResult::Available && OtherContextResult != ECommonUserPrivilegeResult::Available)
		{
			// Other context is worse, use that
			GameContextResult = OtherContextResult;
		}

		GameCache->CachedPrivileges.Add(Privilege, GameContextResult);
	}
}

void UCommonUserInfo::UpdateCachedNetId(const FUniqueNetIdRepl& NewId, ECommonUserOnlineContext Context)
{
	FCachedData* ContextCache = GetCachedData(Context);

	if (ensure(ContextCache))
	{
		ContextCache->CachedNetId = NewId;
	}

	// We don't merge the ids because of how guests work
}

class UCommonUserSubsystem* UCommonUserInfo::GetSubsystem() const
{
	return Cast<UCommonUserSubsystem>(GetOuter());
}

ECommonUserPrivilegeResult UCommonUserInfo::GetCachedPrivilegeResult(ECommonUserPrivilege Privilege, ECommonUserOnlineContext Context) const
{
	const FCachedData* FoundCached = GetCachedData(Context);

	if (FoundCached)
	{
		const ECommonUserPrivilegeResult* FoundResult = FoundCached->CachedPrivileges.Find(Privilege);
		if (FoundResult)
		{
			return *FoundResult;
		}
	}
	return ECommonUserPrivilegeResult::Unknown;
}

ECommonUserAvailability UCommonUserInfo::GetPrivilegeAvailability(ECommonUserPrivilege Privilege) const
{
	// Bad feature or user
	if ((int32)Privilege < 0 || (int32)Privilege >= (int32)ECommonUserPrivilege::Invalid_Count || InitializationState == ECommonUserInitializationState::Invalid)
	{
		return ECommonUserAvailability::Invalid;
	}

	ECommonUserPrivilegeResult CachedResult = GetCachedPrivilegeResult(Privilege, ECommonUserOnlineContext::Game);

	// First handle explicit failures
	switch (CachedResult)
	{
	case ECommonUserPrivilegeResult::LicenseInvalid:
	case ECommonUserPrivilegeResult::VersionOutdated:
	case ECommonUserPrivilegeResult::AgeRestricted:
		return ECommonUserAvailability::AlwaysUnavailable;

	case ECommonUserPrivilegeResult::NetworkConnectionUnavailable:
	case ECommonUserPrivilegeResult::AccountTypeRestricted:
	case ECommonUserPrivilegeResult::AccountUseRestricted:
	case ECommonUserPrivilegeResult::PlatformFailure:
		return ECommonUserAvailability::CurrentlyUnavailable;

	default:
		break;
	}

	if (bIsGuest)
	{
		// Guests can only play, cannot use online features
		if (Privilege == ECommonUserPrivilege::CanPlay)
		{
			return ECommonUserAvailability::NowAvailable;
		}
		else
		{
			return ECommonUserAvailability::AlwaysUnavailable;
		}
	}

	// Check network status
	if (Privilege == ECommonUserPrivilege::CanPlayOnline ||
		Privilege == ECommonUserPrivilege::CanUseCrossPlay ||
		Privilege == ECommonUserPrivilege::CanCommunicateViaTextOnline ||
		Privilege == ECommonUserPrivilege::CanCommunicateViaVoiceOnline)
	{
		UCommonUserSubsystem* Subsystem = GetSubsystem();
		if (ensure(Subsystem) && !Subsystem->HasOnlineConnection(ECommonUserOnlineContext::Game))
		{
			return ECommonUserAvailability::CurrentlyUnavailable;
		}
	}

	if (InitializationState == ECommonUserInitializationState::FailedtoLogin)
	{
		// Failed a prior login attempt
		return ECommonUserAvailability::CurrentlyUnavailable;
	}
	else if (InitializationState == ECommonUserInitializationState::Unknown || InitializationState == ECommonUserInitializationState::DoingInitialLogin)
	{
		// Haven't logged in yet
		return ECommonUserAvailability::PossiblyAvailable;
	}
	else if (InitializationState == ECommonUserInitializationState::LoggedInLocalOnly || InitializationState == ECommonUserInitializationState::DoingNetworkLogin)
	{
		// Local login succeeded so play checks are valid
		if (Privilege == ECommonUserPrivilege::CanPlay && CachedResult == ECommonUserPrivilegeResult::Available)
		{
			return ECommonUserAvailability::NowAvailable;
		}

		// Haven't logged in online yet
		return ECommonUserAvailability::PossiblyAvailable;
	}
	else if (InitializationState == ECommonUserInitializationState::LoggedInOnline)
	{
		// Fully logged in
		if (CachedResult == ECommonUserPrivilegeResult::Available)
		{
			return ECommonUserAvailability::NowAvailable;
		}

		// Failed for other reason
		return ECommonUserAvailability::CurrentlyUnavailable;
	}

	return ECommonUserAvailability::Unknown;
}

FUniqueNetIdRepl UCommonUserInfo::GetNetId(ECommonUserOnlineContext Context) const
{
	const FCachedData* FoundCached = GetCachedData(Context);

	if (FoundCached)
	{
		return FoundCached->CachedNetId;
	}

	return FUniqueNetIdRepl();
}

FString UCommonUserInfo::GetNickname() const
{
	if (bIsGuest)
	{
		return NSLOCTEXT("CommonUser", "GuestNickname", "Guest").ToString();
	}

	const UCommonUserSubsystem* Subsystem = GetSubsystem();

	if (ensure(Subsystem))
	{
#if COMMONUSER_OSSV1
		IOnlineIdentity* Identity = Subsystem->GetOnlineIdentity(ECommonUserOnlineContext::Game);
		if (ensure(Identity))
		{
			return Identity->GetPlayerNickname(PlatformUserIndex);
		}
#else
		if (IAuthPtr AuthService = Subsystem->GetOnlineAuth(ECommonUserOnlineContext::Game))
		{
			if (TSharedPtr<FAccountInfo> AccountInfo = Subsystem->GetOnlineServiceAccountInfo(AuthService, FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex)))
			{
				return AccountInfo->DisplayName;
			}
		}
#endif // COMMONUSER_OSSV1
	}
	return FString();
}

FString UCommonUserInfo::GetDebugString() const
{
	FUniqueNetIdRepl NetId = GetNetId();
	return NetId.ToDebugString();
}

FPlatformUserId UCommonUserInfo::GetPlatformUserId() const
{
#if COMMONUSER_OSSV1
	const UCommonUserSubsystem* Subsystem = GetSubsystem();

	if (ensure(Subsystem))
	{
		IOnlineIdentity* Identity = Subsystem->GetOnlineIdentity(ECommonUserOnlineContext::PlatformOrDefault);

		if (ensure(Identity))
		{
			return Identity->GetPlatformUserIdFromLocalUserNum(PlatformUserIndex);
		}
	}
	return PLATFORMUSERID_NONE;
#else
	return FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex);
#endif // COMMONUSER_OSSV1
}


//////////////////////////////////////////////////////////////////////
// UCommonUserSubsystem

void UCommonUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create our OSS wrappers
	CreateOnlineContexts();

	BindOnlineDelegates();

	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &ThisClass::HandleControllerConnectionChanged);

	// Matches the engine default
	SetMaxLocalPlayers(4);

	ResetUserState();
}

void UCommonUserSubsystem::CreateOnlineContexts()
{
	// First initialize default
	DefaultContextInternal = new FOnlineContextCache();
#if COMMONUSER_OSSV1
	DefaultContextInternal->OnlineSubsystem = Online::GetSubsystem(GetWorld());
	check(DefaultContextInternal->OnlineSubsystem);
	DefaultContextInternal->IdentityInterface = DefaultContextInternal->OnlineSubsystem->GetIdentityInterface();
	check(DefaultContextInternal->IdentityInterface.IsValid());

	IOnlineSubsystem* PlatformSub = IOnlineSubsystem::GetByPlatform();

	if (PlatformSub && DefaultContextInternal->OnlineSubsystem != PlatformSub)
	{
		// Set up the optional platform service if it exists
		PlatformContextInternal = new FOnlineContextCache();
		PlatformContextInternal->OnlineSubsystem = PlatformSub;
		PlatformContextInternal->IdentityInterface = PlatformSub->GetIdentityInterface();
		check(PlatformContextInternal->IdentityInterface.IsValid());
	}
#else
	DefaultContextInternal->OnlineServices = GetServices(GetWorld(), EOnlineServices::Default);
	check(DefaultContextInternal->OnlineServices);
	DefaultContextInternal->AuthService = DefaultContextInternal->OnlineServices->GetAuthInterface();
	check(DefaultContextInternal->AuthService);

	UE::Online::IOnlineServicesPtr PlatformServices = GetServices(GetWorld(), EOnlineServices::Platform);
	if (PlatformServices && DefaultContextInternal->OnlineServices != PlatformServices)
	{
		PlatformContextInternal = new FOnlineContextCache();
		PlatformContextInternal->OnlineServices = PlatformServices;
		PlatformContextInternal->AuthService = PlatformContextInternal->OnlineServices->GetAuthInterface();
		check(PlatformContextInternal->AuthService);
	}
#endif

	// Explicit external services can be set up after if needed
}

void UCommonUserSubsystem::Deinitialize()
{
	DestroyOnlineContexts();

	FCoreDelegates::OnControllerConnectionChange.RemoveAll(this);
	LocalUserInfos.Reset();
	ActiveLoginRequests.Reset();

	Super::Deinitialize();
}

void UCommonUserSubsystem::DestroyOnlineContexts()
{
	// All cached shared ptrs must be cleared here
	if (ServiceContextInternal && ServiceContextInternal != DefaultContextInternal)
	{
		delete ServiceContextInternal;
	}
	if (PlatformContextInternal && PlatformContextInternal != DefaultContextInternal)
	{
		delete PlatformContextInternal;
	}
	if (DefaultContextInternal)
	{
		delete DefaultContextInternal;
	}

	ServiceContextInternal = PlatformContextInternal = DefaultContextInternal = nullptr;
}

UCommonUserInfo* UCommonUserSubsystem::CreateLocalUserInfo(int32 LocalPlayerIndex)
{
	UCommonUserInfo* NewUser = nullptr;
	if (ensure(!LocalUserInfos.Contains(LocalPlayerIndex)))
	{
		NewUser = NewObject<UCommonUserInfo>(this);
		NewUser->LocalPlayerIndex = LocalPlayerIndex;
		NewUser->InitializationState = ECommonUserInitializationState::Unknown;

		// Always create game and default cache
		NewUser->CachedDataMap.Add(ECommonUserOnlineContext::Game, UCommonUserInfo::FCachedData());
		NewUser->CachedDataMap.Add(ECommonUserOnlineContext::Default, UCommonUserInfo::FCachedData());

		// Add platform if needed
		if (HasSeparatePlatformContext())
		{
			NewUser->CachedDataMap.Add(ECommonUserOnlineContext::Platform, UCommonUserInfo::FCachedData());
		}

		LocalUserInfos.Add(LocalPlayerIndex, NewUser);
	}
	return NewUser;
}

bool UCommonUserSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass
	return ChildClasses.Num() == 0;
}

void UCommonUserSubsystem::BindOnlineDelegates()
{
#if COMMONUSER_OSSV1
	return BindOnlineDelegatesOSSv1();
#else
	return BindOnlineDelegatesOSSv2();
#endif
}

void UCommonUserSubsystem::LogOutLocalUser(int32 PlatformUserIndex)
{
	UCommonUserInfo* UserInfo = ModifyInfo(GetUserInfoForPlatformUserIndex(PlatformUserIndex));

	// Don't need to do anything if the user has never logged in fully or is in the process of logging in
	if (UserInfo && (UserInfo->InitializationState == ECommonUserInitializationState::LoggedInLocalOnly || UserInfo->InitializationState == ECommonUserInitializationState::LoggedInOnline))
	{
		ECommonUserAvailability OldAvailablity = UserInfo->GetPrivilegeAvailability(ECommonUserPrivilege::CanPlay);

		UserInfo->InitializationState = ECommonUserInitializationState::FailedtoLogin;

		// This will broadcast the game delegate
		HandleChangedAvailability(UserInfo, ECommonUserPrivilege::CanPlay, OldAvailablity);
	}
}

bool UCommonUserSubsystem::TransferPlatformAuth(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
#if COMMONUSER_OSSV1
	// Not supported in V1 path
	return false;
#else
	return TransferPlatformAuthOSSv2(System, Request, PlatformUserIndex);
#endif
}

bool UCommonUserSubsystem::AutoLogin(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
#if COMMONUSER_OSSV1
	return AutoLoginOSSv1(System, Request, PlatformUserIndex);
#else
	return AutoLoginOSSv2(System, Request, PlatformUserIndex);
#endif
}

bool UCommonUserSubsystem::ShowLoginUI(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
#if COMMONUSER_OSSV1
	return ShowLoginUIOSSv1(System, Request, PlatformUserIndex);
#else
	return ShowLoginUIOSSv2(System, Request, PlatformUserIndex);
#endif
}

bool UCommonUserSubsystem::QueryUserPrivilege(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
#if COMMONUSER_OSSV1
	return QueryUserPrivilegeOSSv1(System, Request, PlatformUserIndex);
#else
	return QueryUserPrivilegeOSSv2(System, Request, PlatformUserIndex);
#endif
}


#if COMMONUSER_OSSV1
IOnlineSubsystem* UCommonUserSubsystem::GetOnlineSubsystem(ECommonUserOnlineContext Context) const
{
	const FOnlineContextCache* System = GetContextCache(Context);

	if (System)
	{
		return System->OnlineSubsystem;
	}

	return nullptr;
}

IOnlineIdentity* UCommonUserSubsystem::GetOnlineIdentity(ECommonUserOnlineContext Context) const
{
	const FOnlineContextCache* System = GetContextCache(Context);
	if (System)
	{
		return System->IdentityInterface.Get();
	}

	return nullptr;
}

FName UCommonUserSubsystem::GetOnlineSubsystemName(ECommonUserOnlineContext Context) const
{
	IOnlineSubsystem* SubSystem = GetOnlineSubsystem(Context);
	if (SubSystem)
	{
		return SubSystem->GetSubsystemName();
	}

	return NAME_None;
}

EOnlineServerConnectionStatus::Type UCommonUserSubsystem::GetConnectionStatus(ECommonUserOnlineContext Context) const
{
	const FOnlineContextCache* System = GetContextCache(Context);
	if (System)
	{
		return System->CurrentConnectionStatus;
	}

	return EOnlineServerConnectionStatus::ServiceUnavailable;
}

void UCommonUserSubsystem::BindOnlineDelegatesOSSv1()
{
	ECommonUserOnlineContext ServiceType = ResolveOnlineContext(ECommonUserOnlineContext::ServiceOrDefault);
	ECommonUserOnlineContext PlatformType = ResolveOnlineContext(ECommonUserOnlineContext::PlatformOrDefault);
	FOnlineContextCache* ServiceContext = GetContextCache(ServiceType);
	FOnlineContextCache* PlatformContext = GetContextCache(PlatformType);
	check(ServiceContext && ServiceContext->OnlineSubsystem && PlatformContext && PlatformContext->OnlineSubsystem);
	// Connection delegates need to listen for both systems

	ServiceContext->OnlineSubsystem->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleNetworkConnectionStatusChanged, ServiceType));
	ServiceContext->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;

	for (int32 PlayerIdx = 0; PlayerIdx < MAX_LOCAL_PLAYERS; PlayerIdx++)
	{
		ServiceContext->IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(PlayerIdx, FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleIdentityLoginStatusChanged, ServiceType));
		ServiceContext->IdentityInterface->AddOnLoginCompleteDelegate_Handle(PlayerIdx, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleUserLoginCompleted, ServiceType));
	}

	if (ServiceType != PlatformType)
	{
		PlatformContext->OnlineSubsystem->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleNetworkConnectionStatusChanged, PlatformType));
		PlatformContext->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;

		for (int32 PlayerIdx = 0; PlayerIdx < MAX_LOCAL_PLAYERS; PlayerIdx++)
		{
			PlatformContext->IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(PlayerIdx, FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleIdentityLoginStatusChanged, PlatformType));
			PlatformContext->IdentityInterface->AddOnLoginCompleteDelegate_Handle(PlayerIdx, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleUserLoginCompleted, PlatformType));
		}
	}

	// Hardware change delegates only listen to platform
	PlatformContext->IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &ThisClass::HandleControllerPairingChanged));
}

bool UCommonUserSubsystem::AutoLoginOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	return System->IdentityInterface->AutoLogin(PlatformUserIndex);
}

bool UCommonUserSubsystem::ShowLoginUIOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	IOnlineExternalUIPtr ExternalUI = System->OnlineSubsystem->GetExternalUIInterface();
	if (ExternalUI.IsValid())
	{
		// TODO Unclear which flags should be set
		return ExternalUI->ShowLoginUI(PlatformUserIndex, false, false, FOnLoginUIClosedDelegate::CreateUObject(this, &ThisClass::HandleOnLoginUIClosed, Request->CurrentContext));
	}
	return false;
}

bool UCommonUserSubsystem::QueryUserPrivilegeOSSv1(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	// Start query on unknown or failure
	EUserPrivileges::Type OSSPrivilege = ConvertOSSPrivilege(Request->DesiredPrivilege);

	FUniqueNetIdRepl CurrentId = GetLocalUserNetId(PlatformUserIndex, Request->CurrentContext);
	check(CurrentId.IsValid());
	IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate Delegate = IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &UCommonUserSubsystem::HandleCheckPrivilegesComplete, Request->DesiredPrivilege, Request->UserInfo, Request->CurrentContext);
	System->IdentityInterface->GetUserPrivilege(*CurrentId, OSSPrivilege, Delegate);

	// This may immediately succeed and reenter this function, so we have to return
	return true;
}

#else

UE::Online::EOnlineServices UCommonUserSubsystem::GetOnlineServicesProvider(ECommonUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->OnlineServices->GetServicesProvider();
	}
	return UE::Online::EOnlineServices::None;
}

UE::Online::IAuthPtr UCommonUserSubsystem::GetOnlineAuth(ECommonUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->AuthService;
	}
	return nullptr;
}

UE::Online::EOnlineServicesConnectionStatus UCommonUserSubsystem::GetConnectionStatus(ECommonUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->CurrentConnectionStatus;
	}
	return UE::Online::EOnlineServicesConnectionStatus::NotConnected;
}

void UCommonUserSubsystem::BindOnlineDelegatesOSSv2()
{
	ECommonUserOnlineContext ServiceType = ResolveOnlineContext(ECommonUserOnlineContext::ServiceOrDefault);
	ECommonUserOnlineContext PlatformType = ResolveOnlineContext(ECommonUserOnlineContext::PlatformOrDefault);
	FOnlineContextCache* ServiceContext = GetContextCache(ServiceType);
	FOnlineContextCache* PlatformContext = GetContextCache(PlatformType);
	check(ServiceContext && ServiceContext->OnlineServices && PlatformContext && PlatformContext->OnlineServices);

	ServiceContext->AuthService->OnLoginStatusChanged().Add(this, &ThisClass::HandleAuthLoginStatusChanged, ServiceType);
	if (IConnectivityPtr ConnectivityInterface = ServiceContext->OnlineServices->GetConnectivityInterface())
	{
		ConnectivityInterface->OnConnectionStatusChanged().Add(this, &ThisClass::HandleNetworkConnectionStatusChanged, ServiceType);
	}
	CacheConnectionStatus(ServiceType);

	if (ServiceType != PlatformType)
	{
		PlatformContext->AuthService->OnLoginStatusChanged().Add(this, &ThisClass::HandleAuthLoginStatusChanged, PlatformType);
		if (IConnectivityPtr ConnectivityInterface = PlatformContext->OnlineServices->GetConnectivityInterface())
		{
			ConnectivityInterface->OnConnectionStatusChanged().Add(this, &ThisClass::HandleNetworkConnectionStatusChanged, PlatformType);
		}
		CacheConnectionStatus(PlatformType);
	}
	// TODO:  Controller Pairing Changed - move out of OSS and listen to CoreDelegate directly?
}

void UCommonUserSubsystem::CacheConnectionStatus(ECommonUserOnlineContext Context)
{
	FOnlineContextCache* ContextCache = GetContextCache(Context);
	check(ContextCache);

	EOnlineServicesConnectionStatus ConnectionStatus = EOnlineServicesConnectionStatus::NotConnected;
	if (IConnectivityPtr ConnectivityInterface = ContextCache->OnlineServices->GetConnectivityInterface())
	{
		const TOnlineResult<FGetConnectionStatus> Result = ConnectivityInterface->GetConnectionStatus(FGetConnectionStatus::Params());
		if (Result.IsOk())
		{
			ConnectionStatus = Result.GetOkValue().Status;
		}
	}
	else
	{
		ConnectionStatus = EOnlineServicesConnectionStatus::Connected;
	}

	UE::Online::FConnectionStatusChanged EventParams;
	EventParams.PreviousStatus = ContextCache->CurrentConnectionStatus;
	EventParams.CurrentStatus = ConnectionStatus;
	HandleNetworkConnectionStatusChanged(EventParams, Context);
}

bool UCommonUserSubsystem::TransferPlatformAuthOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	IAuthPtr PlatformAuthInterface = GetOnlineAuth(ECommonUserOnlineContext::Platform);
	if (Request->CurrentContext != ECommonUserOnlineContext::Platform
		&& PlatformAuthInterface)
	{
		FAuthGenerateAuthToken::Params Params;
		Params.LocalUserId = GetLocalUserNetId(PlatformUserIndex, ECommonUserOnlineContext::Platform).GetV2();

		PlatformAuthInterface->GenerateAuthToken(MoveTemp(Params))
		.OnComplete(this, [this, Request](const TOnlineResult<FAuthGenerateAuthToken>& Result)
		{
			UCommonUserInfo* UserInfo = Request->UserInfo.Get();
			if (!UserInfo)
			{
				// User is gone, just delete this request
				ActiveLoginRequests.Remove(Request);
				return;
			}

			if (Result.IsOk())
			{
				const FAuthGenerateAuthToken::Result& GenerateAuthTokenResult = Result.GetOkValue();
				FAuthLogin::Params Params;
				Params.PlatformUserId = UserInfo->GetPlatformUserId();
				Params.CredentialsType = GenerateAuthTokenResult.Type;
				Params.CredentialsToken = GenerateAuthTokenResult.Token;

				IAuthPtr PrimaryAuthInterface = GetOnlineAuth(Request->CurrentContext);
				PrimaryAuthInterface->Login(MoveTemp(Params))
				.OnComplete(this, [this, Request](const TOnlineResult<FAuthLogin>& Result)
				{
					UCommonUserInfo* UserInfo = Request->UserInfo.Get();
					if (!UserInfo)
					{
						// User is gone, just delete this request
						ActiveLoginRequests.Remove(Request);
						return;
					}

					if (Result.IsOk())
					{
						Request->TransferPlatformAuthState = ECommonUserAsyncTaskState::Done;
						Request->Error.Reset();
					}
					else
					{
						Request->TransferPlatformAuthState = ECommonUserAsyncTaskState::Failed;
						Request->Error = Result.GetErrorValue();
					}
					ProcessLoginRequest(Request);
				});
			}
			else
			{
				Request->TransferPlatformAuthState = ECommonUserAsyncTaskState::Failed;
				Request->Error = Result.GetErrorValue();
				ProcessLoginRequest(Request);
			}
		});
		return true;
	}
	return false;
}

bool UCommonUserSubsystem::AutoLoginOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	FAuthLogin::Params LoginParameters;
	LoginParameters.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex);
	// Leave other LoginParameters as default to allow the online service to determine how to try to automatically log in the user
	TOnlineAsyncOpHandle<FAuthLogin> LoginHandle = System->AuthService->Login(MoveTemp(LoginParameters));
	LoginHandle.OnComplete(this, &ThisClass::HandleUserLoginCompletedV2, PlatformUserIndex, Request->CurrentContext);
	return true;
}

bool UCommonUserSubsystem::ShowLoginUIOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	IExternalUIPtr ExternalUI = System->OnlineServices->GetExternalUIInterface();
	if (ExternalUI.IsValid())
	{
		FExternalUIShowLoginUI::Params ShowLoginUIParameters;
		ShowLoginUIParameters.PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex);
		TOnlineAsyncOpHandle<FExternalUIShowLoginUI> LoginHandle = ExternalUI->ShowLoginUI(MoveTemp(ShowLoginUIParameters));
		LoginHandle.OnComplete(this, &ThisClass::HandleOnLoginUIClosedV2, PlatformUserIndex, Request->CurrentContext);
		return true;
	}
	return false;
}

bool UCommonUserSubsystem::QueryUserPrivilegeOSSv2(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, int32 PlatformUserIndex)
{
	UCommonUserInfo* UserInfo = Request->UserInfo.Get();

	if (IPrivilegesPtr PrivilegesInterface = System->OnlineServices->GetPrivilegesInterface())
	{
		const EUserPrivileges DesiredPrivilege = ConvertOnlineServicesPrivilege(Request->DesiredPrivilege);

		FQueryUserPrivilege::Params Params;
		Params.LocalUserId = GetLocalUserNetId(PlatformUserIndex, Request->CurrentContext).GetV2();
		Params.Privilege = DesiredPrivilege;
		TOnlineAsyncOpHandle<FQueryUserPrivilege> QueryHandle = PrivilegesInterface->QueryUserPrivilege(MoveTemp(Params));
		QueryHandle.OnComplete(this, &ThisClass::HandleCheckPrivilegesComplete, Request->UserInfo, DesiredPrivilege, Request->CurrentContext);
		return true;
	}
	else
	{
		UpdateUserPrivilegeResult(UserInfo, Request->DesiredPrivilege, ECommonUserPrivilegeResult::Available, Request->CurrentContext);
	}
	return false;
}

TSharedPtr<FAccountInfo> UCommonUserSubsystem::GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const
{
	TSharedPtr<FAccountInfo> AccountInfo;
	FAuthGetAccountByPlatformUserId::Params GetAccountParams = { InUserId };
	TOnlineResult<FAuthGetAccountByPlatformUserId> GetAccountResult = AuthService->GetAccountByPlatformUserId(MoveTemp(GetAccountParams));
	if (GetAccountResult.IsOk())
	{
		AccountInfo = GetAccountResult.GetOkValue().AccountInfo;
	}
	return AccountInfo;
}

#endif

bool UCommonUserSubsystem::HasOnlineConnection(ECommonUserOnlineContext Context) const
{
#if COMMONUSER_OSSV1
	EOnlineServerConnectionStatus::Type ConnectionType = GetConnectionStatus(Context);

	if (ConnectionType == EOnlineServerConnectionStatus::Normal || ConnectionType == EOnlineServerConnectionStatus::Connected)
	{
		return true;
	}

	return false;
#else
	return GetConnectionStatus(Context) == UE::Online::EOnlineServicesConnectionStatus::Connected;
#endif
}

ELoginStatusType UCommonUserSubsystem::GetLocalUserLoginStatus(int32 PlatformUserIndex, ECommonUserOnlineContext Context) const
{
	if (!IsValidPlatformUserIndex(PlatformUserIndex))
	{
		return ELoginStatusType::NotLoggedIn;
	}

	const FOnlineContextCache* System = GetContextCache(Context);
	if (System)
	{
#if COMMONUSER_OSSV1
		return System->IdentityInterface->GetLoginStatus(PlatformUserIndex);
#else
		if (TSharedPtr<FAccountInfo> AccountInfo = GetOnlineServiceAccountInfo(System->AuthService, FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex)))
		{
			return AccountInfo->LoginStatus;
		}
#endif
	}
	return ELoginStatusType::NotLoggedIn;
}

FUniqueNetIdRepl UCommonUserSubsystem::GetLocalUserNetId(int32 PlatformUserIndex, ECommonUserOnlineContext Context) const
{
	if (!IsValidPlatformUserIndex(PlatformUserIndex))
	{
		return FUniqueNetIdRepl();
	}

	const FOnlineContextCache* System = GetContextCache(Context);
	if (System)
	{
#if COMMONUSER_OSSV1
		return FUniqueNetIdRepl(System->IdentityInterface->GetUniquePlayerId(PlatformUserIndex));
#else
		// TODO:  OSSv2 FUniqueNetIdRepl wrapping FOnlineAccountIdHandle is in progress
		if (TSharedPtr<FAccountInfo> AccountInfo = GetOnlineServiceAccountInfo(System->AuthService, FPlatformMisc::GetPlatformUserForUserIndex(PlatformUserIndex)))
		{
			return FUniqueNetIdRepl(AccountInfo->UserId);
		}
#endif
	}

	return FUniqueNetIdRepl();
}

void UCommonUserSubsystem::SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText)
{
	OnHandleSystemMessage.Broadcast(MessageType, TitleText, BodyText);
}

void UCommonUserSubsystem::SetMaxLocalPlayers(int32 InMaxLocalPlayers)
{
	if (ensure(InMaxLocalPlayers >= 1))
	{
		// We can have more local players than MAX_LOCAL_PLAYERS, the rest are treated as guests
		MaxNumberOfLocalPlayers = InMaxLocalPlayers;

		UGameInstance* GameInstance = GetGameInstance();
		UGameViewportClient* ViewportClient = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;

		if (ViewportClient)
		{
			ViewportClient->MaxSplitscreenPlayers = MaxNumberOfLocalPlayers;
		}
	}
}

int32 UCommonUserSubsystem::GetMaxLocalPlayers() const
{
	return MaxNumberOfLocalPlayers;
}

int32 UCommonUserSubsystem::GetNumLocalPlayers() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (ensure(GameInstance))
	{
		return GameInstance->GetNumLocalPlayers();
	}
	return 1;
}

ECommonUserInitializationState UCommonUserSubsystem::GetLocalPlayerInitializationState(int32 LocalPlayerIndex) const
{
	const UCommonUserInfo* UserInfo = GetUserInfoForLocalPlayerIndex(LocalPlayerIndex);
	if (UserInfo)
	{
		return UserInfo->InitializationState;
	}

	if (LocalPlayerIndex < 0 || LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		return ECommonUserInitializationState::Invalid;
	}

	return ECommonUserInitializationState::Unknown;
}

bool UCommonUserSubsystem::TryToInitializeForLocalPlay(int32 LocalPlayerIndex, int32 ControllerId, bool bCanUseGuestLogin)
{
	if (!ensure(IsValidControllerId(ControllerId)))
	{
		return false;
	}

	FCommonUserInitializeParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.ControllerId = ControllerId;
	Params.bCanUseGuestLogin = bCanUseGuestLogin;
	Params.bCanCreateNewLocalPlayer = true;
	Params.RequestedPrivilege = ECommonUserPrivilege::CanPlay;

	return TryToInitializeUser(Params);
}

bool UCommonUserSubsystem::TryToLoginForOnlinePlay(int32 LocalPlayerIndex)
{
	FCommonUserInitializeParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.bCanCreateNewLocalPlayer = false;
	Params.RequestedPrivilege = ECommonUserPrivilege::CanPlayOnline;

	return TryToInitializeUser(Params);
}

bool UCommonUserSubsystem::TryToInitializeUser(FCommonUserInitializeParams Params)
{
	if (Params.LocalPlayerIndex < 0 || (!Params.bCanCreateNewLocalPlayer && Params.LocalPlayerIndex >= GetNumLocalPlayers()))
	{
		UE_LOG(LogCommonUser, Error, TEXT("TryToInitializeUser %d failed with current %d and max %d, invalid index"), 
			Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
		return false;
	}

	if (Params.LocalPlayerIndex > GetNumLocalPlayers() || Params.LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		UE_LOG(LogCommonUser, Error, TEXT("TryToInitializeUser %d failed with current %d and max %d, can only create in order up to max players"), 
			Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
		return false;
	}

	UCommonUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));
	UCommonUserInfo* LocalUserInfoForController = ModifyInfo(GetUserInfoForControllerId(Params.ControllerId));

	if (LocalUserInfoForController && LocalUserInfo && LocalUserInfoForController != LocalUserInfo)
	{
		UE_LOG(LogCommonUser, Error, TEXT("TryToInitializeUser %d failed because controller %d is already assigned to player %d"),
			Params.LocalPlayerIndex, Params.ControllerId, LocalUserInfoForController->LocalPlayerIndex);
		return false;
	}

	if (Params.LocalPlayerIndex == 0 && Params.bCanUseGuestLogin)
	{
		UE_LOG(LogCommonUser, Error, TEXT("TryToInitializeUser failed because player 0 cannot be a guest"));
		return false;
	}

	// Fill in platform user if possible
	if (IsValidControllerId(Params.ControllerId) && !IsValidPlatformUserIndex(Params.PlatformUserIndex))
	{
		Params.PlatformUserIndex = GetPlatformUserIndexForControllerId(Params.ControllerId);
	}

	if (!LocalUserInfo)
	{
		LocalUserInfo = CreateLocalUserInfo(Params.LocalPlayerIndex);
	}
	else
	{
		// Copy from existing user info
		if (!IsValidControllerId(Params.ControllerId))
		{
			Params.ControllerId = LocalUserInfo->PrimaryControllerId;
		}

		if (!IsValidPlatformUserIndex(Params.PlatformUserIndex))
		{
			Params.PlatformUserIndex = LocalUserInfo->PlatformUserIndex;
		}
	}
	
	if (LocalUserInfo->InitializationState != ECommonUserInitializationState::Unknown && LocalUserInfo->InitializationState != ECommonUserInitializationState::FailedtoLogin)
	{
		// Not allowed to change parameters during login
		if (LocalUserInfo->PrimaryControllerId != Params.ControllerId || LocalUserInfo->PlatformUserIndex != Params.PlatformUserIndex || LocalUserInfo->bCanBeGuest != Params.bCanUseGuestLogin)
		{
			UE_LOG(LogCommonUser, Error, TEXT("TryToInitializeUser failed because player %d has already started the login process with diffrent settings!"), Params.LocalPlayerIndex);
			return false;
		}
	}

	// Set desired index now so if it creates a player it knows what controller to use
	LocalUserInfo->PrimaryControllerId = Params.ControllerId;
	LocalUserInfo->PlatformUserIndex = Params.PlatformUserIndex;
	LocalUserInfo->bCanBeGuest = Params.bCanUseGuestLogin;
	RefreshLocalUserInfo(LocalUserInfo);

	// Either doing an initial or network login
	if (LocalUserInfo->GetPrivilegeAvailability(ECommonUserPrivilege::CanPlay) == ECommonUserAvailability::NowAvailable && Params.RequestedPrivilege == ECommonUserPrivilege::CanPlayOnline)
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::DoingNetworkLogin;
	}
	else
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::DoingInitialLogin;
	}

	LoginLocalUser(LocalUserInfo, Params.RequestedPrivilege, Params.OnlineContext, FOnLocalUserLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginForUserInitialize, Params));

	return true;
}

void UCommonUserSubsystem::ListenForLoginKeyInput(TArray<FKey> AnyUserKeys, TArray<FKey> NewUserKeys, FCommonUserInitializeParams Params)
{
	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
	if (ensure(ViewportClient))
	{
		const bool bIsMapped = LoginKeysForAnyUser.Num() > 0 || LoginKeysForNewUser.Num() > 0;
		const bool bShouldBeMapped = AnyUserKeys.Num() > 0 || NewUserKeys.Num() > 0;

		if (bIsMapped && !bShouldBeMapped)
		{
			// Set it back to wrapped handler
			ViewportClient->OnOverrideInputKey() = WrappedInputKeyHandler;
			WrappedInputKeyHandler.Unbind();
		}
		else if (!bIsMapped && bShouldBeMapped)
		{
			// Set up a wrapped handler
			WrappedInputKeyHandler = ViewportClient->OnOverrideInputKey();
			ViewportClient->OnOverrideInputKey().BindUObject(this, &UCommonUserSubsystem::OverrideInputKeyForLogin);
		}

		LoginKeysForAnyUser = AnyUserKeys;
		LoginKeysForNewUser = NewUserKeys;

		if (bShouldBeMapped)
		{
			ParamsForLoginKey = Params;
		}
		else
		{
			ParamsForLoginKey = FCommonUserInitializeParams();
		}
	}
}

bool UCommonUserSubsystem::CancelUserInitialization(int32 LocalPlayerIndex)
{
	UCommonUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(LocalPlayerIndex));
	if (!LocalUserInfo)
	{
		return false;
	}

	if (LocalUserInfo->InitializationState != ECommonUserInitializationState::DoingInitialLogin && LocalUserInfo->InitializationState != ECommonUserInitializationState::DoingNetworkLogin)
	{
		return false;
	}

	// Remove from login queue
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		if (Request->UserInfo.IsValid() && Request->UserInfo->LocalPlayerIndex == LocalPlayerIndex)
		{
			ActiveLoginRequests.Remove(Request);
		}
	}

	// Set state with best guess
	if (LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingNetworkLogin)
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::LoggedInLocalOnly;
	}
	else
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::FailedtoLogin;
	}

	return true;
}

void UCommonUserSubsystem::ResetUserState()
{
	// Manually purge existing info objects
	for (TPair<int32, UCommonUserInfo*> Pair : LocalUserInfos)
	{
		if (Pair.Value)
		{
			Pair.Value->MarkAsGarbage();
		}
	}

	LocalUserInfos.Reset();

	// Cancel in-progress logins
	ActiveLoginRequests.Reset();

	// Create player info for id 0
	UCommonUserInfo* FirstUser = CreateLocalUserInfo(0);

	if (GEngine->IsControllerIdUsingPlatformUserId())
	{
		FirstUser->PlatformUserIndex = 0;
		FirstUser->PrimaryControllerId = 0;
	}
	else
	{
		ensureMsgf(0, TEXT("No default platform user id set up!"));
	}

	// TODO: Schedule a refresh of player 0 for next frame?
	RefreshLocalUserInfo(FirstUser);

}

bool UCommonUserSubsystem::OverrideInputKeyForLogin(FInputKeyEventArgs& EventArgs)
{
	int32 NextLocalPlayerIndex = INDEX_NONE;

	const UCommonUserInfo* MappedUser = GetUserInfoForControllerId(EventArgs.ControllerId);
	if (EventArgs.Event == IE_Pressed)
	{
		if (MappedUser == nullptr || MappedUser->InitializationState == ECommonUserInitializationState::Unknown || MappedUser->InitializationState == ECommonUserInitializationState::FailedtoLogin)
		{
			if (MappedUser)
			{
				NextLocalPlayerIndex = MappedUser->LocalPlayerIndex;
			}
			else
			{
				// Find next player
				for (int32 i = 0; i < MaxNumberOfLocalPlayers; i++)
				{
					if (GetLocalPlayerInitializationState(i) == ECommonUserInitializationState::Unknown)
					{
						NextLocalPlayerIndex = i;
						break;
					}
				}
			}

			if (NextLocalPlayerIndex != INDEX_NONE)
			{
				if (LoginKeysForAnyUser.Contains(EventArgs.Key))
				{
					// Press start screen
					FCommonUserInitializeParams NewParams = ParamsForLoginKey;
					NewParams.LocalPlayerIndex = NextLocalPlayerIndex;
					NewParams.ControllerId = EventArgs.ControllerId;

					return TryToInitializeUser(NewParams);
				}

				// See if this controller id is mapped
				MappedUser = GetUserInfoForControllerId(EventArgs.ControllerId);

				if (!MappedUser || MappedUser->LocalPlayerIndex == INDEX_NONE)
				{
					if (LoginKeysForNewUser.Contains(EventArgs.Key))
					{
						// Local multiplayer
						FCommonUserInitializeParams NewParams = ParamsForLoginKey;
						NewParams.LocalPlayerIndex = NextLocalPlayerIndex;
						NewParams.ControllerId = EventArgs.ControllerId;

						return TryToInitializeUser(NewParams);
					}
				}
			}
		}
	}

	if ((LoginKeysForAnyUser.Contains(EventArgs.Key) || LoginKeysForNewUser.Contains(EventArgs.Key))
		&& (MappedUser == nullptr || MappedUser->InitializationState != ECommonUserInitializationState::LoggedInOnline))
	{
		return true;
	}

	if (WrappedInputKeyHandler.IsBound())
	{
		return WrappedInputKeyHandler.Execute(EventArgs);
	}

	return false;
}

static inline FText GetErrorText(const FOnlineErrorType& InOnlineError)
{
#if COMMONUSER_OSSV1
	return InOnlineError.GetErrorMessage();
#else
	return InOnlineError.GetText();
#endif
}

void UCommonUserSubsystem::HandleLoginForUserInitialize(const UCommonUserInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, const TOptional<FOnlineErrorType>& InError, ECommonUserOnlineContext Context, FCommonUserInitializeParams Params)
{
	UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);
	FTimerManager& TimerManager = GameInstance->GetTimerManager();
	TOptional<FOnlineErrorType> Error = InError; // Copy so we can reset on handled errors

	UCommonUserInfo* LocalUserInfo = ModifyInfo(UserInfo);
	UCommonUserInfo* FirstUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(0));

	if (!ensure(LocalUserInfo && FirstUserInfo))
	{
		return;
	}

	// Check the hard platform/service ids
	RefreshLocalUserInfo(LocalUserInfo);

	FUniqueNetIdRepl FirstPlayerId = FirstUserInfo->GetNetId(ECommonUserOnlineContext::PlatformOrDefault);

	// Check to see if we should make a guest after a login failure. Some platforms return success but reuse the first player's id, count this as a failure
	if (LocalUserInfo != FirstUserInfo && LocalUserInfo->bCanBeGuest && (NewStatus == ELoginStatusType::NotLoggedIn || NetId == FirstPlayerId))
	{
#if COMMONUSER_OSSV1
		NetId = (FUniqueNetIdRef)FUniqueNetIdString::Create(FString::Printf(TEXT("GuestPlayer%d"), LocalUserInfo->LocalPlayerIndex), NULL_SUBSYSTEM);
#else
		// TODO:  OSSv2 FUniqueNetIdRepl wrapping FOnlineAccountIdHandle is in progress
		// TODO:  OSSv2 - How to handle guest accounts?
#endif
		LocalUserInfo->bIsGuest = true;
		NewStatus = ELoginStatusType::UsingLocalProfile;
		Error.Reset();
		UE_LOG(LogCommonUser, Log, TEXT("HandleLoginForUserInitialize created guest id %s for local player %d"), *NetId.ToString(), LocalUserInfo->LocalPlayerIndex);
	}
	else
	{
		LocalUserInfo->bIsGuest = false;
	}

	ensure(LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingInitialLogin || LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingNetworkLogin);

	if (Error.IsSet())
	{
		FText ErrorText = GetErrorText(Error.GetValue());
		TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCommonUserSubsystem::HandleUserInitializeFailed, Params, ErrorText));
		return;
	}

	if (Context == ECommonUserOnlineContext::Game)
	{
		LocalUserInfo->UpdateCachedNetId(NetId, ECommonUserOnlineContext::Game);
	}
		
	ULocalPlayer* CurrentPlayer = GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex);
	if (!CurrentPlayer && Params.bCanCreateNewLocalPlayer)
	{
		FString ErrorString;
		CurrentPlayer = GameInstance->CreateLocalPlayer(LocalUserInfo->PrimaryControllerId, ErrorString, true);

		if (!CurrentPlayer)
		{
			TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCommonUserSubsystem::HandleUserInitializeFailed, Params, FText::AsCultureInvariant(ErrorString)));
			return;
		}
		ensure(GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex) == CurrentPlayer);
	}

	// Updates controller and net id if needed
	SetLocalPlayerUserInfo(CurrentPlayer, LocalUserInfo);

	// Set a delayed callback
	TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCommonUserSubsystem::HandleUserInitializeSucceeded, Params));
}

void UCommonUserSubsystem::HandleUserInitializeFailed(FCommonUserInitializeParams Params, FText Error)
{
	UCommonUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));

	if (!LocalUserInfo)
	{
		// The user info was reset since this was scheduled
		return;
	}

	UE_LOG(LogCommonUser, Warning, TEXT("TryToInitializeUser %d failed with error %s"), LocalUserInfo->LocalPlayerIndex, *Error.ToString());

	// If state is wrong, abort as we might have gotten canceled
	if (!ensure(LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingInitialLogin || LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingNetworkLogin))
	{
		return;
	}

	// If initial login failed or we ended up totally logged out, set to complete failure
	ELoginStatusType NewStatus = GetLocalUserLoginStatus(Params.LocalPlayerIndex, Params.OnlineContext);
	if (NewStatus == ELoginStatusType::NotLoggedIn || LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingInitialLogin)
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::FailedtoLogin;
	}
	else
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::LoggedInLocalOnly;
	}

	FText TitleText = NSLOCTEXT("CommonUser", "LoginFailedTitle", "Login Failure");

	if (!Params.bSuppressLoginErrors)
	{
		SendSystemMessage(FCommonUserTags::SystemMessage_Error_InitializeLocalPlayerFailed, TitleText, Error);
	}
	
	// Call callbacks
	Params.OnUserInitializeComplete.ExecuteIfBound(LocalUserInfo, false, Error, Params.RequestedPrivilege, Params.OnlineContext);
	OnUserInitializeComplete.Broadcast(LocalUserInfo, false, Error, Params.RequestedPrivilege, Params.OnlineContext);
}

void UCommonUserSubsystem::HandleUserInitializeSucceeded(FCommonUserInitializeParams Params)
{
	UCommonUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));

	if (!LocalUserInfo)
	{
		// The user info was reset since this was scheduled
		return;
	}

	// If state is wrong, abort as we might have gotten cancelled
	if (!ensure(LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingInitialLogin || LocalUserInfo->InitializationState == ECommonUserInitializationState::DoingNetworkLogin))
	{
		return;
	}

	// Fix up state
	if (Params.RequestedPrivilege == ECommonUserPrivilege::CanPlayOnline)
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::LoggedInOnline;
	}
	else
	{
		LocalUserInfo->InitializationState = ECommonUserInitializationState::LoggedInLocalOnly;
	}

	ensure(LocalUserInfo->GetPrivilegeAvailability(Params.RequestedPrivilege) == ECommonUserAvailability::NowAvailable);

	// Call callbacks
	Params.OnUserInitializeComplete.ExecuteIfBound(LocalUserInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
	OnUserInitializeComplete.Broadcast(LocalUserInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
}

bool UCommonUserSubsystem::LoginLocalUser(const UCommonUserInfo* UserInfo, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext Context, FOnLocalUserLoginCompleteDelegate OnComplete)
{
	UCommonUserInfo* LocalUserInfo = ModifyInfo(UserInfo);
	if (!ensure(UserInfo))
	{
		return false;
	}

	TSharedRef<FUserLoginRequest> NewRequest = MakeShared<FUserLoginRequest>(LocalUserInfo, RequestedPrivilege, Context, MoveTemp(OnComplete));
	ActiveLoginRequests.Add(NewRequest);

	// This will execute callback or start login process
	ProcessLoginRequest(NewRequest);

	return true;
}

void UCommonUserSubsystem::ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request)
{
	// First, see if we've fully logged in
	UCommonUserInfo* UserInfo = Request->UserInfo.Get();

	if (!UserInfo)
	{
		// User is gone, just delete this request
		ActiveLoginRequests.Remove(Request);

		return;
	}

	const int32 PlatformUserIndex = UserInfo->PlatformUserIndex;

	// If the platform user id is invalid because this is a guest, skip right to failure
	if (!IsValidPlatformUserIndex(PlatformUserIndex))
	{
#if COMMONUSER_OSSV1
		Request->Error = FOnlineError(NSLOCTEXT("CommonUser", "InvalidPlatformUser", "Invalid Platform User"));
#else
		Request->Error = UE::Online::Errors::InvalidUser();
#endif
		// Remove from active array
		ActiveLoginRequests.Remove(Request);

		// Execute delegate if bound
		Request->Delegate.ExecuteIfBound(UserInfo, ELoginStatusType::NotLoggedIn, FUniqueNetIdRepl(), Request->Error, Request->DesiredContext);

		return;
	}

	// Figure out what context to process first
	if (Request->CurrentContext == ECommonUserOnlineContext::Invalid)
	{
		// First start with platform context if this is a game login
		if (Request->DesiredContext == ECommonUserOnlineContext::Game)
		{
			Request->CurrentContext = ResolveOnlineContext(ECommonUserOnlineContext::PlatformOrDefault);
		}
		else
		{
			Request->CurrentContext = ResolveOnlineContext(Request->DesiredContext);
		}
	}

	ELoginStatusType CurrentStatus = GetLocalUserLoginStatus(PlatformUserIndex, Request->CurrentContext);
	FUniqueNetIdRepl CurrentId = GetLocalUserNetId(PlatformUserIndex, Request->CurrentContext);
	FOnlineContextCache* System = GetContextCache(Request->CurrentContext);

	if (!ensure(System))
	{
		return;
	}

	// Starting a new request
	if (Request->OverallLoginState == ECommonUserAsyncTaskState::NotStarted)
	{
		Request->OverallLoginState = ECommonUserAsyncTaskState::InProgress;
	}

	bool bHasRequiredStatus = (CurrentStatus == ELoginStatusType::LoggedIn);
	if (Request->DesiredPrivilege == ECommonUserPrivilege::CanPlay)
	{
		// If this is not an online required login, allow local profile to count as fully logged in
		bHasRequiredStatus |= (CurrentStatus == ELoginStatusType::UsingLocalProfile);
	}

	// Check for overall success
	if (CurrentStatus != ELoginStatusType::NotLoggedIn && CurrentId.IsValid())
	{
		// Stall if we're waiting for the login UI to close
		if (Request->LoginUIState == ECommonUserAsyncTaskState::InProgress)
		{
			return;
		}

		Request->OverallLoginState = ECommonUserAsyncTaskState::Done;
	}
	else
	{
		// Try using platform auth to login
		if (Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::NotStarted)
		{
			Request->TransferPlatformAuthState = ECommonUserAsyncTaskState::InProgress;

			if (TransferPlatformAuth(System, Request, PlatformUserIndex))
			{
				return;
			}
			// We didn't start a login attempt, so set failure
			Request->TransferPlatformAuthState = ECommonUserAsyncTaskState::Failed;
		}

		// Next check AutoLogin
		if (Request->AutoLoginState == ECommonUserAsyncTaskState::NotStarted)
		{
			if (Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::Done || Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::Failed)
			{
				Request->AutoLoginState = ECommonUserAsyncTaskState::InProgress;

				// Try an auto login with default credentials, this will work on many platforms
				if (AutoLogin(System, Request, PlatformUserIndex))
				{
					return;
				}
				// We didn't start an autologin attempt, so set failure
				Request->AutoLoginState = ECommonUserAsyncTaskState::Failed;
			}
		}

		// Next check login UI
		if (Request->LoginUIState == ECommonUserAsyncTaskState::NotStarted)
		{
			if ((Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::Done || Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::Failed)
				&& (Request->AutoLoginState == ECommonUserAsyncTaskState::Done || Request->AutoLoginState == ECommonUserAsyncTaskState::Failed))
			{
				Request->LoginUIState = ECommonUserAsyncTaskState::InProgress;

				if (ShowLoginUI(System, Request, PlatformUserIndex))
				{
					return;
				}
				// We didn't show a UI, so set failure
				Request->LoginUIState = ECommonUserAsyncTaskState::Failed;
			}
		}
	}

	// Check for overall failure
	if (Request->LoginUIState == ECommonUserAsyncTaskState::Failed &&
		Request->AutoLoginState == ECommonUserAsyncTaskState::Failed &&
		Request->TransferPlatformAuthState == ECommonUserAsyncTaskState::Failed)
	{
		Request->OverallLoginState = ECommonUserAsyncTaskState::Failed;
	}

	if (Request->OverallLoginState == ECommonUserAsyncTaskState::Done)
	{
		// Do the permissions check if needed
		if (Request->PrivilegeCheckState == ECommonUserAsyncTaskState::NotStarted)
		{
			Request->PrivilegeCheckState = ECommonUserAsyncTaskState::InProgress;

			ECommonUserPrivilegeResult CachedResult = UserInfo->GetCachedPrivilegeResult(Request->DesiredPrivilege, Request->CurrentContext);
			if (CachedResult == ECommonUserPrivilegeResult::Available)
			{
				// Use cached success value
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Done;
			}
			else
			{
				if (QueryUserPrivilege(System, Request, PlatformUserIndex))
				{
					return;
				}
				else
				{
#if !COMMONUSER_OSSV1
					// Temp while OSSv2 gets privileges implemented
					CachedResult = ECommonUserPrivilegeResult::Available;
					Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Done;
#endif
				}
			}
		}

		if (Request->PrivilegeCheckState == ECommonUserAsyncTaskState::Failed)
		{
			// Count a privilege failure as a login failure
			Request->OverallLoginState = ECommonUserAsyncTaskState::Failed;
		}
		else if (Request->PrivilegeCheckState == ECommonUserAsyncTaskState::Done)
		{
			// If platform context done but still need to do service context, do that next
			ECommonUserOnlineContext ResolvedDesiredContext = ResolveOnlineContext(Request->DesiredContext);

			if (Request->OverallLoginState == ECommonUserAsyncTaskState::Done && Request->CurrentContext != ResolvedDesiredContext)
			{
				Request->CurrentContext = ResolvedDesiredContext;
				Request->OverallLoginState = ECommonUserAsyncTaskState::NotStarted;
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::NotStarted;

				// Reprocess and immediately return
				ProcessLoginRequest(Request);
				return;
			}
		}
	}

	if (Request->PrivilegeCheckState == ECommonUserAsyncTaskState::InProgress)
	{
		// Stall to wait for it to finish
		return;
	}

	// If done, remove and do callback
	if (Request->OverallLoginState == ECommonUserAsyncTaskState::Done || Request->OverallLoginState == ECommonUserAsyncTaskState::Failed)
	{
		// Remove from active array
		ActiveLoginRequests.Remove(Request);

		// Execute delegate if bound
		Request->Delegate.ExecuteIfBound(UserInfo, CurrentStatus, CurrentId, Request->Error, Request->DesiredContext);
	}
}

#if COMMONUSER_OSSV1
void UCommonUserSubsystem::HandleUserLoginCompleted(int32 PlatformUserIndex, bool bWasSuccessful, const FUniqueNetId& NetId, const FString& ErrorString, ECommonUserOnlineContext Context)
{
	ELoginStatusType NewStatus = GetLocalUserLoginStatus(PlatformUserIndex, Context);
	FUniqueNetIdRepl NewId = FUniqueNetIdRepl(NetId);
	UE_LOG(LogCommonUser, Log, TEXT("Player login Completed - System:%s, UserIdx:%d, Successful:%d, NewStatus:%s, NewId:%s, ErrorIfAny:%s"),
		*GetOnlineSubsystemName(Context).ToString(),
		PlatformUserIndex,
		(int32)bWasSuccessful,
		ELoginStatus::ToString(NewStatus),
		*NetId.ToString(),
		*ErrorString);

	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UCommonUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		if (UserInfo->PlatformUserIndex == PlatformUserIndex && Request->CurrentContext == Context)
		{
			// On some platforms this gets called from the login UI with a failure
			if (Request->AutoLoginState == ECommonUserAsyncTaskState::InProgress)
			{
				Request->AutoLoginState = bWasSuccessful ? ECommonUserAsyncTaskState::Done : ECommonUserAsyncTaskState::Failed;
			}

			if (!bWasSuccessful)
			{
				Request->Error = FOnlineError(FText::FromString(ErrorString));
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UCommonUserSubsystem::HandleOnLoginUIClosed(TSharedPtr<const FUniqueNetId> LoggedInNetId, const int PlatformUserIndex, const FOnlineError& Error, ECommonUserOnlineContext Context)
{
	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UCommonUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		if (UserInfo->PlatformUserIndex == PlatformUserIndex && Request->CurrentContext == Context && Request->LoginUIState == ECommonUserAsyncTaskState::InProgress)
		{
			if (LoggedInNetId.IsValid() && LoggedInNetId->IsValid() && Error.WasSuccessful())
			{
				Request->LoginUIState = ECommonUserAsyncTaskState::Done;
				Request->Error.Reset();
			}
			else
			{
				Request->LoginUIState = ECommonUserAsyncTaskState::Failed;
				Request->Error = Error;
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UCommonUserSubsystem::HandleCheckPrivilegesComplete(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults, ECommonUserPrivilege UserPrivilege, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, ECommonUserOnlineContext Context)
{
	// Only handle if user still exists
	UCommonUserInfo* UserInfo = CommonUserInfo.Get();

	if (!UserInfo)
	{
		return;
	}

	ECommonUserPrivilegeResult UserResult = ConvertOSSPrivilegeResult(Privilege, PrivilegeResults);

	// Update the user cached value
	UpdateUserPrivilegeResult(UserInfo, UserPrivilege, UserResult, Context);

	FOnlineContextCache* ContextCache = GetContextCache(Context);
	check(ContextCache);

	// If this returns disconnected, update the connection status
	if (UserResult == ECommonUserPrivilegeResult::NetworkConnectionUnavailable)
	{
		ContextCache->CurrentConnectionStatus = EOnlineServerConnectionStatus::NoNetworkConnection;
	}
	else if (UserResult == ECommonUserPrivilegeResult::Available && UserPrivilege == ECommonUserPrivilege::CanPlayOnline)
	{
		if (ContextCache->CurrentConnectionStatus == EOnlineServerConnectionStatus::NoNetworkConnection)
		{
			ContextCache->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;
		}
	}
		
	// See if a login request is waiting on this
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		if (Request->UserInfo.Get() == UserInfo && Request->CurrentContext == Context && Request->DesiredPrivilege == UserPrivilege && Request->PrivilegeCheckState == ECommonUserAsyncTaskState::InProgress)
		{
			if (UserResult == ECommonUserPrivilegeResult::Available)
			{
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Done;
			}
			else
			{
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Failed;

				// Forms strings in english like "(The user is not allowed) to (play the game)"
				Request->Error = FOnlineError(FText::Format(NSLOCTEXT("CommonUser", "PrivilegeFailureFormat", "{0} to {1}"), GetPrivilegeResultDescription(UserResult), GetPrivilegeDescription(UserPrivilege)));
			}

			ProcessLoginRequest(Request);
		}
	}
}
#else

void UCommonUserSubsystem::HandleUserLoginCompletedV2(const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result, int32 PlatformUserIndex, ECommonUserOnlineContext Context)
{
	const bool bWasSuccessful = Result.IsOk();
	FOnlineAccountIdHandle NewId;
	if (bWasSuccessful)
	{
		NewId = Result.GetOkValue().AccountInfo->UserId;
	}
	ELoginStatusType NewStatus = GetLocalUserLoginStatus(PlatformUserIndex, Context);
	UE_LOG(LogCommonUser, Log, TEXT("Player login Completed - System:%d, UserIdx:%d, Successful:%d, NewId:%s, ErrorIfAny:%s"),
		(int32)Context,
		PlatformUserIndex,
		(int32)Result.IsOk(),
		*ToLogString(NewId),
		Result.IsError() ? *Result.GetErrorValue().GetLogString() : TEXT(""));

	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UCommonUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		if (UserInfo->PlatformUserIndex == PlatformUserIndex && Request->CurrentContext == Context)
		{
			// On some platforms this gets called from the login UI with a failure
			if (Request->AutoLoginState == ECommonUserAsyncTaskState::InProgress)
			{
				Request->AutoLoginState = bWasSuccessful ? ECommonUserAsyncTaskState::Done : ECommonUserAsyncTaskState::Failed;
			}

			if (bWasSuccessful)
			{
				Request->Error.Reset();
			}
			else
			{
				Request->Error = Result.GetErrorValue();
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UCommonUserSubsystem::HandleOnLoginUIClosedV2(const UE::Online::TOnlineResult<UE::Online::FExternalUIShowLoginUI>& Result, int32 PlatformUserIndex, ECommonUserOnlineContext Context)
{
	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UCommonUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		if (UserInfo->PlatformUserIndex == PlatformUserIndex && Request->CurrentContext == Context && Request->LoginUIState == ECommonUserAsyncTaskState::InProgress)
		{
			if (Result.IsOk())
			{
				Request->LoginUIState = ECommonUserAsyncTaskState::Done;
				Request->Error.Reset();
			}
			else
			{
				Request->LoginUIState = ECommonUserAsyncTaskState::Failed;
				Request->Error = Result.GetErrorValue();
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UCommonUserSubsystem::HandleCheckPrivilegesComplete(const UE::Online::TOnlineResult<UE::Online::FQueryUserPrivilege>& Result, TWeakObjectPtr<UCommonUserInfo> CommonUserInfo, EUserPrivileges DesiredPrivilege, ECommonUserOnlineContext Context)
{
	// Only handle if user still exists
	UCommonUserInfo* UserInfo = CommonUserInfo.Get();
	if (!UserInfo)
	{
		return;
	}

	ECommonUserPrivilege UserPrivilege = ConvertOnlineServicesPrivilege(DesiredPrivilege);
	ECommonUserPrivilegeResult UserResult = ECommonUserPrivilegeResult::PlatformFailure;
	if (const FQueryUserPrivilege::Result* OkResult = Result.TryGetOkValue())
	{
		UserResult = ConvertOnlineServicesPrivilegeResult(DesiredPrivilege, OkResult->PrivilegeResult);
	}
	else
	{
		UE_LOG(LogCommonUser, Warning, TEXT("QueryUserPrivilege failed: %s"), *Result.GetErrorValue().GetLogString());
	}

	// Update the user cached value
	UserInfo->UpdateCachedPrivilegeResult(UserPrivilege, UserResult, Context);

	// See if a login request is waiting on this
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		if (Request->UserInfo.Get() == UserInfo && Request->CurrentContext == Context && Request->DesiredPrivilege == UserPrivilege && Request->PrivilegeCheckState == ECommonUserAsyncTaskState::InProgress)
		{
			if (UserResult == ECommonUserPrivilegeResult::Available)
			{
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Done;
			}
			else
			{
				Request->PrivilegeCheckState = ECommonUserAsyncTaskState::Failed;
				Request->Error = Result.IsError() ? Result.GetErrorValue() : UE::Online::Errors::Unknown();
			}

			ProcessLoginRequest(Request);
		}
	}
}
#endif // COMMONUSER_OSSV1

void UCommonUserSubsystem::RefreshLocalUserInfo(UCommonUserInfo* UserInfo)
{
	if (ensure(UserInfo))
	{
		// Always update default
		UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUserIndex, ECommonUserOnlineContext::Default), ECommonUserOnlineContext::Default);

		if (HasSeparatePlatformContext())
		{
			// Also update platform
			UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUserIndex, ECommonUserOnlineContext::Platform), ECommonUserOnlineContext::Platform);
		}
	}
}

void UCommonUserSubsystem::HandleChangedAvailability(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability)
{
	ECommonUserAvailability NewAvailability = UserInfo->GetPrivilegeAvailability(Privilege);

	if (OldAvailability != NewAvailability)
	{
		OnUserPrivilegeChanged.Broadcast(UserInfo, Privilege, OldAvailability, NewAvailability);
	}
}

void UCommonUserSubsystem::UpdateUserPrivilegeResult(UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserPrivilegeResult Result, ECommonUserOnlineContext Context)
{
	check(UserInfo);
	
	ECommonUserAvailability OldAvailability = UserInfo->GetPrivilegeAvailability(Privilege);

	UserInfo->UpdateCachedPrivilegeResult(Privilege, Result, Context);

	HandleChangedAvailability(UserInfo, Privilege, OldAvailability);
}

#if COMMONUSER_OSSV1
ECommonUserPrivilege UCommonUserSubsystem::ConvertOSSPrivilege(EUserPrivileges::Type Privilege) const
{
	switch (Privilege)
	{
	case EUserPrivileges::CanPlay:
		return ECommonUserPrivilege::CanPlay;
	case EUserPrivileges::CanPlayOnline:
		return ECommonUserPrivilege::CanPlayOnline;
	case EUserPrivileges::CanCommunicateOnline:
		return ECommonUserPrivilege::CanCommunicateViaTextOnline; // No good thing to do here, just mapping to text.
	case EUserPrivileges::CanUseUserGeneratedContent:
		return ECommonUserPrivilege::CanUseUserGeneratedContent;
	case EUserPrivileges::CanUserCrossPlay:
		return ECommonUserPrivilege::CanUseCrossPlay;
	default:
		return ECommonUserPrivilege::Invalid_Count;
	}
}

EUserPrivileges::Type UCommonUserSubsystem::ConvertOSSPrivilege(ECommonUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case ECommonUserPrivilege::CanPlay:
		return EUserPrivileges::CanPlay;
	case ECommonUserPrivilege::CanPlayOnline:
		return EUserPrivileges::CanPlayOnline;
	case ECommonUserPrivilege::CanCommunicateViaTextOnline:
	case ECommonUserPrivilege::CanCommunicateViaVoiceOnline:
		return EUserPrivileges::CanCommunicateOnline;
	case ECommonUserPrivilege::CanUseUserGeneratedContent:
		return EUserPrivileges::CanUseUserGeneratedContent;
	case ECommonUserPrivilege::CanUseCrossPlay:
		return EUserPrivileges::CanUserCrossPlay;
	default:
		// No failure type, return CanPlay
		return EUserPrivileges::CanPlay;
	}
}

ECommonUserPrivilegeResult UCommonUserSubsystem::ConvertOSSPrivilegeResult(EUserPrivileges::Type Privilege, uint32 Results) const
{
	// The V1 results enum is a bitfield where each platform behaves a bit differently
	if (Results == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		return ECommonUserPrivilegeResult::Available;
	}
	if ((Results & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotFound) || (Results & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotLoggedIn))
	{
		return ECommonUserPrivilegeResult::UserNotLoggedIn;
	}
	if ((Results & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable) || (Results & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate))
	{
		return ECommonUserPrivilegeResult::VersionOutdated;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure)
	{
		return ECommonUserPrivilegeResult::AgeRestricted;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::AccountTypeFailure)
	{
		return ECommonUserPrivilegeResult::AccountTypeRestricted;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::NetworkConnectionUnavailable)
	{
		return ECommonUserPrivilegeResult::NetworkConnectionUnavailable;
	}

	// Bucket other account failures together
	uint32 AccountUseFailures = (uint32)IOnlineIdentity::EPrivilegeResults::OnlinePlayRestricted 
		| (uint32)IOnlineIdentity::EPrivilegeResults::UGCRestriction 
		| (uint32)IOnlineIdentity::EPrivilegeResults::ChatRestriction;

	if (Results & AccountUseFailures)
	{
		return ECommonUserPrivilegeResult::AccountUseRestricted;
	}

	// If you can't play at all, this is a license failure
	if (Privilege == EUserPrivileges::CanPlay)
	{
		return ECommonUserPrivilegeResult::LicenseInvalid;
	}

	// Unknown reason
	return ECommonUserPrivilegeResult::PlatformFailure;
}
#else
ECommonUserPrivilege UCommonUserSubsystem::ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const
{
	switch (Privilege)
	{
	case EUserPrivileges::CanPlay:
		return ECommonUserPrivilege::CanPlay;
	case EUserPrivileges::CanPlayOnline:
		return ECommonUserPrivilege::CanPlayOnline;
	case EUserPrivileges::CanCommunicateViaTextOnline:
		return ECommonUserPrivilege::CanCommunicateViaTextOnline;
	case EUserPrivileges::CanCommunicateViaVoiceOnline:
		return ECommonUserPrivilege::CanCommunicateViaVoiceOnline;
	case EUserPrivileges::CanUseUserGeneratedContent:
		return ECommonUserPrivilege::CanUseUserGeneratedContent;
	case EUserPrivileges::CanCrossPlay:
		return ECommonUserPrivilege::CanUseCrossPlay;
	default:
		return ECommonUserPrivilege::Invalid_Count;
	}
}

EUserPrivileges UCommonUserSubsystem::ConvertOnlineServicesPrivilege(ECommonUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case ECommonUserPrivilege::CanPlay:
		return EUserPrivileges::CanPlay;
	case ECommonUserPrivilege::CanPlayOnline:
		return EUserPrivileges::CanPlayOnline;
	case ECommonUserPrivilege::CanCommunicateViaTextOnline:
		return EUserPrivileges::CanCommunicateViaTextOnline;
	case ECommonUserPrivilege::CanCommunicateViaVoiceOnline:
		return EUserPrivileges::CanCommunicateViaVoiceOnline;
	case ECommonUserPrivilege::CanUseUserGeneratedContent:
		return EUserPrivileges::CanUseUserGeneratedContent;
	case ECommonUserPrivilege::CanUseCrossPlay:
		return EUserPrivileges::CanCrossPlay;
	default:
		// No failure type, return CanPlay
		return EUserPrivileges::CanPlay;
	}
}

ECommonUserPrivilegeResult UCommonUserSubsystem::ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const
{
	// The V1 results enum is a bitfield where each platform behaves a bit differently
	if (Results == EPrivilegeResults::NoFailures)
	{
		return ECommonUserPrivilegeResult::Available;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::UserNotFound | EPrivilegeResults::UserNotLoggedIn))
	{
		return ECommonUserPrivilegeResult::UserNotLoggedIn;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::RequiredPatchAvailable | EPrivilegeResults::RequiredSystemUpdate))
	{
		return ECommonUserPrivilegeResult::VersionOutdated;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AgeRestrictionFailure))
	{
		return ECommonUserPrivilegeResult::AgeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AccountTypeFailure))
	{
		return ECommonUserPrivilegeResult::AccountTypeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::NetworkConnectionUnavailable))
	{
		return ECommonUserPrivilegeResult::NetworkConnectionUnavailable;
	}

	// Bucket other account failures together
	const EPrivilegeResults AccountUseFailures = EPrivilegeResults::OnlinePlayRestricted
		| EPrivilegeResults::UGCRestriction
		| EPrivilegeResults::ChatRestriction;

	if (EnumHasAnyFlags(Results, AccountUseFailures))
	{
		return ECommonUserPrivilegeResult::AccountUseRestricted;
	}

	// If you can't play at all, this is a license failure
	if (Privilege == EUserPrivileges::CanPlay)
	{
		return ECommonUserPrivilegeResult::LicenseInvalid;
	}

	// Unknown reason
	return ECommonUserPrivilegeResult::PlatformFailure;
}
#endif // COMMONUSER_OSSV1

FString UCommonUserSubsystem::PlatformUserIdToString(FPlatformUserId UserId)
{
	if (UserId == PLATFORMUSERID_NONE)
	{
		return TEXT("None");
	}
	else
	{
		return FString::Printf(TEXT("%d"), UserId.GetInternalId());
	}
}

FString UCommonUserSubsystem::ECommonUserOnlineContextToString(ECommonUserOnlineContext Context)
{
	switch (Context)
	{
	case ECommonUserOnlineContext::Game:
		return TEXT("Game");
	case ECommonUserOnlineContext::Default:
		return TEXT("Default");
	case ECommonUserOnlineContext::Service:
		return TEXT("Service");
	case ECommonUserOnlineContext::ServiceOrDefault:
		return TEXT("Service/Default");
	case ECommonUserOnlineContext::Platform:
		return TEXT("Platform");
	case ECommonUserOnlineContext::PlatformOrDefault:
		return TEXT("Platform/Default");
	default:
		return TEXT("Invalid");
	}
}

FText UCommonUserSubsystem::GetPrivilegeDescription(ECommonUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case ECommonUserPrivilege::CanPlay:
		return NSLOCTEXT("CommonUser", "PrivilegeCanPlay", "play the game");
	case ECommonUserPrivilege::CanPlayOnline:
		return NSLOCTEXT("CommonUser", "PrivilegeCanPlayOnline", "play online");
	case ECommonUserPrivilege::CanCommunicateViaTextOnline:
		return NSLOCTEXT("CommonUser", "PrivilegeCanCommunicateViaTextOnline", "communicate with text");
	case ECommonUserPrivilege::CanCommunicateViaVoiceOnline:
		return NSLOCTEXT("CommonUser", "PrivilegeCanCommunicateViaVoiceOnline", "communicate with voice");
	case ECommonUserPrivilege::CanUseUserGeneratedContent:
		return NSLOCTEXT("CommonUser", "PrivilegeCanUseUserGeneratedContent", "access user content");
	case ECommonUserPrivilege::CanUseCrossPlay:
		return NSLOCTEXT("CommonUser", "PrivilegeCanUseCrossPlay", "play with other platforms");
	default:
		return NSLOCTEXT("CommonUser", "PrivilegeInvalid", "");
	}
}

FText UCommonUserSubsystem::GetPrivilegeResultDescription(ECommonUserPrivilegeResult Result) const
{
	// TODO these strings might have cert requirements we need to override per console
	switch (Result)
	{
	case ECommonUserPrivilegeResult::Unknown:
		return NSLOCTEXT("CommonUser", "ResultUnknown", "Unknown if the user is allowed");
	case ECommonUserPrivilegeResult::Available:
		return NSLOCTEXT("CommonUser", "ResultAvailable", "The user is allowed");
	case ECommonUserPrivilegeResult::UserNotLoggedIn:
		return NSLOCTEXT("CommonUser", "ResultUserNotLoggedIn", "The user must login");
	case ECommonUserPrivilegeResult::LicenseInvalid:
		return NSLOCTEXT("CommonUser", "ResultLicenseInvalid", "A valid game license is required");
	case ECommonUserPrivilegeResult::VersionOutdated:
		return NSLOCTEXT("CommonUser", "VersionOutdated", "The game or hardware needs to be updated");
	case ECommonUserPrivilegeResult::NetworkConnectionUnavailable:
		return NSLOCTEXT("CommonUser", "ResultNetworkConnectionUnavailable", "A network connection is required");
	case ECommonUserPrivilegeResult::AgeRestricted:
		return NSLOCTEXT("CommonUser", "ResultAgeRestricted", "This age restricted account is not allowed");
	case ECommonUserPrivilegeResult::AccountTypeRestricted:
		return NSLOCTEXT("CommonUser", "ResultAccountTypeRestricted", "This account type does not have access");
	case ECommonUserPrivilegeResult::AccountUseRestricted:
		return NSLOCTEXT("CommonUser", "ResultAccountUseRestricted", "This account is not allowed");
	case ECommonUserPrivilegeResult::PlatformFailure:
		return NSLOCTEXT("CommonUser", "ResultPlatformFailure", "Not allowed");
	default:
		return NSLOCTEXT("CommonUser", "ResultInvalid", "");

	}
}

const UCommonUserSubsystem::FOnlineContextCache* UCommonUserSubsystem::GetContextCache(ECommonUserOnlineContext Context) const
{
	return const_cast<UCommonUserSubsystem*>(this)->GetContextCache(Context);
}

UCommonUserSubsystem::FOnlineContextCache* UCommonUserSubsystem::GetContextCache(ECommonUserOnlineContext Context)
{
	switch (Context)
	{
	case ECommonUserOnlineContext::Game:
	case ECommonUserOnlineContext::Default:
		return DefaultContextInternal;

	case ECommonUserOnlineContext::Service:
		return ServiceContextInternal;
	case ECommonUserOnlineContext::ServiceOrDefault:
		return ServiceContextInternal ? ServiceContextInternal : DefaultContextInternal;

	case ECommonUserOnlineContext::Platform:
		return PlatformContextInternal;
	case ECommonUserOnlineContext::PlatformOrDefault:
		return PlatformContextInternal ? PlatformContextInternal : DefaultContextInternal;
	}

	return nullptr;
}

ECommonUserOnlineContext UCommonUserSubsystem::ResolveOnlineContext(ECommonUserOnlineContext Context) const
{
	switch (Context)
	{
	case ECommonUserOnlineContext::Game:
	case ECommonUserOnlineContext::Default:
		return ECommonUserOnlineContext::Default;

	case ECommonUserOnlineContext::Service:
		return ServiceContextInternal ? ECommonUserOnlineContext::Service : ECommonUserOnlineContext::Invalid;
	case ECommonUserOnlineContext::ServiceOrDefault:
		return ServiceContextInternal ? ECommonUserOnlineContext::Service : ECommonUserOnlineContext::Default;

	case ECommonUserOnlineContext::Platform:
		return PlatformContextInternal ? ECommonUserOnlineContext::Platform : ECommonUserOnlineContext::Invalid;
	case ECommonUserOnlineContext::PlatformOrDefault:
		return PlatformContextInternal ? ECommonUserOnlineContext::Platform : ECommonUserOnlineContext::Default;
	}

	return  ECommonUserOnlineContext::Invalid;
}

bool UCommonUserSubsystem::HasSeparatePlatformContext() const
{
	ECommonUserOnlineContext ServiceType = ResolveOnlineContext(ECommonUserOnlineContext::ServiceOrDefault);
	ECommonUserOnlineContext PlatformType = ResolveOnlineContext(ECommonUserOnlineContext::PlatformOrDefault);

	if (ServiceType != PlatformType)
	{
		return true;
	}
	return false;
}

void UCommonUserSubsystem::SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const UCommonUserInfo* UserInfo)
{
	if (ensure(LocalPlayer && UserInfo))
	{
		LocalPlayer->SetControllerId(UserInfo->PrimaryControllerId);

		if (!GEngine->IsControllerIdUsingPlatformUserId())
		{
			LocalPlayer->SetPlatformUserId(UserInfo->GetPlatformUserId());
		}

		FUniqueNetIdRepl NetId = UserInfo->GetNetId(ECommonUserOnlineContext::Game);
		LocalPlayer->SetCachedUniqueNetId(NetId);

		// Also update player state if possible
		APlayerController* PlayerController = LocalPlayer->GetPlayerController(nullptr);
		if (PlayerController && PlayerController->PlayerState)
		{
			PlayerController->PlayerState->SetUniqueId(NetId);
		}
	}
}

const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const
{
	UCommonUserInfo* const* Found = LocalUserInfos.Find(LocalPlayerIndex);
	if (Found)
	{
		return *Found;
	}
	return nullptr;
}

const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForPlatformUserIndex(int32 PlatformUserIndex) const
{
	if (!IsValidPlatformUserIndex(PlatformUserIndex))
	{
		return nullptr;
	}

	for (TPair<int32, UCommonUserInfo*> Pair : LocalUserInfos)
	{
		// Don't include guest users in this check
		if (ensure(Pair.Value) && Pair.Value->PlatformUserIndex == PlatformUserIndex && !Pair.Value->bIsGuest)
		{
			return Pair.Value;
		}
	}

	return nullptr;
}

const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const
{
	if (!NetId.IsValid())
	{
		// TODO do we need to handle pre-login case on mobile platforms where netID is invalid?
		return nullptr;
	}

	for (TPair<int32, UCommonUserInfo*> UserPair : LocalUserInfos)
	{
		if (ensure(UserPair.Value))
		{
			for (const TPair<ECommonUserOnlineContext, UCommonUserInfo::FCachedData>& CachedPair : UserPair.Value->CachedDataMap)
			{
				if (NetId == CachedPair.Value.CachedNetId)
				{
					return UserPair.Value;
				}
			}
		}
	}

	return nullptr;
}

const UCommonUserInfo* UCommonUserSubsystem::GetUserInfoForControllerId(int32 ControllerId) const
{
	if (!IsValidControllerId(ControllerId))
	{
		return nullptr;
	}

	for (TPair<int32, UCommonUserInfo*> Pair : LocalUserInfos)
	{
		if (ensure(Pair.Value) && Pair.Value->PrimaryControllerId == ControllerId)
		{
			return Pair.Value;
		}
	}

	return nullptr;
}

bool UCommonUserSubsystem::IsValidControllerId(int32 ControllerId) const
{
	if (ControllerId < 0)
	{
		return false;
	}

	return true;
}

bool UCommonUserSubsystem::IsValidPlatformUserIndex(int32 PlatformUserIndex) const
{
	if (PlatformUserIndex < 0)
	{
		return false;
	}

#if COMMONUSER_OSSV1
	if (PlatformUserIndex >= MAX_LOCAL_PLAYERS || (PlatformUserIndex > 0 && GetTraitTags().HasTag(FCommonUserTags::Platform_Trait_SingleOnlineUser)))
	{
		// Check against OSS count
		return false;
	}
#else
	// TODO:  OSSv2 define MAX_LOCAL_PLAYERS?
#endif
	return true;
}

int32 UCommonUserSubsystem::GetPlatformUserIndexForControllerId(int32 ControllerId) const
{
	if (ControllerId < 0)
	{
		return INDEX_NONE;
	}

	if (GEngine->IsControllerIdUsingPlatformUserId())
	{
		// Backward compatible path, this can return an invalid platform user index above the max players
		return ControllerId;
	}

	ensureMsgf(0, TEXT("Controller Id to Local User Index not implemented!"));
	return ControllerId;
}

void UCommonUserSubsystem::SetTraitTags(const FGameplayTagContainer& InTags)
{
	CachedTraitTags = InTags;
}

bool UCommonUserSubsystem::ShouldWaitForStartInput() const
{
	// By default, don't wait for input if this is a single user platform
	return !HasTraitTag(FCommonUserTags::Platform_Trait_SingleOnlineUser.GetTag());
}

#if COMMONUSER_OSSV1
void UCommonUserSubsystem::HandleIdentityLoginStatusChanged(int32 PlatformUserIndex, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId, ECommonUserOnlineContext Context)
{
	UE_LOG(LogCommonUser, Log, TEXT("Player login status changed - System:%s, UserIdx:%d, OldStatus:%s, NewStatus:%s, NewId:%s"),
		*GetOnlineSubsystemName(Context).ToString(),
		PlatformUserIndex,
		ELoginStatus::ToString(OldStatus),
		ELoginStatus::ToString(NewStatus),
		*NewId.ToString());

	if (NewStatus == ELoginStatus::NotLoggedIn && OldStatus != ELoginStatus::NotLoggedIn)
	{
		LogOutLocalUser(PlatformUserIndex);
	}
}

void UCommonUserSubsystem::HandleControllerPairingChanged(int32 ControllerId, FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser)
{
	UE_LOG(LogCommonUser, Log, TEXT("Player controller pairing changed - UserIdx:%d, PreviousUser:%s, NewUser:%s"),
		ControllerId,
		*ToDebugString(PreviousUser),
		*ToDebugString(NewUser));

	// See if we think this is already bound to an existing player
	check(IsValidControllerId(ControllerId));
	
	UGameInstance* GameInstance = GetGameInstance();

	ULocalPlayer* ControlledLocalPlayer = GameInstance->FindLocalPlayerFromControllerId(ControllerId);
	ULocalPlayer* NewLocalPlayer = GameInstance->FindLocalPlayerFromUniqueNetId(NewUser.User);
	const UCommonUserInfo* NewUserInfo = GetUserInfoForUniqueNetId(FUniqueNetIdRepl(NewUser.User));
	const UCommonUserInfo* PreviousUserInfo = GetUserInfoForUniqueNetId(FUniqueNetIdRepl(NewUser.User));

	if (PreviousUser.ControllersRemaining == 0 && PreviousUserInfo && PreviousUserInfo != NewUserInfo)
	{
		// This means that the user deliberately logged out using a platform interface
		int32 PlatformUserIndex = GetPlatformUserIndexForControllerId(ControllerId);

		if (IsValidPlatformUserIndex(PlatformUserIndex))
		{
			LogOutLocalUser(ControllerId);
		}
	}

	if (ControlledLocalPlayer && ControlledLocalPlayer != NewLocalPlayer)
	{
		// TODO Currently the platforms that call this delegate do not really handle swapping controller IDs
		// SetLocalPlayerUserIndex(ControlledLocalPlayer, -1);
	}
}

void UCommonUserSubsystem::HandleNetworkConnectionStatusChanged(const FString& ServiceName, EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus, ECommonUserOnlineContext Context)
{
	UE_LOG(LogCommonUser, Log, TEXT("HandleNetworkConnectionStatusChanged(ServiceName: %s, LastStatus: %s, ConnectionStatus: %s)"),
		*ServiceName,
		EOnlineServerConnectionStatus::ToString(LastConnectionStatus),
		EOnlineServerConnectionStatus::ToString(ConnectionStatus));

	// Cache old availablity for current users
	TMap<UCommonUserInfo*, ECommonUserAvailability> AvailabilityMap;

	for (TPair<int32, UCommonUserInfo*> Pair : LocalUserInfos)
	{
		AvailabilityMap.Add(Pair.Value, Pair.Value->GetPrivilegeAvailability(ECommonUserPrivilege::CanPlayOnline));
	}

	FOnlineContextCache* System = GetContextCache(Context);
	if (ensure(System))
	{
		// Service name is normally the same as the OSS name, but not necessarily on all platforms
		System->CurrentConnectionStatus = ConnectionStatus;
	}

	for (TPair<UCommonUserInfo*, ECommonUserAvailability> Pair : AvailabilityMap)
	{
		// Notify other systems when someone goes online/offline
		HandleChangedAvailability(Pair.Key, ECommonUserPrivilege::CanPlayOnline, Pair.Value);
	}

}
#else
void UCommonUserSubsystem::HandleAuthLoginStatusChanged(const UE::Online::FLoginStatusChanged& EventParameters, ECommonUserOnlineContext Context)
{
	UE_LOG(LogCommonUser, Log, TEXT("Player login status changed - System:%d, UserId:%s, OldStatus:%s, NewStatus:%s"),
		(int)Context,
		*ToLogString(EventParameters.LocalUserId),
		LexToString(EventParameters.PreviousStatus),
		LexToString(EventParameters.CurrentStatus));
}

void UCommonUserSubsystem::HandleNetworkConnectionStatusChanged(const UE::Online::FConnectionStatusChanged& EventParameters, ECommonUserOnlineContext Context)
{
	UE_LOG(LogCommonUser, Log, TEXT("HandleNetworkConnectionStatusChanged(Context:%d, ServiceName:%s, OldStatus:%s, NewStatus:%s)"),
		(int)Context,
		*EventParameters.ServiceName,
		LexToString(EventParameters.PreviousStatus),
		LexToString(EventParameters.CurrentStatus));

	// Cache old availablity for current users
	TMap<UCommonUserInfo*, ECommonUserAvailability> AvailabilityMap;

	for (TPair<int32, UCommonUserInfo*> Pair : LocalUserInfos)
	{
		AvailabilityMap.Add(Pair.Value, Pair.Value->GetPrivilegeAvailability(ECommonUserPrivilege::CanPlayOnline));
	}

	FOnlineContextCache* System = GetContextCache(Context);
	if (ensure(System))
	{
		// Service name is normally the same as the OSS name, but not necessarily on all platforms
		System->CurrentConnectionStatus = EventParameters.CurrentStatus;
	}

	for (TPair<UCommonUserInfo*, ECommonUserAvailability> Pair : AvailabilityMap)
	{
		// Notify other systems when someone goes online/offline
		HandleChangedAvailability(Pair.Key, ECommonUserPrivilege::CanPlayOnline, Pair.Value);
	}
}
#endif // COMMONUSER_OSSV1

void UCommonUserSubsystem::HandleControllerConnectionChanged(bool bConnected, FPlatformUserId UserId, int32 ControllerId)
{
	UE_LOG(LogCommonUser, Log, TEXT("Controller connection changed - UserIdx:%d, UserID:%s, Connected:%d"), ControllerId, *PlatformUserIdToString(UserId), bConnected ? 1 : 0);

	// TODO Implement for platforms that support this
}
