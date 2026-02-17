// Copyright Epic Games, Inc. All Rights Reserved.

#include "UserManagerEOS.h"
#include "OnlineSubsystemEOS.h"
#include "OnlineSubsystemEOSPrivate.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/OutputDeviceRedirector.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "OnlineError.h"
#include "EOSSettings.h"
#include "CoreMinimal.h"
#include "IEOSSDKManager.h"

#include COMPILED_PLATFORM_HEADER(EOSHelpers.h)

#if WITH_EOS_SDK

#include "eos_auth.h"
#include "eos_userinfo.h"
#include "eos_friends.h"
#include "eos_presence.h"
#include "eos_ui.h"

static inline EInviteStatus::Type ToEInviteStatus(EOS_EFriendsStatus InStatus)
{
	switch (InStatus)
	{
		case EOS_EFriendsStatus::EOS_FS_InviteSent:
		{
			return EInviteStatus::PendingOutbound;
		}
		case EOS_EFriendsStatus::EOS_FS_InviteReceived:
		{
			return EInviteStatus::PendingInbound;
		}
		case EOS_EFriendsStatus::EOS_FS_Friends:
		{
			return EInviteStatus::Accepted;
		}
	}
	return EInviteStatus::Unknown;
}

static inline EOnlinePresenceState::Type ToEOnlinePresenceState(EOS_Presence_EStatus InStatus)
{
	switch (InStatus)
	{
		case EOS_Presence_EStatus::EOS_PS_Online:
		{
			return EOnlinePresenceState::Online;
		}
		case EOS_Presence_EStatus::EOS_PS_Away:
		{
			return EOnlinePresenceState::Away;
		}
		case EOS_Presence_EStatus::EOS_PS_ExtendedAway:
		{
			return EOnlinePresenceState::ExtendedAway;
		}
		case EOS_Presence_EStatus::EOS_PS_DoNotDisturb:
		{
			return EOnlinePresenceState::DoNotDisturb;
		}
	}
	return EOnlinePresenceState::Offline;
}

static inline EOS_Presence_EStatus ToEOS_Presence_EStatus(EOnlinePresenceState::Type InStatus)
{
	switch (InStatus)
	{
		case EOnlinePresenceState::Online:
		{
			return EOS_Presence_EStatus::EOS_PS_Online;
		}
		case EOnlinePresenceState::Away:
		{
			return EOS_Presence_EStatus::EOS_PS_Away;
		}
		case EOnlinePresenceState::ExtendedAway:
		{
			return EOS_Presence_EStatus::EOS_PS_ExtendedAway;
		}
		case EOnlinePresenceState::DoNotDisturb:
		{
			return EOS_Presence_EStatus::EOS_PS_DoNotDisturb;
		}
	}
	return EOS_Presence_EStatus::EOS_PS_Offline;
}

static inline EOS_EExternalCredentialType ToEOS_EExternalCredentialType(FName OSSName, const FOnlineAccountCredentials& AccountCredentials)
{
#if PLATFORM_DESKTOP
	if (OSSName == STEAM_SUBSYSTEM)
	{
		return EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET;
	}
#endif
	if (OSSName == PS4_SUBSYSTEM || USE_PSN_ID_TOKEN)
	{
		return EOS_EExternalCredentialType::EOS_ECT_PSN_ID_TOKEN;
	}
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	else if (OSSName == LIVE_SUBSYSTEM || USE_XBL_XSTS_TOKEN)
	{
		return EOS_EExternalCredentialType::EOS_ECT_XBL_XSTS_TOKEN;
	}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	else if (OSSName == SWITCH_SUBSYSTEM)
	{
		if (AccountCredentials.Type == TEXT("NintendoAccount"))
		{
			return EOS_EExternalCredentialType::EOS_ECT_NINTENDO_ID_TOKEN;
		}
		else
		{
			return EOS_EExternalCredentialType::EOS_ECT_NINTENDO_NSA_ID_TOKEN;
		}
	}
	else if (OSSName == APPLE_SUBSYSTEM)
	{
		return EOS_EExternalCredentialType::EOS_ECT_APPLE_ID_TOKEN;
	}
	// Unknown means OpenID
	return EOS_EExternalCredentialType::EOS_ECT_OPENID_ACCESS_TOKEN;
}

/** Delegates that are used for internal calls and are meant to be ignored */
IOnlinePresence::FOnPresenceTaskCompleteDelegate IgnoredPresenceDelegate;
IOnlineUser::FOnQueryExternalIdMappingsComplete IgnoredMappingDelegate;

typedef TEOSGlobalCallback<EOS_UI_OnDisplaySettingsUpdatedCallback, EOS_UI_OnDisplaySettingsUpdatedCallbackInfo, FUserManagerEOS> FOnDisplaySettingsUpdatedCallback;

FUserManagerEOS::FUserManagerEOS(FOnlineSubsystemEOS* InSubsystem)
	: TSharedFromThis<FUserManagerEOS, ESPMode::ThreadSafe>()
	, EOSSubsystem(InSubsystem)
	, DefaultLocalUser(-1)
	, LoginNotificationId(0)
	, LoginNotificationCallback(nullptr)
	, FriendsNotificationId(0)
	, FriendsNotificationCallback(nullptr)
	, PresenceNotificationId(0)
	, PresenceNotificationCallback(nullptr)
	, DisplaySettingsUpdatedId(0)
	, DisplaySettingsUpdatedCallback(nullptr)
{
}

void FUserManagerEOS::Init()
{
	// This delegate would cause a crash when running a dedicated server
	if (!IsRunningDedicatedServer())
	{
		// Adding subscription to external ui display change event
		EOS_UI_AddNotifyDisplaySettingsUpdatedOptions Options = {};
		Options.ApiVersion = EOS_UI_ADDNOTIFYDISPLAYSETTINGSUPDATED_API_LATEST;

		FOnDisplaySettingsUpdatedCallback* CallbackObj = new FOnDisplaySettingsUpdatedCallback(AsWeak());
		DisplaySettingsUpdatedCallback = CallbackObj;
		CallbackObj->CallbackLambda = [this](const EOS_UI_OnDisplaySettingsUpdatedCallbackInfo* Data)
		{
			TriggerOnExternalUIChangeDelegates((bool)Data->bIsVisible);
		};

		DisplaySettingsUpdatedId = EOS_UI_AddNotifyDisplaySettingsUpdated(EOSSubsystem->UIHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
}

void FUserManagerEOS::Shutdown()
{
	// This delegate would cause a crash when running a dedicated server
	if (DisplaySettingsUpdatedId != EOS_INVALID_NOTIFICATIONID)
	{
		// Removing subscription to external ui display change event
		EOS_UI_RemoveNotifyDisplaySettingsUpdated(EOSSubsystem->UIHandle, DisplaySettingsUpdatedId);

		if (DisplaySettingsUpdatedCallback)
		{
			delete DisplaySettingsUpdatedCallback;
		}
	}
}

FUserManagerEOS::~FUserManagerEOS()
{
	Shutdown();
}

void FUserManagerEOS::LoginStatusChanged(const EOS_Auth_LoginStatusChangedCallbackInfo* Data)
{
	if (Data->CurrentStatus == EOS_ELoginStatus::EOS_LS_NotLoggedIn)
	{
		if (AccountIdToUserNumMap.Contains(Data->LocalUserId))
		{
			int32 LocalUserNum = AccountIdToUserNumMap[Data->LocalUserId];
			FUniqueNetIdEOSPtr UserNetId = UserNumToNetIdMap[LocalUserNum];
			TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *UserNetId);
			// Need to remove the local user
			RemoveLocalUser(LocalUserNum);

			// Clean up user based notifies if we have no logged in users
			if (UserNumToNetIdMap.Num() == 0)
			{
				if (LoginNotificationId > 0)
				{
					// Remove the callback
					EOS_Auth_RemoveNotifyLoginStatusChanged(EOSSubsystem->AuthHandle, LoginNotificationId);
					delete LoginNotificationCallback;
					LoginNotificationCallback = nullptr;
					LoginNotificationId = 0;
				}
				if (FriendsNotificationId > 0)
				{
					EOS_Friends_RemoveNotifyFriendsUpdate(EOSSubsystem->FriendsHandle, FriendsNotificationId);
					delete FriendsNotificationCallback;
					FriendsNotificationCallback = nullptr;
					FriendsNotificationId = 0;
				}
				if (PresenceNotificationId > 0)
				{
					EOS_Presence_RemoveNotifyOnPresenceChanged(EOSSubsystem->PresenceHandle, PresenceNotificationId);
					delete PresenceNotificationCallback;
					PresenceNotificationCallback = nullptr;
					PresenceNotificationId = 0;
				}
				// Remove the per user connect login notification
				if (LocalUserNumToConnectLoginNotifcationMap.Contains(LocalUserNum))
				{
					FNotificationIdCallbackPair* NotificationPair = LocalUserNumToConnectLoginNotifcationMap[LocalUserNum];
					LocalUserNumToConnectLoginNotifcationMap.Remove(LocalUserNum);

					EOS_Connect_RemoveNotifyAuthExpiration(EOSSubsystem->ConnectHandle, NotificationPair->NotificationId);

					delete NotificationPair;
				}
			}
		}
	}
}

IOnlineSubsystem* FUserManagerEOS::GetPlatformOSS() const
{
	IOnlineSubsystem* PlatformOSS = IOnlineSubsystem::GetByPlatform();
	if (PlatformOSS == nullptr)
#if !PLATFORM_DESKTOP
	{
		UE_LOG_ONLINE(Error, TEXT("GetPlatformOSS() failed due to no platform OSS being configured"));
	}
#else
	{
		// Attempt to load Steam before treating it as an error
		PlatformOSS = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	}
#endif
	return PlatformOSS;
}

void FUserManagerEOS::GetPlatformAuthToken(int32 LocalUserNum, const FOnGetLinkedAccountAuthTokenCompleteDelegate& Delegate) const
{
	IOnlineSubsystem* PlatformOSS = GetPlatformOSS();
	if (PlatformOSS == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("ConnectLoginNoEAS(%d) failed due to no platform OSS"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FExternalAuthToken());
		return;
	}
	IOnlineIdentityPtr PlatformIdentity = PlatformOSS->GetIdentityInterface();
	if (!PlatformIdentity.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("ConnectLoginNoEAS(%d) failed due to no platform OSS identity interface"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FExternalAuthToken());
		return;
	}
	// Request the auth token from the platform
	PlatformIdentity->GetLinkedAccountAuthToken(LocalUserNum, Delegate);
}

FString FUserManagerEOS::GetPlatformDisplayName(int32 LocalUserNum) const
{
	FString Result;

	IOnlineSubsystem* PlatformOSS = GetPlatformOSS();
	if (PlatformOSS == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("GetPlatformDisplayName(%d) failed due to no platform OSS"), LocalUserNum);
		return Result;
	}
	IOnlineIdentityPtr PlatformIdentity = PlatformOSS->GetIdentityInterface();
	if (!PlatformIdentity.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("GetPlatformDisplayName(%d) failed due to no platform OSS identity interface"), LocalUserNum);
		return Result;
	}

	Result = PlatformIdentity->GetPlayerNickname(LocalUserNum);

	return Result;
}

typedef TEOSCallback<EOS_Auth_OnLoginCallback, EOS_Auth_LoginCallbackInfo, FUserManagerEOS> FLoginCallback;
typedef TEOSCallback<EOS_Connect_OnLoginCallback, EOS_Connect_LoginCallbackInfo, FUserManagerEOS> FConnectLoginCallback;
typedef TEOSCallback<EOS_Auth_OnDeletePersistentAuthCallback, EOS_Auth_DeletePersistentAuthCallbackInfo, FUserManagerEOS> FDeletePersistentAuthCallback;

// Chose arbitrarily since the SDK doesn't define it
#define EOS_MAX_TOKEN_SIZE 4096

struct FAuthCredentials :
	public EOS_Auth_Credentials
{
	FAuthCredentials() :
		EOS_Auth_Credentials()
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Id = IdAnsi;
		Token = TokenAnsi;

		FMemory::Memset(IdAnsi, 0, sizeof(IdAnsi));
		FMemory::Memset(TokenAnsi, 0, sizeof(TokenAnsi));
	}

	FAuthCredentials(const FAuthCredentials& Other)
	{
		ApiVersion = Other.ApiVersion;
		Id = IdAnsi;
		Token = TokenAnsi;
		Type = Other.Type;
		SystemAuthCredentialsOptions = Other.SystemAuthCredentialsOptions;
		ExternalType = Other.ExternalType;

		FCStringAnsi::Strncpy(IdAnsi, Other.IdAnsi, EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(TokenAnsi, Other.TokenAnsi, EOS_MAX_TOKEN_SIZE);
	}

	FAuthCredentials& operator=(FAuthCredentials& Other)
	{
		ApiVersion = Other.ApiVersion;
		Type = Other.Type;
		SystemAuthCredentialsOptions = Other.SystemAuthCredentialsOptions;
		ExternalType = Other.ExternalType;

		FCStringAnsi::Strncpy(IdAnsi, Other.IdAnsi, EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(TokenAnsi, Other.TokenAnsi, EOS_MAX_TOKEN_SIZE);

		return *this;
	}

	FAuthCredentials(EOS_EExternalCredentialType InExternalType, const FExternalAuthToken& AuthToken) :
		EOS_Auth_Credentials()
	{
		if (AuthToken.HasTokenData())
		{
			Init(InExternalType, AuthToken.TokenData);
		}
		else if (AuthToken.HasTokenString())
		{
			Init(InExternalType, AuthToken.TokenString);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FAuthCredentials object cannot be constructed with invalid FExternalAuthToken parameter"));
		}
	}

	void Init(EOS_EExternalCredentialType InExternalType, const FString& InTokenString)
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		ExternalType = InExternalType;
		Id = IdAnsi;
		Token = TokenAnsi;

		FCStringAnsi::Strncpy(TokenAnsi, TCHAR_TO_UTF8(*InTokenString), InTokenString.Len()+1);
	}

	void Init(EOS_EExternalCredentialType InExternalType, const TArray<uint8>& InToken)
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		ExternalType = InExternalType;
		Id = IdAnsi;
		Token = TokenAnsi;

		uint32_t InOutBufferLength = EOS_MAX_TOKEN_SIZE;
		EOS_ByteArray_ToString(InToken.GetData(), InToken.Num(), TokenAnsi, &InOutBufferLength);
	}

	char IdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char TokenAnsi[EOS_MAX_TOKEN_SIZE];
};

bool FUserManagerEOS::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	LocalUserNumToLastLoginCredentials.Emplace(LocalUserNum, MakeShared<FOnlineAccountCredentials>(AccountCredentials));

	FEOSSettings Settings = UEOSSettings::GetSettings();

	// Are we configured to run at all?
	if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && !Settings.bUseEAS && !Settings.bUseEOSConnect)
	{
		UE_LOG_ONLINE(Warning, TEXT("Neither EAS nor EOS are configured to be used. Failed to login in user (%d)"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Not configured")));
		return true;
	}

	// See if we are configured to just use EOS and not EAS
	if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && !Settings.bUseEAS && Settings.bUseEOSConnect)
	{
		// Call the EOS + Platform login path
		return ConnectLoginNoEAS(LocalUserNum);
	}

	// We don't support offline logged in, so they are either logged in or not
	if (GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		UE_LOG_ONLINE(Warning, TEXT("User (%d) already logged in."), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Already logged in")));
		return true;
	}

	// See if we are logging in using platform credentials to link to EAS
	if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && Settings.bUseEAS)
	{
		LoginViaExternalAuth(LocalUserNum);
		return true;
	}

	EOS_Auth_LoginOptions LoginOptions = { };
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
	LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

	FPlatformEOSHelpersPtr EOSHelpers = EOSSubsystem->GetEOSHelpers();

	FAuthCredentials Credentials;
	LoginOptions.Credentials = &Credentials;
	EOSHelpers->PlatformAuthCredentials(Credentials);

	bool bIsPersistentLogin = false;

	if (AccountCredentials.Type == TEXT("exchangecode"))
	{
		// This is how the Epic launcher will pass credentials to you
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
	}
	else if (AccountCredentials.Type == TEXT("developer"))
	{
		// This is auth via the EOS auth tool
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
		FCStringAnsi::Strncpy(Credentials.IdAnsi, TCHAR_TO_UTF8(*AccountCredentials.Id), EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
	}
	else if (AccountCredentials.Type == TEXT("password"))
	{
		// This is using a direct username / password. Restricted and not generally available.
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Password;
		FCStringAnsi::Strncpy(Credentials.IdAnsi, TCHAR_TO_UTF8(*AccountCredentials.Id), EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
	}
	else if (AccountCredentials.Type == TEXT("accountportal"))
	{
		// This is auth via the EOS Account Portal
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
	}
	else if (AccountCredentials.Type == TEXT("persistentauth"))
	{
		// Use locally stored token managed by EOSSDK keyring to attempt login.
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_PersistentAuth;

		// Id and Token must be null when using EOS_ELoginCredentialType::EOS_LCT_PersistentAuth
		Credentials.Id = nullptr;
		Credentials.Token = nullptr;

		// Store selection of persistent auth.
		// The persistent auth token is handled by the EOSSDK. On a login failure the persistent token may need to be deleted if it is invalid.
		bIsPersistentLogin = true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Unable to Login() user (%d) due to missing auth parameters"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Missing auth parameters")));
		return false;
	}

	FLoginCallback* CallbackObj = new FLoginCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, LocalUserNum, bIsPersistentLogin](const EOS_Auth_LoginCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			// Continue the login process by getting the product user id for EAS only
			ConnectLoginEAS(LocalUserNum, Data->LocalUserId);
		}
		else
		{
			auto TriggerLoginFailure = [this, LocalUserNum, LoginResultCode = Data->ResultCode]()
			{
				FString ErrorString = FString::Printf(TEXT("Login(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(LoginResultCode)));
				UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
				TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
			};

			const bool bShouldRemoveCachedToken =
				Data->ResultCode == EOS_EResult::EOS_InvalidAuth ||
				Data->ResultCode == EOS_EResult::EOS_AccessDenied ||
				Data->ResultCode == EOS_EResult::EOS_Auth_InvalidToken;

			// Check for invalid persistent login credentials.
			if (bIsPersistentLogin && bShouldRemoveCachedToken)
			{
				FDeletePersistentAuthCallback* DeleteAuthCallbackObj = new FDeletePersistentAuthCallback(AsWeak());
				DeleteAuthCallbackObj->CallbackLambda = [this, LocalUserNum, TriggerLoginFailure](const EOS_Auth_DeletePersistentAuthCallbackInfo* Data)
				{
					// Deleting the auth token is best effort.
					TriggerLoginFailure();
				};

				EOS_Auth_DeletePersistentAuthOptions DeletePersistentAuthOptions;
				DeletePersistentAuthOptions.ApiVersion = EOS_AUTH_DELETEPERSISTENTAUTH_API_LATEST;
				DeletePersistentAuthOptions.RefreshToken = nullptr;
				EOS_Auth_DeletePersistentAuth(EOSSubsystem->AuthHandle, &DeletePersistentAuthOptions, (void*)DeleteAuthCallbackObj, DeleteAuthCallbackObj->GetCallbackPtr());
			}
			else
			{
				TriggerLoginFailure();
			}
		}
	};
	// Perform the auth call
	EOS_Auth_Login(EOSSubsystem->AuthHandle, &LoginOptions, (void*)CallbackObj, CallbackObj->GetCallbackPtr());
	return true;
}

void FUserManagerEOS::LoginViaExternalAuth(int32 LocalUserNum)
{
	GetPlatformAuthToken(LocalUserNum,
		FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda([this, WeakThis = AsWeak()](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				if (!bWasSuccessful || !AuthToken.IsValid())
				{
					UE_LOG_ONLINE(Warning, TEXT("Unable to Login() user (%d) due to an empty platform auth token"), LocalUserNum);
					TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Missing platform auth token")));
					return;
				}

				EOS_Auth_LoginOptions LoginOptions = { };
				LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
				LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

				check(LocalUserNumToLastLoginCredentials.Contains(LocalUserNum));
				FAuthCredentials Credentials(ToEOS_EExternalCredentialType(GetPlatformOSS()->GetSubsystemName(), *LocalUserNumToLastLoginCredentials[LocalUserNum]), AuthToken);
				LoginOptions.Credentials = &Credentials;

				FLoginCallback* CallbackObj = new FLoginCallback(AsWeak());
				CallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Auth_LoginCallbackInfo* Data)
				{
					if (Data->ResultCode == EOS_EResult::EOS_Success)
					{
						ConnectLoginEAS(LocalUserNum, Data->LocalUserId);
					}
					else if (Data->ResultCode == EOS_EResult::EOS_InvalidUser)
					{
						// Link the account
						LinkEAS(LocalUserNum, Data->ContinuanceToken);
					}
					else
					{
						FString ErrorString = FString::Printf(TEXT("Login(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
						UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
						TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
					}
				};
				// Perform the auth call
				EOS_Auth_Login(EOSSubsystem->AuthHandle, &LoginOptions, (void*)CallbackObj, CallbackObj->GetCallbackPtr());
			}
		}));
}

struct FLinkAccountOptions :
	public EOS_Auth_LinkAccountOptions
{
	FLinkAccountOptions(EOS_ContinuanceToken Token)
		: EOS_Auth_LinkAccountOptions()
	{
		ApiVersion = EOS_AUTH_LINKACCOUNT_API_LATEST;
		ContinuanceToken = Token;
	}
};

typedef TEOSCallback<EOS_Auth_OnLinkAccountCallback, EOS_Auth_LinkAccountCallbackInfo, FUserManagerEOS> FLinkAccountCallback;

void FUserManagerEOS::LinkEAS(int32 LocalUserNum, EOS_ContinuanceToken Token)
{
	FLinkAccountOptions Options(Token);
	FLinkAccountCallback* CallbackObj = new FLinkAccountCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Auth_LinkAccountCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			// Continue the login process by getting the product user id
			ConnectLoginEAS(LocalUserNum, Data->LocalUserId);
		}
		else
		{
			FString ErrorString = FString::Printf(TEXT("Login(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
			TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
		}
	};
	EOS_Auth_LinkAccount(EOSSubsystem->AuthHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
}

struct FConnectCredentials :
	public EOS_Connect_Credentials
{
	FConnectCredentials(EOS_EExternalCredentialType InType, const FExternalAuthToken& AuthToken) :
		EOS_Connect_Credentials()
	{
		if (AuthToken.HasTokenData())
		{
			Init(InType, AuthToken.TokenData);
		}
		else if (AuthToken.HasTokenString())
		{
			Init(InType, AuthToken.TokenString);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FConnectCredentials object cannot be constructed with invalid FExternalAuthToken parameter"));
		}
	}

	void Init(EOS_EExternalCredentialType InType, const FString& InTokenString)
	{
		ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Token = TokenAnsi;
		Type = InType;

		FCStringAnsi::Strncpy(TokenAnsi, TCHAR_TO_UTF8(*InTokenString), InTokenString.Len() + 1);
	}

	void Init(EOS_EExternalCredentialType InType, const TArray<uint8>& InToken)
	{
		ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Token = TokenAnsi;
		Type = InType;

		uint32_t InOutBufferLength = EOS_MAX_TOKEN_SIZE;
		EOS_ByteArray_ToString(InToken.GetData(), InToken.Num(), TokenAnsi, &InOutBufferLength);
	}

	char TokenAnsi[EOS_MAX_TOKEN_SIZE];
};

bool FUserManagerEOS::ConnectLoginNoEAS(int32 LocalUserNum)
{
	GetPlatformAuthToken(LocalUserNum,
		FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda([this, WeakThis = AsWeak()](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				if (!bWasSuccessful || !AuthToken.IsValid())
				{
					const FString ErrorString = FString::Printf(TEXT("ConnectLoginNoEAS(%d) failed due to the platform OSS giving an empty auth token"), LocalUserNum);
					UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
					TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
					return;
				}

				// Now login into our EOS account
				check(LocalUserNumToLastLoginCredentials.Contains(LocalUserNum));
				FConnectCredentials Credentials(ToEOS_EExternalCredentialType(GetPlatformOSS()->GetSubsystemName(), *LocalUserNumToLastLoginCredentials[LocalUserNum]), AuthToken);
				EOS_Connect_LoginOptions Options = { };
				Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
				Options.Credentials = &Credentials;

#if ADD_USER_LOGIN_INFO
				EOS_Connect_UserLoginInfo UserLoginInfo = {};
				UserLoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
				const FTCHARToUTF8 DisplayNameUtf8(*GetPlatformDisplayName(LocalUserNum));
				UserLoginInfo.DisplayName = DisplayNameUtf8.Get();

				Options.UserLoginInfo = &UserLoginInfo;
#endif

				FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
				CallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Data)
				{
					if (Data->ResultCode == EOS_EResult::EOS_Success)
					{
						// We have an account mapping to the platform account, skip to final login
						FullLoginCallback(LocalUserNum, nullptr, Data->LocalUserId);
					}
					else if (Data->ResultCode == EOS_EResult::EOS_InvalidUser)
					{
						// We need to create the platform account mapping for this user using the continuation token
						CreateConnectedLogin(LocalUserNum, nullptr, Data->ContinuanceToken);
					}
					else
					{
						const FString ErrorString = FString::Printf(TEXT("ConnectLoginNoEAS(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
						UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
						TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
					}
				};
				EOS_Connect_Login(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
			}
		}));

	return true;
}

bool FUserManagerEOS::ConnectLoginEAS(int32 LocalUserNum, EOS_EpicAccountId AccountId)
{
	EOS_Auth_Token* AuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions CopyOptions = { };
	CopyOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->AuthHandle, &CopyOptions, AccountId, &AuthToken);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		EOS_Connect_Credentials Credentials = { };
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
		Credentials.Token = AuthToken->AccessToken;

		EOS_Connect_LoginOptions Options = { };
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;

		FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
		CallbackObj->CallbackLambda = [LocalUserNum, AccountId, this](const EOS_Connect_LoginCallbackInfo* Data)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				// We have an account mapping, skip to final login
				FullLoginCallback(LocalUserNum, AccountId, Data->LocalUserId);
			}
			else if (Data->ResultCode == EOS_EResult::EOS_InvalidUser)
			{
				// We need to create the mapping for this user using the continuation token
				CreateConnectedLogin(LocalUserNum, AccountId, Data->ContinuanceToken);
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
				Logout(LocalUserNum);
			}
		};
		EOS_Connect_Login(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

		EOS_Auth_Token_Release(AuthToken);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(CopyResult)));
		Logout(LocalUserNum);
	}
	return true;
}

void FUserManagerEOS::RefreshConnectLogin(int32 LocalUserNum)
{
	if (!UserNumToAccountIdMap.Contains(LocalUserNum))
	{
		UE_LOG_ONLINE(Error, TEXT("Can't refresh ConnectLogin(%d) since (%d) is not logged in"), LocalUserNum, LocalUserNum);
		return;
	}

	const FEOSSettings Settings = UEOSSettings::GetSettings();
	if (Settings.bUseEAS)
	{
		EOS_EpicAccountId AccountId = UserNumToAccountIdMap[LocalUserNum];
		EOS_Auth_Token* AuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions CopyOptions = { };
		CopyOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->AuthHandle, &CopyOptions, AccountId, &AuthToken);
		if (CopyResult == EOS_EResult::EOS_Success)
		{
			// We update the auth token cached in the user account, along with the user information
			const FUniqueNetIdEOSPtr UniqueNetId = UserNumToNetIdMap.FindChecked(LocalUserNum);
			const FUserOnlineAccountEOSRef UserAccountRef = StringToUserAccountMap.FindChecked(UniqueNetId->ToString());
			UserAccountRef->SetAuthAttribute(AUTH_ATTR_ID_TOKEN, AuthToken->AccessToken);
			UpdateUserInfo(UserAccountRef, AccountId, AccountId);

			EOS_Connect_Credentials Credentials = { };
			Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
			Credentials.Token = AuthToken->AccessToken;

			EOS_Connect_LoginOptions Options = { };
			Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			Options.Credentials = &Credentials;

			FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
			CallbackObj->CallbackLambda = [LocalUserNum, AccountId, this](const EOS_Connect_LoginCallbackInfo* Data)
			{
				if (Data->ResultCode != EOS_EResult::EOS_Success)
				{
					UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
					Logout(LocalUserNum);
				}
			};
			EOS_Connect_Login(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

			EOS_Auth_Token_Release(AuthToken);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(CopyResult)));
			Logout(LocalUserNum);
		}
	}
	else
	{
		// Not using EAS so grab the platform auth token
		GetPlatformAuthToken(LocalUserNum,
			FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda([this, WeakThis = AsWeak()](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
			{
				if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
				{
					if (!bWasSuccessful || !AuthToken.IsValid())
					{
						UE_LOG_ONLINE(Error, TEXT("ConnectLoginNoEAS(%d) failed due to the platform OSS giving an empty auth token"), LocalUserNum);
						Logout(LocalUserNum);
						return;
					}

					// Now login into our EOS account
					check(LocalUserNumToLastLoginCredentials.Contains(LocalUserNum));
					const FOnlineAccountCredentials& Creds = *LocalUserNumToLastLoginCredentials[LocalUserNum];
					EOS_EExternalCredentialType CredType = ToEOS_EExternalCredentialType(GetPlatformOSS()->GetSubsystemName(), Creds);
					FConnectCredentials Credentials(CredType, AuthToken);
					EOS_Connect_LoginOptions Options = { };
					Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
					Options.Credentials = &Credentials;

					FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
					CallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Data)
					{
						if (Data->ResultCode != EOS_EResult::EOS_Success)
						{
							UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
							Logout(LocalUserNum);
						}
					};
					EOS_Connect_Login(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
				}
			}));
	}
}

typedef TEOSCallback<EOS_Connect_OnCreateUserCallback, EOS_Connect_CreateUserCallbackInfo, FUserManagerEOS> FCreateUserCallback;

void FUserManagerEOS::CreateConnectedLogin(int32 LocalUserNum, EOS_EpicAccountId AccountId, EOS_ContinuanceToken Token)
{
	EOS_Connect_CreateUserOptions Options = { };
	Options.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
	Options.ContinuanceToken = Token;

	FCreateUserCallback* CallbackObj = new FCreateUserCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, AccountId, this](const EOS_Connect_CreateUserCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			FullLoginCallback(LocalUserNum, AccountId, Data->LocalUserId);
		}
		else
		{
// @todo joeg - logout?
			FString ErrorString = FString::Printf(TEXT("Login(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
		}
	};
	EOS_Connect_CreateUser(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
}

typedef TEOSGlobalCallback<EOS_Connect_OnAuthExpirationCallback, EOS_Connect_AuthExpirationCallbackInfo, FUserManagerEOS> FRefreshAuthCallback;
typedef TEOSGlobalCallback<EOS_Presence_OnPresenceChangedCallback, EOS_Presence_PresenceChangedCallbackInfo, FUserManagerEOS> FPresenceChangedCallback;
typedef TEOSGlobalCallback<EOS_Friends_OnFriendsUpdateCallback, EOS_Friends_OnFriendsUpdateInfo, FUserManagerEOS> FFriendsStatusUpdateCallback;
typedef TEOSGlobalCallback<EOS_Auth_OnLoginStatusChangedCallback, EOS_Auth_LoginStatusChangedCallbackInfo, FUserManagerEOS> FLoginStatusChangedCallback;

void FUserManagerEOS::FullLoginCallback(int32 LocalUserNum, EOS_EpicAccountId AccountId, EOS_ProductUserId UserId)
{
	// Add our login status changed callback if not already set
	if (LoginNotificationId == 0)
	{
		FLoginStatusChangedCallback* CallbackObj = new FLoginStatusChangedCallback(AsWeak());
		LoginNotificationCallback = CallbackObj;
		CallbackObj->CallbackLambda = [this](const EOS_Auth_LoginStatusChangedCallbackInfo* Data)
		{
			LoginStatusChanged(Data);
		};

		EOS_Auth_AddNotifyLoginStatusChangedOptions Options = { };
		Options.ApiVersion = EOS_AUTH_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST;
		LoginNotificationId = EOS_Auth_AddNotifyLoginStatusChanged(EOSSubsystem->AuthHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	// Register for friends updates if not set yet
	if (FriendsNotificationId == 0)
	{
		FFriendsStatusUpdateCallback* CallbackObj = new FFriendsStatusUpdateCallback(AsWeak());
		FriendsNotificationCallback = CallbackObj;
		CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Friends_OnFriendsUpdateInfo* Data)
		{
			FriendStatusChanged(Data);
		};

		EOS_Friends_AddNotifyFriendsUpdateOptions Options = { };
		Options.ApiVersion = EOS_FRIENDS_ADDNOTIFYFRIENDSUPDATE_API_LATEST;
		FriendsNotificationId = EOS_Friends_AddNotifyFriendsUpdate(EOSSubsystem->FriendsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	// Register for presence updates if not set yet
	if (PresenceNotificationId == 0)
	{
		FPresenceChangedCallback* CallbackObj = new FPresenceChangedCallback(AsWeak());
		PresenceNotificationCallback = CallbackObj;
		CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Presence_PresenceChangedCallbackInfo* Data)
		{
			if (EpicAccountIdToOnlineUserMap.Contains(Data->PresenceUserId))
			{
				// Update the presence data to the most recent
				UpdatePresence(Data->PresenceUserId);
				return;
			}
		};

		EOS_Presence_AddNotifyOnPresenceChangedOptions Options = { };
		Options.ApiVersion = EOS_PRESENCE_ADDNOTIFYONPRESENCECHANGED_API_LATEST;
		PresenceNotificationId = EOS_Presence_AddNotifyOnPresenceChanged(EOSSubsystem->PresenceHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	// Add auth refresh notification if not set for this user yet
	if (!LocalUserNumToConnectLoginNotifcationMap.Contains(LocalUserNum))
	{
		FNotificationIdCallbackPair* NotificationPair = new FNotificationIdCallbackPair();
		LocalUserNumToConnectLoginNotifcationMap.Emplace(LocalUserNum, NotificationPair);

		FRefreshAuthCallback* CallbackObj = new FRefreshAuthCallback(AsWeak());
		NotificationPair->Callback = CallbackObj;
		CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Connect_AuthExpirationCallbackInfo* Data)
		{
			RefreshConnectLogin(LocalUserNum);
		};

		EOS_Connect_AddNotifyAuthExpirationOptions Options = { };
		Options.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
		NotificationPair->NotificationId = EOS_Connect_AddNotifyAuthExpiration(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}

	AddLocalUser(LocalUserNum, AccountId, UserId);
	FUniqueNetIdEOSPtr UserNetId = GetLocalUniqueNetIdEOS(LocalUserNum);
	check(UserNetId.IsValid());

	TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UserNetId, FString());
	TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::NotLoggedIn, ELoginStatus::LoggedIn, *UserNetId);
}

typedef TEOSCallback<EOS_Auth_OnLogoutCallback, EOS_Auth_LogoutCallbackInfo, FUserManagerEOS> FLogoutCallback;

bool FUserManagerEOS::Logout(int32 LocalUserNum)
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
		return false;
	}

	FLogoutCallback* CallbackObj = new FLogoutCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Auth_LogoutCallbackInfo* Data)
	{
		FDeletePersistentAuthCallback* DeleteAuthCallbackObj = new FDeletePersistentAuthCallback(AsWeak());
		DeleteAuthCallbackObj->CallbackLambda = [this, LocalUserNum, LogoutResultCode = Data->ResultCode](const EOS_Auth_DeletePersistentAuthCallbackInfo* Data)
		{
			if (LogoutResultCode == EOS_EResult::EOS_Success)
			{
				RemoveLocalUser(LocalUserNum);

				TriggerOnLogoutCompleteDelegates(LocalUserNum, true);
			}
			else
			{
				TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
			}
		};

		EOS_Auth_DeletePersistentAuthOptions DeletePersistentAuthOptions;
		DeletePersistentAuthOptions.ApiVersion = EOS_AUTH_DELETEPERSISTENTAUTH_API_LATEST;
		DeletePersistentAuthOptions.RefreshToken = nullptr;
		EOS_Auth_DeletePersistentAuth(EOSSubsystem->AuthHandle, &DeletePersistentAuthOptions, (void*)DeleteAuthCallbackObj, DeleteAuthCallbackObj->GetCallbackPtr());
	};

	EOS_Auth_LogoutOptions LogoutOptions = { };
	LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
	LogoutOptions.LocalUserId = UserId->GetEpicAccountId();

	EOS_Auth_Logout(EOSSubsystem->AuthHandle, &LogoutOptions, CallbackObj, CallbackObj->GetCallbackPtr());

	LocalUserNumToLastLoginCredentials.Remove(LocalUserNum);

	return true;
}

bool FUserManagerEOS::AutoLogin(int32 LocalUserNum)
{
	FString LoginId;
	FString Password;
	FString AuthType;

	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), LoginId);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), Password);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

	FEOSSettings Settings = UEOSSettings::GetSettings();

	if (EOSSubsystem->bIsDefaultOSS && Settings.bUseEAS && AuthType.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Unable to AutoLogin user (%d) due to missing auth command line args"), LocalUserNum);
		return false;
	}
	FOnlineAccountCredentials Creds(AuthType, LoginId, Password);

	LocalUserNumToLastLoginCredentials.Emplace(LocalUserNum, MakeShared<FOnlineAccountCredentials>(Creds));

	return Login(LocalUserNum, Creds);
}

void FUserManagerEOS::AddLocalUser(int32 LocalUserNum, EOS_EpicAccountId EpicAccountId, EOS_ProductUserId UserId)
{
	// Set the default user to the first one that logs in
	if (DefaultLocalUser == -1)
	{
		DefaultLocalUser = LocalUserNum;
	}

	FUniqueNetIdEOSRef UserNetId = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, UserId).ToSharedRef();
	const FString& NetId = UserNetId->ToString();
	FUserOnlineAccountEOSRef UserAccountRef(new FUserOnlineAccountEOS(UserNetId));

	UserNumToNetIdMap.Emplace(LocalUserNum, UserNetId);
	UserNumToAccountIdMap.Emplace(LocalUserNum, EpicAccountId);
	AccountIdToUserNumMap.Emplace(EpicAccountId, LocalUserNum);
	NetIdStringToOnlineUserMap.Emplace(*NetId, UserAccountRef);
	StringToUserAccountMap.Emplace(NetId, UserAccountRef);
	AccountIdToStringMap.Emplace(EpicAccountId, NetId);
	ProductUserIdToStringMap.Emplace(UserId, *NetId);
	EpicAccountIdToAttributeAccessMap.Emplace(EpicAccountId, UserAccountRef);
	UserNumToProductUserIdMap.Emplace(LocalUserNum, UserId);
	ProductUserIdToUserNumMap.Emplace(UserId, LocalUserNum);

	// Init player lists
	FFriendsListEOSRef FriendsList = MakeShareable(new FFriendsListEOS(LocalUserNum, UserNetId));
	LocalUserNumToFriendsListMap.Emplace(LocalUserNum, FriendsList);
	NetIdStringToFriendsListMap.Emplace(NetId, FriendsList);
	ReadFriendsList(LocalUserNum, FString());

	FBlockedPlayersListEOSRef BlockedPlayersList = MakeShareable(new FBlockedPlayersListEOS(LocalUserNum, UserNetId));
	LocalUserNumToBlockedPlayerListMap.Emplace(LocalUserNum, BlockedPlayersList);
	NetIdStringToBlockedPlayerListMap.Emplace(NetId, BlockedPlayersList);
	QueryBlockedPlayers(*UserNetId);

	FRecentPlayersListEOSRef RecentPlayersList = MakeShareable(new FRecentPlayersListEOS(LocalUserNum, UserNetId));
	LocalUserNumToRecentPlayerListMap.Emplace(LocalUserNum, RecentPlayersList);
	NetIdStringToRecentPlayerListMap.Emplace(NetId, RecentPlayersList);

	// Get auth token info
	EOS_Auth_Token* AuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions Options = { };
	Options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->AuthHandle, &Options, EpicAccountId, &AuthToken);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		UserAccountRef->SetAuthAttribute(AUTH_ATTR_ID_TOKEN, AuthToken->AccessToken);
		EOS_Auth_Token_Release(AuthToken);

		UpdateUserInfo(UserAccountRef, EpicAccountId, EpicAccountId);
	}
}

void FUserManagerEOS::UpdateUserInfo(IAttributeAccessInterfaceRef AttributeAccessRef, EOS_EpicAccountId LocalId, EOS_EpicAccountId AccountId)
{
	EOS_UserInfo_CopyUserInfoOptions Options = { };
	Options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
	Options.LocalUserId = LocalId;
	Options.TargetUserId = AccountId;

	EOS_UserInfo* UserInfo = nullptr;

	EOS_EResult CopyResult = EOS_UserInfo_CopyUserInfo(EOSSubsystem->UserInfoHandle, &Options, &UserInfo);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		AttributeAccessRef->SetInternalAttribute(USER_ATTR_DISPLAY_NAME, UTF8_TO_TCHAR(UserInfo->DisplayName));
		AttributeAccessRef->SetInternalAttribute(USER_ATTR_COUNTRY, UTF8_TO_TCHAR(UserInfo->Country));
		AttributeAccessRef->SetInternalAttribute(USER_ATTR_LANG, UTF8_TO_TCHAR(UserInfo->PreferredLanguage));
		EOS_UserInfo_Release(UserInfo);
	}
}

bool FUserManagerEOS::IsFriendQueryUserInfoOngoing(int32 LocalUserNum)
{
	// If we have an entry for this user and the corresponding array has any element, users are still being processed
	if (IsFriendQueryUserInfoOngoingForLocalUserMap.Contains(LocalUserNum))
	{
		if(IsFriendQueryUserInfoOngoingForLocalUserMap[LocalUserNum].Num() > 0)
		{
			return true;
		}
	}
	
	if (IsPlayerQueryExternalMappingsOngoingForLocalUserMap.Contains(LocalUserNum))
	{
		if(IsPlayerQueryExternalMappingsOngoingForLocalUserMap[LocalUserNum].Num() > 0)
		{
			return true;
		}
	}

	return false;
}

TSharedPtr<FUserOnlineAccount> FUserManagerEOS::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const FUserOnlineAccountEOSRef* FoundUserAccount = StringToUserAccountMap.Find(EOSID.ToString());
	if (FoundUserAccount != nullptr)
	{
		return *FoundUserAccount;
	}

	return nullptr;
}

TArray<TSharedPtr<FUserOnlineAccount>> FUserManagerEOS::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount>> Result;

	for (TMap<FString, FUserOnlineAccountEOSRef>::TConstIterator It(StringToUserAccountMap); It; ++It)
	{
		Result.Add(It.Value());
	}
	return Result;
}

FUniqueNetIdPtr FUserManagerEOS::GetUniquePlayerId(int32 LocalUserNum) const
{
	return GetLocalUniqueNetIdEOS(LocalUserNum);
}

int32 FUserManagerEOS::GetLocalUserNumFromUniqueNetId(const FUniqueNetId& NetId) const
{
	const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(NetId);

	const EOS_EpicAccountId AccountId = EosId.GetEpicAccountId();
	if (const int32* UserNum = AccountIdToUserNumMap.Find(AccountId))
	{
		return *UserNum;
	}

	const EOS_ProductUserId ProductUserId = EosId.GetProductUserId();
	if (const int32* UserNum = ProductUserIdToUserNumMap.Find(ProductUserId))
	{
		return *UserNum;
	}

	// Use the default user if we can't find the person that they want
	return DefaultLocalUser;
}

bool FUserManagerEOS::IsLocalUser(const FUniqueNetId& NetId) const
{
	const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(NetId);
	return AccountIdToUserNumMap.Contains(EosId.GetEpicAccountId()) || ProductUserIdToUserNumMap.Contains(EosId.GetProductUserId());
}

FUniqueNetIdEOSPtr FUserManagerEOS::GetLocalUniqueNetIdEOS(int32 LocalUserNum) const
{
	const FUniqueNetIdEOSPtr* FoundId = UserNumToNetIdMap.Find(LocalUserNum);
	if (FoundId != nullptr)
	{
		return *FoundId;
	}
	return nullptr;
}

FUniqueNetIdEOSPtr FUserManagerEOS::GetLocalUniqueNetIdEOS(EOS_ProductUserId UserId) const
{
	if (ProductUserIdToUserNumMap.Contains(UserId))
	{
		return GetLocalUniqueNetIdEOS(ProductUserIdToUserNumMap[UserId]);
	}
	return nullptr;
}

FUniqueNetIdEOSPtr FUserManagerEOS::GetLocalUniqueNetIdEOS(EOS_EpicAccountId AccountId) const
{
	if (AccountIdToUserNumMap.Contains(AccountId))
	{
		return GetLocalUniqueNetIdEOS(AccountIdToUserNumMap[AccountId]);
	}
	return nullptr;
}

EOS_EpicAccountId FUserManagerEOS::GetLocalEpicAccountId(int32 LocalUserNum) const
{
	if (UserNumToAccountIdMap.Contains(LocalUserNum))
	{
		return UserNumToAccountIdMap[LocalUserNum];
	}
	return nullptr;
}

EOS_EpicAccountId FUserManagerEOS::GetLocalEpicAccountId() const
{
	return GetLocalEpicAccountId(DefaultLocalUser);
}

EOS_ProductUserId FUserManagerEOS::GetLocalProductUserId(int32 LocalUserNum) const
{
	if (UserNumToProductUserIdMap.Contains(LocalUserNum))
	{
		return UserNumToProductUserIdMap[LocalUserNum];
	}
	return nullptr;
}

EOS_ProductUserId FUserManagerEOS::GetLocalProductUserId() const
{
	return GetLocalProductUserId(DefaultLocalUser);
}

EOS_EpicAccountId FUserManagerEOS::GetLocalEpicAccountId(EOS_ProductUserId UserId) const
{
	if (ProductUserIdToUserNumMap.Contains(UserId))
	{
		return GetLocalEpicAccountId(ProductUserIdToUserNumMap[UserId]);
	}
	return nullptr;
}

EOS_ProductUserId FUserManagerEOS::GetLocalProductUserId(EOS_EpicAccountId AccountId) const
{
	if (AccountIdToUserNumMap.Contains(AccountId))
	{
		return GetLocalProductUserId(AccountIdToUserNumMap[AccountId]);
	}
	return nullptr;
}

typedef TEOSCallback<EOS_Connect_OnQueryProductUserIdMappingsCallback, EOS_Connect_QueryProductUserIdMappingsCallbackInfo, FUserManagerEOS> FConnectQueryProductUserIdMappingsCallback;

/**
 * Uses the Connect API to retrieve the EOS_EpicAccountId for a given EOS_ProductUserId
 *
 * @param ProductUserId the product user id we want to query
 * @Param OutEpicAccountId the epic account id we will assign if the query is successful
 *
 * @return true if the operation was successful, false otherwise
 */
bool FUserManagerEOS::GetEpicAccountIdFromProductUserId(const EOS_ProductUserId& ProductUserId, EOS_EpicAccountId& OutEpicAccountId) const
{
	bool bResult = false;

	char EpicIdStr[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH+1];
	int32 EpicIdStrSize = sizeof(EpicIdStr);

	EOS_Connect_GetProductUserIdMappingOptions Options = { };
	Options.ApiVersion = EOS_CONNECT_GETPRODUCTUSERIDMAPPING_API_LATEST;
	Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
	Options.LocalUserId = GetLocalProductUserId();
	Options.TargetProductUserId = ProductUserId;

	EOS_EResult Result = EOS_Connect_GetProductUserIdMapping(EOSSubsystem->ConnectHandle, &Options, EpicIdStr, &EpicIdStrSize);
	if (Result == EOS_EResult::EOS_Success)
	{
		OutEpicAccountId = EOS_EpicAccountId_FromString(EpicIdStr);
		bResult = true;
	}
	else
	{
		UE_LOG_ONLINE(Verbose, TEXT("[FUserManagerEOS::GetEpicAccountIdFromProductUserId] EOS_Connect_GetProductUserIdMapping not successful for ProductUserId (%s). Finished with EOS_EResult %s"), *LexToString(ProductUserId), ANSI_TO_TCHAR(EOS_EResult_ToString(Result)));
	}

	return bResult;
}

void FUserManagerEOS::ResolveUniqueNetId(const EOS_ProductUserId& ProductUserId, const FResolveUniqueNetIdCallback& Callback) const
{
	TArray<EOS_ProductUserId> ProductUserIds = { const_cast<EOS_ProductUserId>(ProductUserId) };

	FResolveUniqueNetIdsCallback GroupCallback = [ProductUserId, OriginalCallback = Callback](TMap<EOS_ProductUserId, FUniqueNetIdEOSRef> ResolvedUniqueNetIds) {
		OriginalCallback(ResolvedUniqueNetIds[ProductUserId]);
	};

	ResolveUniqueNetIds(ProductUserIds, GroupCallback);
}

void FUserManagerEOS::ResolveUniqueNetIds(const TArray<EOS_ProductUserId>& ProductUserIds, const FResolveUniqueNetIdsCallback& Callback) const
{
	TMap<EOS_ProductUserId, FUniqueNetIdEOSRef> ResolvedUniqueNetIds;
	TArray<EOS_ProductUserId> ProductUserIdsToResolve;

	for (const EOS_ProductUserId& ProductUserId : ProductUserIds)
	{
		EOS_EpicAccountId EpicAccountId;

		// We check first if the Product User Id has already been queried, which would allow us to retrieve its Epic Account Id directly
		if (GetEpicAccountIdFromProductUserId(ProductUserId, EpicAccountId))
		{
			const FUniqueNetIdEOSRef UniqueNetId = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, ProductUserId).ToSharedRef();

			ResolvedUniqueNetIds.Add(ProductUserId, UniqueNetId);
		}
		else
		{
			// If that's not the case, we'll have to query them first
			ProductUserIdsToResolve.Add(ProductUserId);
		}
	}

	if (!ProductUserIdsToResolve.IsEmpty())
	{
		EOS_Connect_QueryProductUserIdMappingsOptions QueryProductUserIdMappingsOptions = {};
		QueryProductUserIdMappingsOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
		QueryProductUserIdMappingsOptions.LocalUserId = EOSSubsystem->UserManager->GetLocalProductUserId(0);
		QueryProductUserIdMappingsOptions.ProductUserIds = ProductUserIdsToResolve.GetData();
		QueryProductUserIdMappingsOptions.ProductUserIdCount = ProductUserIdsToResolve.Num();

		FConnectQueryProductUserIdMappingsCallback* CallbackObj = new FConnectQueryProductUserIdMappingsCallback(FUserManagerEOSConstWeakPtr(AsShared()));
		CallbackObj->CallbackLambda = [this, ProductUserIdsToResolve, ResolvedUniqueNetIds = MoveTemp(ResolvedUniqueNetIds), Callback](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data) mutable
		{
			if (Data->ResultCode != EOS_EResult::EOS_Success)
			{
				UE_LOG_ONLINE(Verbose, TEXT("[FUserManagerEOS::ResolveUniqueNetIds] EOS_Connect_QueryProductUserIdMappings not successful for user (%s). Finished with EOS_EResult %s."), *LexToString(Data->LocalUserId), ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
			}

			for (const EOS_ProductUserId& ProductUserId : ProductUserIdsToResolve)
			{
				EOS_EpicAccountId EpicAccountId = nullptr;

				GetEpicAccountIdFromProductUserId(ProductUserId, EpicAccountId);

				const FUniqueNetIdEOSRef UniqueNetId = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, ProductUserId).ToSharedRef();

				ResolvedUniqueNetIds.Add(ProductUserId, UniqueNetId);
			}

			Callback(ResolvedUniqueNetIds);
		};

		EOS_Connect_QueryProductUserIdMappings(EOSSubsystem->ConnectHandle, &QueryProductUserIdMappingsOptions, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	else
	{
		Callback(ResolvedUniqueNetIds);
	}
}

FOnlineUserPtr FUserManagerEOS::GetLocalOnlineUser(int32 LocalUserNum) const
{
	FOnlineUserPtr OnlineUser;
	if (UserNumToNetIdMap.Contains(LocalUserNum))
	{
		const FUniqueNetIdEOSPtr NetId = UserNumToNetIdMap.FindRef(LocalUserNum);
		if (NetIdStringToOnlineUserMap.Contains(NetId->ToString()))
		{
			OnlineUser = NetIdStringToOnlineUserMap.FindRef(NetId->ToString());
		}
	}
	return OnlineUser;
}

FOnlineUserPtr FUserManagerEOS::GetOnlineUser(EOS_ProductUserId UserId) const
{
	FOnlineUserPtr OnlineUser;
	if (ProductUserIdToStringMap.Contains(UserId))
	{
		const FString& NetId = ProductUserIdToStringMap.FindRef(UserId);
		if (NetIdStringToOnlineUserMap.Contains(NetId))
		{
			OnlineUser = NetIdStringToOnlineUserMap.FindRef(*NetId);
		}
	}
	return OnlineUser;
}

FOnlineUserPtr FUserManagerEOS::GetOnlineUser(EOS_EpicAccountId AccountId) const
{
	FOnlineUserPtr OnlineUser;
	if (AccountIdToStringMap.Contains(AccountId))
	{
		const FString& NetId = AccountIdToStringMap.FindRef(AccountId);
		if (NetIdStringToOnlineUserMap.Contains(NetId))
		{
			OnlineUser = NetIdStringToOnlineUserMap.FindRef(NetId);
		}
	}
	return OnlineUser;
}

void FUserManagerEOS::RemoveLocalUser(int32 LocalUserNum)
{
	const FUniqueNetIdEOSPtr* FoundId = UserNumToNetIdMap.Find(LocalUserNum);
	if (FoundId != nullptr)
	{
		EOSSubsystem->ReleaseVoiceChatUserInterface(**FoundId);
		LocalUserNumToFriendsListMap.Remove(LocalUserNum);
		const FString& NetId = (*FoundId)->ToString();
		const EOS_EpicAccountId AccountId = (*FoundId)->GetEpicAccountId();
		AccountIdToStringMap.Remove(AccountId);
		AccountIdToUserNumMap.Remove(AccountId);
		NetIdStringToOnlineUserMap.Remove(NetId);
		StringToUserAccountMap.Remove(NetId);
		UserNumToNetIdMap.Remove(LocalUserNum);
		UserNumToAccountIdMap.Remove(LocalUserNum);
		EOS_ProductUserId UserId = UserNumToProductUserIdMap[LocalUserNum];
		ProductUserIdToUserNumMap.Remove(UserId);
		ProductUserIdToStringMap.Remove(UserId);
		UserNumToProductUserIdMap.Remove(LocalUserNum);
	}
	// Reset this for the next user login
	if (LocalUserNum == DefaultLocalUser)
	{
		DefaultLocalUser = -1;
	}
}

FUniqueNetIdPtr FUserManagerEOS::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	// If we're passed an EOSPlus id, the first EOS_ID_BYTE_SIZE bytes are the EAS|EOS part we care about.
	Size = FMath::Min(Size, EOS_ID_BYTE_SIZE);
	return FUniqueNetIdEOSRegistry::FindOrAdd(Bytes, Size);
}

FUniqueNetIdPtr FUserManagerEOS::CreateUniquePlayerId(const FString& InStr)
{
	FString NetIdStr = InStr;
	// If we're passed an EOSPlus id, remove the platform id and separator.
	NetIdStr.Split(EOSPLUS_ID_SEPARATOR, nullptr, &NetIdStr);
	return FUniqueNetIdEOSRegistry::FindOrAdd(NetIdStr);
}

ELoginStatus::Type FUserManagerEOS::GetLoginStatus(int32 LocalUserNum) const
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FUserManagerEOS::GetLoginStatus(const FUniqueNetIdEOS& UserId) const
{
	FEOSSettings Settings = UEOSSettings::GetSettings();
	// If the user isn't using EAS, then only check for a product user id
	if (!Settings.bUseEAS)
	{
		const EOS_ProductUserId ProductUserId = UserId.GetProductUserId();
		if (ProductUserId != nullptr)
		{
			return ELoginStatus::LoggedIn;
		}
		return ELoginStatus::NotLoggedIn;
	}

	const EOS_EpicAccountId AccountId = UserId.GetEpicAccountId();
	if (AccountId == nullptr)
	{
		return ELoginStatus::NotLoggedIn;
	}

	EOS_ELoginStatus LoginStatus = EOS_Auth_GetLoginStatus(EOSSubsystem->AuthHandle, AccountId);
	switch (LoginStatus)
	{
		case EOS_ELoginStatus::EOS_LS_LoggedIn:
		{
			return ELoginStatus::LoggedIn;
		}
		case EOS_ELoginStatus::EOS_LS_UsingLocalProfile:
		{
			return ELoginStatus::UsingLocalProfile;
		}
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FUserManagerEOS::GetLoginStatus(const FUniqueNetId& UserId) const
{
	const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(UserId);
	return GetLoginStatus(EosId);
}

FString FUserManagerEOS::GetPlayerNickname(int32 LocalUserNum) const
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetDisplayName();
		}
	}
	return FString();
}

FString FUserManagerEOS::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid())
	{
		return UserAccount->GetDisplayName();
	}
	return FString();
}

FString FUserManagerEOS::GetAuthToken(int32 LocalUserNum) const
{
	FUniqueNetIdPtr UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FUserManagerEOS::RevokeAuthToken(const FUniqueNetId& LocalUserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(LocalUserId, FOnlineError(EOnlineErrorResult::NotImplemented));
}

FPlatformUserId FUserManagerEOS::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	return GetPlatformUserIdFromLocalUserNum(GetLocalUserNumFromUniqueNetId(UniqueNetId));
}

void FUserManagerEOS::GetLinkedAccountAuthToken(int32 LocalUserNum, const FOnGetLinkedAccountAuthTokenCompleteDelegate& Delegate) const
{
	FExternalAuthToken ExternalToken;
	ExternalToken.TokenString = GetAuthToken(LocalUserNum);
	Delegate.ExecuteIfBound(LocalUserNum, ExternalToken.IsValid(), ExternalToken);
}

void FUserManagerEOS::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FString FUserManagerEOS::GetAuthType() const
{
	return TEXT("epic");
}

// IOnlineExternalUI Interface

bool FUserManagerEOS::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	FPlatformEOSHelpersPtr EOSHelpers = EOSSubsystem->GetEOSHelpers();
	EOSHelpers->PlatformTriggerLoginUI(EOSSubsystem, ControllerIndex, bShowOnlineOnly, bShowSkipButton, Delegate);

	return true;
}

bool FUserManagerEOS::ShowAccountCreationUI(const int ControllerIndex, const FOnAccountCreationUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowAccountCreationUI] This method is not implemented."));
	
	EOSSubsystem->ExecuteNextTick([ControllerIndex, Delegate]()
		{
			Delegate.ExecuteIfBound(ControllerIndex, FOnlineAccountCredentials(), FOnlineError(EOnlineErrorResult::NotImplemented));
		});

	return true;
}

typedef TEOSCallback<EOS_UI_OnShowFriendsCallback, EOS_UI_ShowFriendsCallbackInfo, FUserManagerEOS> FOnShowFriendsCallback;

bool FUserManagerEOS::ShowFriendsUI(int32 LocalUserNum)
{
	EOS_UI_ShowFriendsOptions Options = {};
	Options.ApiVersion = EOS_UI_SHOWFRIENDS_API_LATEST;
	Options.LocalUserId = GetLocalEpicAccountId(LocalUserNum);

	FOnShowFriendsCallback* CallbackObj = new FOnShowFriendsCallback(AsWeak());
	CallbackObj->CallbackLambda = [](const EOS_UI_ShowFriendsCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			UE_LOG_ONLINE_EXTERNALUI(VeryVerbose, TEXT("[FUserManagerEOS::ShowFriendsUI] EOS_UI_ShowFriends was successful."));
		}
		else
		{
			UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowFriendsUI] EOS_UI_ShowFriends was not successful. Finished with error %s"), ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
		}
	};

	EOS_UI_ShowFriends(EOSSubsystem->UIHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}


bool FUserManagerEOS::ShowInviteUI(int32 LocalUserNum, FName SessionName)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowInviteUI] This method is not implemented."));

	return false;
}

bool FUserManagerEOS::ShowAchievementsUI(int32 LocalUserNum)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowAchievementsUI] This method is not implemented."));

	return false;
}

bool FUserManagerEOS::ShowLeaderboardUI(const FString& LeaderboardName)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowLeaderboardUI] This method is not implemented."));

	return false;
}

bool FUserManagerEOS::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowWebURL] This method is not implemented."));

	if (Delegate.IsBound())
	{
		EOSSubsystem->ExecuteNextTick([Delegate]()
			{
				Delegate.ExecuteIfBound(FString());
			});
		return true;
	}
	return false;
}

bool FUserManagerEOS::CloseWebURL()
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::CloseWebURL] This method is not implemented."));

	return false;
}

bool FUserManagerEOS::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowProfileUI] This method is not implemented."));

	if (Delegate.IsBound())
	{
		EOSSubsystem->ExecuteNextTick([Delegate]()
			{
				Delegate.ExecuteIfBound();
			});
		return true;
	}
	return false;
}

bool FUserManagerEOS::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowAccountUpgradeUI] This method is not implemented."));

	return false;
}

bool FUserManagerEOS::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowStoreUI] This method is not implemented."));

	if (Delegate.IsBound())
	{
		EOSSubsystem->ExecuteNextTick([Delegate]()
			{
				Delegate.ExecuteIfBound(false);
			});
		return true;
	}
	return false;
}

bool FUserManagerEOS::ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("[FUserManagerEOS::ShowSendMessageUI] This method is not implemented."));

	if (Delegate.IsBound())
	{
		EOSSubsystem->ExecuteNextTick([Delegate]()
			{
				Delegate.ExecuteIfBound(false);
			});
		return true;
	}
	return false;
}

// ~IOnlineExternalUI Interface

typedef TEOSCallback<EOS_Friends_OnQueryFriendsCallback, EOS_Friends_QueryFriendsCallbackInfo, FUserManagerEOS> FReadFriendsCallback;

void FUserManagerEOS::FriendStatusChanged(const EOS_Friends_OnFriendsUpdateInfo* Data)
{
	// This seems to happen due to the SDK's local cache going from empty to filled, so ignore it
	// It's not really a valid transition since there should have been a pending invite inbetween
	if (Data->PreviousStatus == EOS_EFriendsStatus::EOS_FS_NotFriends && Data->CurrentStatus == EOS_EFriendsStatus::EOS_FS_Friends)
	{
		return;
	}

	// Get the local user information
	if (AccountIdToUserNumMap.Contains(Data->LocalUserId))
	{
		int32 LocalUserNum = AccountIdToUserNumMap[Data->LocalUserId];
		FUniqueNetIdEOSPtr LocalEOSID = UserNumToNetIdMap[LocalUserNum];
		// If we don't know them yet, then add them to kick off the reads
		if (!AccountIdToStringMap.Contains(Data->TargetUserId))
		{
			AddFriend(LocalUserNum, Data->TargetUserId);
		}
		// They are in our list now
		FOnlineUserPtr OnlineUser = EpicAccountIdToOnlineUserMap[Data->TargetUserId];
		FOnlineFriendEOSPtr Friend = LocalUserNumToFriendsListMap[LocalUserNum]->GetByNetIdString(AccountIdToStringMap[Data->TargetUserId]);
		// Figure out which notification to fire
		if (Data->CurrentStatus == EOS_EFriendsStatus::EOS_FS_Friends)
		{
			Friend->SetInviteStatus(EInviteStatus::Accepted);
			TriggerOnInviteAcceptedDelegates(*LocalEOSID, *OnlineUser->GetUserId());
		}
		else if (Data->PreviousStatus == EOS_EFriendsStatus::EOS_FS_Friends && Data->CurrentStatus == EOS_EFriendsStatus::EOS_FS_NotFriends)
		{
			LocalUserNumToFriendsListMap[LocalUserNum]->Remove(AccountIdToStringMap[Data->TargetUserId], Friend.ToSharedRef());
			Friend->SetInviteStatus(EInviteStatus::Unknown);
			TriggerOnFriendRemovedDelegates(*LocalEOSID, *OnlineUser->GetUserId());
		}
		else if (Data->PreviousStatus < EOS_EFriendsStatus::EOS_FS_Friends && Data->CurrentStatus == EOS_EFriendsStatus::EOS_FS_NotFriends)
		{
			LocalUserNumToFriendsListMap[LocalUserNum]->Remove(AccountIdToStringMap[Data->TargetUserId], Friend.ToSharedRef());
			Friend->SetInviteStatus(EInviteStatus::Unknown);
			TriggerOnInviteRejectedDelegates(*LocalEOSID, *OnlineUser->GetUserId());
		}
		else if (Data->CurrentStatus == EOS_EFriendsStatus::EOS_FS_InviteReceived)
		{
			Friend->SetInviteStatus(EInviteStatus::PendingInbound);
			TriggerOnInviteReceivedDelegates(*LocalEOSID, *OnlineUser->GetUserId());
		}
		TriggerOnFriendsChangeDelegates(LocalUserNum);
	}
}

void FUserManagerEOS::AddFriend(int32 LocalUserNum, EOS_EpicAccountId EpicAccountId)
{
	FUniqueNetIdEOSRef FriendNetId = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, nullptr).ToSharedRef();
	const FString NetId = FriendNetId->ToString();
	FOnlineFriendEOSRef FriendRef = MakeShareable(new FOnlineFriendEOS(FriendNetId));
	LocalUserNumToFriendsListMap[LocalUserNum]->Add(NetId, FriendRef);

	EOS_Friends_GetStatusOptions Options = { };
	Options.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];
	Options.TargetUserId = EpicAccountId;
	EOS_EFriendsStatus Status = EOS_Friends_GetStatus(EOSSubsystem->FriendsHandle, &Options);
	
	FriendRef->SetInviteStatus(ToEInviteStatus(Status));

	// Add this friend as a remote player (this will grab user info)
	AddRemotePlayer(LocalUserNum, NetId, EpicAccountId, FriendNetId, FriendRef, FriendRef);

	// Querying the presence of a non-friend would cause an SDK error.
	// Players that sent/recieved a friend invitation from us still count as "friends", so check
	// our friend relationship here.
	if(Status == EOS_EFriendsStatus::EOS_FS_Friends)
	{
		QueryPresence(*FriendNetId, IgnoredPresenceDelegate);
	}
}

void FUserManagerEOS::AddRemotePlayer(int32 LocalUserNum, const FString& NetId, EOS_EpicAccountId EpicAccountId)
{
	FUniqueNetIdEOSRef EOSID = FUniqueNetIdEOSRegistry::FindOrAdd(NetId).ToSharedRef();
	FOnlineUserEOSRef UserRef = MakeShareable(new FOnlineUserEOS(EOSID));
	// Add this user as a remote (this will grab presence & user info)
	AddRemotePlayer(LocalUserNum, NetId, EpicAccountId, EOSID, UserRef, UserRef);
}

void FUserManagerEOS::AddRemotePlayer(int32 LocalUserNum, const FString& NetId, EOS_EpicAccountId EpicAccountId, FUniqueNetIdEOSPtr UniqueNetId, FOnlineUserPtr OnlineUser, IAttributeAccessInterfaceRef AttributeRef)
{
	NetIdStringToOnlineUserMap.Emplace(NetId, OnlineUser);
	EpicAccountIdToOnlineUserMap.Emplace(EpicAccountId, OnlineUser);
	NetIdStringToAttributeAccessMap.Emplace(NetId, AttributeRef);
	EpicAccountIdToAttributeAccessMap.Emplace(EpicAccountId, AttributeRef);

	AccountIdToStringMap.Emplace(EpicAccountId, NetId);

	// Read the user info for this player
	ReadUserInfo(LocalUserNum, EpicAccountId);
}

void FUserManagerEOS::UpdateRemotePlayerProductUserId(EOS_EpicAccountId EpicAccountId, EOS_ProductUserId ProductUserId)
{
	// See if the net ids have changed for this user and bail if they are the same
	const FString PrevNetIdStr = AccountIdToStringMap[EpicAccountId];
	// Note that calling FindOrAdd with previously invalid EAS/EOS components will update the net id in place.
	const FString NewNetIdStr = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, ProductUserId)->ToString();
	if (PrevNetIdStr == NewNetIdStr)
	{
		// No change, so skip any work
		return;
	}

	const FString ProductUserIdStr = LexToString(ProductUserId);

	// Get the unique net id and rebuild the string for it
	IAttributeAccessInterfaceRef AttrAccess = NetIdStringToAttributeAccessMap[PrevNetIdStr];
	FUniqueNetIdEOSPtr NetIdEOS = AttrAccess->GetUniqueNetIdEOS();
	// Update any old friends entries with the new net id key
	for (TMap<int32, FFriendsListEOSRef>::TConstIterator It(LocalUserNumToFriendsListMap); It; ++It)
	{
		FFriendsListEOSRef FriendsList = It.Value();
		FOnlineFriendEOSPtr FoundFriend = FriendsList->GetByNetIdString(PrevNetIdStr);
		if (FoundFriend.IsValid())
		{
			FriendsList->UpdateNetIdStr(PrevNetIdStr, NewNetIdStr);
		}
	}
	// Update all of the other net id to X mappings
	AccountIdToStringMap.Remove(EpicAccountId);
	AccountIdToStringMap.Emplace(EpicAccountId, NewNetIdStr);
	ProductUserIdToStringMap.Remove(ProductUserId);
	ProductUserIdToStringMap.Emplace(ProductUserId, *NewNetIdStr);
	FOnlineUserPtr OnlineUser = NetIdStringToOnlineUserMap[PrevNetIdStr];
	NetIdStringToOnlineUserMap.Remove(PrevNetIdStr);
	NetIdStringToOnlineUserMap.Emplace(NewNetIdStr, OnlineUser);
	NetIdStringToAttributeAccessMap.Remove(PrevNetIdStr);
	NetIdStringToAttributeAccessMap.Emplace(NewNetIdStr, AttrAccess);
	// Presence may not be available for all online users
	if (NetIdStringToOnlineUserPresenceMap.Contains(PrevNetIdStr))
	{
		FOnlineUserPresenceRef UserPresence = NetIdStringToOnlineUserPresenceMap[PrevNetIdStr];
		NetIdStringToOnlineUserPresenceMap.Remove(PrevNetIdStr);
		NetIdStringToOnlineUserPresenceMap.Emplace(NewNetIdStr, UserPresence);
	}
}

// IOnlineFriends Interface

bool FUserManagerEOS::ReadFriendsList(int32 LocalUserNum, const FString& ListName, const FOnReadFriendsListComplete& Delegate)
{
	if (!UserNumToNetIdMap.Contains(LocalUserNum))
	{
		const FString ErrorStr = FString::Printf(TEXT("Can't ReadFriendsList() for user (%d) since they are not logged in"), LocalUserNum);
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("%s"), *ErrorStr);
		Delegate.ExecuteIfBound(LocalUserNum, false, ListName, ErrorStr);
		return false;
	}

	// We save the information for this call even if it won't be automatically processed
	const bool bIsReadFriendsListOngoing = CachedReadUserListInfoForLocalUserMap.Contains(LocalUserNum);
	if (!bIsReadFriendsListOngoing)
	{
		CachedReadUserListInfoForLocalUserMap.Emplace(LocalUserNum);
	}
	CachedReadUserListInfoForLocalUserMap[LocalUserNum].Add(ReadUserListInfo(LocalUserNum, ListName, Delegate));

	if (bIsReadFriendsListOngoing)
	{
		UE_LOG_ONLINE_FRIEND(Verbose, TEXT("A ReadFriendsList() operation for user (%d) is already running, we'll save its information and launch it automatically later."), LocalUserNum);
		return true;
	}

	EOS_Friends_QueryFriendsOptions Options = { };
	Options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];

	FReadFriendsCallback* CallbackObj = new FReadFriendsCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, LocalUserNum, ListName, Delegate](const EOS_Friends_QueryFriendsCallbackInfo* Data)
	{
		EOS_EResult Result = Data->ResultCode;
		if (GetLoginStatus(LocalUserNum) != ELoginStatus::LoggedIn)
		{
			// Handle the user logging out while a read is in progress
			Result = EOS_EResult::EOS_InvalidUser;
		}

		bool bWasSuccessful = Result == EOS_EResult::EOS_Success;
		if (bWasSuccessful)
		{
			EOS_Friends_GetFriendsCountOptions Options = { };
			Options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
			Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];
			int32 FriendCount = EOS_Friends_GetFriendsCount(EOSSubsystem->FriendsHandle, &Options);

			LocalUserNumToFriendsListMap[LocalUserNum]->Empty(FriendCount);

			TArray<FString> FriendEasIds;
			FriendEasIds.Reserve(FriendCount);
			// Process each friend returned
			for (int32 Index = 0; Index < FriendCount; Index++)
			{
				EOS_Friends_GetFriendAtIndexOptions FriendIndexOptions = { };
				FriendIndexOptions.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
				FriendIndexOptions.Index = Index;
				FriendIndexOptions.LocalUserId = Options.LocalUserId;
				EOS_EpicAccountId FriendEpicAccountId = EOS_Friends_GetFriendAtIndex(EOSSubsystem->FriendsHandle, &FriendIndexOptions);
				if (FriendEpicAccountId != nullptr)
				{
					AddFriend(LocalUserNum, FriendEpicAccountId);
					FriendEasIds.Add(LexToString(FriendEpicAccountId));
				}
			}

			const TFunction<FOnQueryExternalIdMappingsComplete::TFuncType>& OnExternalIdMappingsQueriedLambda =
				[this, WeakThis = AsWeak(), LocalUserNum](bool bWasSuccessful, const FUniqueNetId& UserId, const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, const FString& Error)
			{
				if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
				{
					ProcessReadFriendsListComplete(LocalUserNum, bWasSuccessful, Error);
				}
			};

			const auto& ExternalMappingsCallback = OSSInternalCallback::Create<FOnQueryExternalIdMappingsComplete>(EOSSubsystem->UserManager, OnExternalIdMappingsQueriedLambda);

			QueryExternalIdMappings(*GetLocalUniqueNetIdEOS(DefaultLocalUser), FExternalIdQueryOptions(), FriendEasIds, ExternalMappingsCallback);

		}
		else
		{
			const FString ErrorString = FString::Printf(TEXT("ReadFriendsList(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Result)));
			ProcessReadFriendsListComplete(LocalUserNum, false, ErrorString);
		}
	};
	EOS_Friends_QueryFriends(EOSSubsystem->FriendsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}

void FUserManagerEOS::ProcessReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ErrorStr)
{
	// If we started any user info queries for friends, we'll just wait until they finish
	if (!IsFriendQueryUserInfoOngoing(LocalUserNum))
	{
		// If not, we'll just trigger the delegates for all cached calls
		TArray<ReadUserListInfo> CachedInfoList;
		if (CachedReadUserListInfoForLocalUserMap.RemoveAndCopyValue(LocalUserNum, CachedInfoList))
		{
			for (const ReadUserListInfo& CachedInfo : CachedInfoList)
			{
				CachedInfo.ExecuteDelegateIfBound(bWasSuccessful, ErrorStr);
			}
		}

		TriggerOnFriendsChangeDelegates(LocalUserNum);

		IsFriendQueryUserInfoOngoingForLocalUserMap.Remove(LocalUserNum);
		IsPlayerQueryExternalMappingsOngoingForLocalUserMap.Remove(LocalUserNum);
	}
}

void FUserManagerEOS::SetFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FString& Alias, const FOnSetFriendAliasComplete& Delegate)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::SetFriendAlias] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([LocalUserNum, FriendId = FriendId.AsShared(), ListName, Delegate]()
		{
			Delegate.ExecuteIfBound(LocalUserNum, *FriendId, ListName, FOnlineError(EOnlineErrorResult::NotImplemented));
		});
}

void FUserManagerEOS::DeleteFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnDeleteFriendAliasComplete& Delegate)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::DeleteFriendAlias] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([LocalUserNum, FriendId = FriendId.AsShared(), ListName, Delegate]()
		{
			Delegate.ExecuteIfBound(LocalUserNum, *FriendId, ListName, FOnlineError(EOnlineErrorResult::NotImplemented));
		});
}

bool FUserManagerEOS::DeleteFriendsList(int32 LocalUserNum, const FString& ListName, const FOnDeleteFriendsListComplete& Delegate)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::DeleteFriendsList] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([LocalUserNum, ListName, Delegate]()
		{
			Delegate.ExecuteIfBound(LocalUserNum, false, ListName, TEXT("This method is not supported."));
		});

	return true;
}

typedef TEOSCallback<EOS_Friends_OnSendInviteCallback, EOS_Friends_SendInviteCallbackInfo, FUserManagerEOS> FSendInviteCallback;

bool FUserManagerEOS::SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnSendInviteComplete& Delegate)
{
	if (!UserNumToNetIdMap.Contains(LocalUserNum))
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't SendInvite() for user (%d) since they are not logged in"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, FString(TEXT("Can't SendInvite() for user (%d) since they are not logged in"), LocalUserNum));
		return false;
	}

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(FriendId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't SendInvite() for user (%d) since the potential player id is unknown"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, FString(TEXT("Can't SendInvite() for user (%d) since the player id is unknown"), LocalUserNum));
		return false;
	}

	FSendInviteCallback* CallbackObj = new FSendInviteCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, ListName, this, Delegate](const EOS_Friends_SendInviteCallbackInfo* Data)
	{
		const FString& NetId = AccountIdToStringMap[Data->TargetUserId];

		FString ErrorString;
		bool bWasSuccessful = Data->ResultCode == EOS_EResult::EOS_Success;
		if (!bWasSuccessful)
		{
			ErrorString = FString::Printf(TEXT("Failed to send invite for user (%d) to player (%s) with result code (%s)"), LocalUserNum, *NetId, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
		}
		Delegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, *FUniqueNetIdEOSRegistry::FindOrAdd(NetId), ListName, ErrorString);
	};

	EOS_Friends_SendInviteOptions Options = { };
	Options.ApiVersion = EOS_FRIENDS_SENDINVITE_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];
	Options.TargetUserId = AccountId;
	EOS_Friends_SendInvite(EOSSubsystem->FriendsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}

typedef TEOSCallback<EOS_Friends_OnAcceptInviteCallback, EOS_Friends_AcceptInviteCallbackInfo, FUserManagerEOS> FAcceptInviteCallback;

bool FUserManagerEOS::AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnAcceptInviteComplete& Delegate)
{
	if (!UserNumToNetIdMap.Contains(LocalUserNum))
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't AcceptInvite() for user (%d) since they are not logged in"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, FString(TEXT("Can't AcceptInvite() for user (%d) since they are not logged in"), LocalUserNum));
		return false;
	}

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(FriendId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't AcceptInvite() for user (%d) since the friend is not in their list"), LocalUserNum);
		Delegate.ExecuteIfBound(LocalUserNum, false, FriendId, ListName, FString(TEXT("Can't AcceptInvite() for user (%d) since the friend is not in their list"), LocalUserNum));
		return false;
	}

	FAcceptInviteCallback* CallbackObj = new FAcceptInviteCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, ListName, this, Delegate](const EOS_Friends_AcceptInviteCallbackInfo* Data)
	{
		const FString& NetId = AccountIdToStringMap[Data->TargetUserId];

		FString ErrorString;
		bool bWasSuccessful = Data->ResultCode == EOS_EResult::EOS_Success;
		if (!bWasSuccessful)
		{
			ErrorString = FString::Printf(TEXT("Failed to accept invite for user (%d) from friend (%s) with result code (%s)"), LocalUserNum, *NetId, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
		}
		Delegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, *FUniqueNetIdEOSRegistry::FindOrAdd(NetId), ListName, ErrorString);
	};

	EOS_Friends_AcceptInviteOptions Options = { };
	Options.ApiVersion = EOS_FRIENDS_ACCEPTINVITE_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];
	Options.TargetUserId = AccountId;
	EOS_Friends_AcceptInvite(EOSSubsystem->FriendsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	return true;
}

void EOS_CALL EOSRejectInviteCallback(const EOS_Friends_RejectInviteCallbackInfo* Data)
{
	// We don't need to notify anyone so ignore
}

bool FUserManagerEOS::RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	if (!UserNumToNetIdMap.Contains(LocalUserNum))
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't RejectInvite() for user (%d) since they are not logged in"), LocalUserNum);
		return false;
	}

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(FriendId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Can't RejectInvite() for user (%d) since the friend is not in their list"), LocalUserNum);
		return false;
	}

	EOS_Friends_RejectInviteOptions Options{ 0 };
	Options.ApiVersion = EOS_FRIENDS_REJECTINVITE_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[LocalUserNum];
	Options.TargetUserId = AccountId;
	EOS_Friends_RejectInvite(EOSSubsystem->FriendsHandle, &Options, nullptr, &EOSRejectInviteCallback);
	return true;
}

bool FUserManagerEOS::DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::DeleteFriend] Friends may only be deleted via the Epic Games Launcher."));

	EOSSubsystem->ExecuteNextTick([this, WeakThis = AsWeak(), LocalUserNum, FriendId = FriendId.AsShared(), ListName]()
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				TriggerOnDeleteFriendCompleteDelegates(LocalUserNum, false, *FriendId, ListName, TEXT("[FUserManagerEOS::DeleteFriend] Friends may only be deleted via the Epic Games Launcher."));
			}
		});

	return true;
}

bool FUserManagerEOS::GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray<TSharedRef<FOnlineFriend>>& OutFriends)
{
	OutFriends.Reset();
	if (LocalUserNumToFriendsListMap.Contains(LocalUserNum))
	{
		FFriendsListEOSRef FriendsList = LocalUserNumToFriendsListMap[LocalUserNum];
		for (FOnlineFriendEOSRef Friend : FriendsList->GetList())
		{
			const FOnlineUserPresence& Presence = Friend->GetPresence();
			// See if they only want online only
			if (ListName == EFriendsLists::ToString(EFriendsLists::OnlinePlayers) && !Presence.bIsOnline)
			{
				continue;
			}
			// Of if they only want friends playing this game
			else if (ListName == EFriendsLists::ToString(EFriendsLists::InGamePlayers) && !Presence.bIsPlayingThisGame)
			{
				continue;
			}
			// If the service hasn't returned the info yet, skip them
			else if (Friend->GetDisplayName().IsEmpty())
			{
				continue;
			}
			OutFriends.Add(Friend);
		}
		// Sort these by those playing the game first, alphabetically, then not playing, then not online
		OutFriends.Sort([](TSharedRef<FOnlineFriend> A, TSharedRef<FOnlineFriend> B)
		{
			const FOnlineUserPresence& APres = A->GetPresence();
			const FOnlineUserPresence& BPres = B->GetPresence();
			// If they are the same, then check playing this game
			if (APres.bIsOnline == BPres.bIsOnline)
			{
				// If they are the same, then sort by name
				if (APres.bIsPlayingThisGame == BPres.bIsPlayingThisGame)
				{
					const EInviteStatus::Type AFriendStatus = A->GetInviteStatus();
					const EInviteStatus::Type BFriendStatus = B->GetInviteStatus();
					// Sort pending friends below accepted friends
					if (AFriendStatus == BFriendStatus && AFriendStatus == EInviteStatus::Accepted)
					{
						const FString& AName = A->GetDisplayName();
						const FString& BName = B->GetDisplayName();
						return AName < BName;
					}
				}
			}
			return false;
		});
		return true;
	}
	return false;
}

TSharedPtr<FOnlineFriend> FUserManagerEOS::GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	if (LocalUserNumToFriendsListMap.Contains(LocalUserNum))
	{
		FFriendsListEOSRef FriendsList = LocalUserNumToFriendsListMap[LocalUserNum];
		const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(FriendId);
		FOnlineFriendEOSPtr FoundFriend = FriendsList->GetByNetIdString(EosId.ToString());
		if (FoundFriend.IsValid())
		{
			const FOnlineUserPresence& Presence = FoundFriend->GetPresence();
			// See if they only want online only
			if (ListName == EFriendsLists::ToString(EFriendsLists::OnlinePlayers) && !Presence.bIsOnline)
			{
				return TSharedPtr<FOnlineFriend>();
			}
			// Of if they only want friends playing this game
			else if (ListName == EFriendsLists::ToString(EFriendsLists::InGamePlayers) && !Presence.bIsPlayingThisGame)
			{
				return TSharedPtr<FOnlineFriend>();
			}
			return FoundFriend;
		}
	}
	return TSharedPtr<FOnlineFriend>();
}

bool FUserManagerEOS::IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	return GetFriend(LocalUserNum, FriendId, ListName).IsValid();
}

bool FUserManagerEOS::QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::QueryRecentPlayers] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([this, WeakThis = AsWeak(), UserId = UserId.AsShared(), Namespace]()
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				TriggerOnQueryRecentPlayersCompleteDelegates(*UserId, Namespace, false, TEXT("This method is not supported."));
			}
		});

	return true;
}

bool FUserManagerEOS::GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace, TArray<TSharedRef<FOnlineRecentPlayer>>& OutRecentPlayers)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::GetRecentPlayers] This method is not supported."));

	return false;
}

bool FUserManagerEOS::BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::BlockPlayer] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([this, WeakThis = AsWeak(), LocalUserNum, PlayerId = PlayerId.AsShared()]()
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				TriggerOnBlockedPlayerCompleteDelegates(LocalUserNum, false, *PlayerId, TEXT(""), TEXT("This method is not supported"));
			}
		});

	return true;
}

bool FUserManagerEOS::UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::UnblockPlayer] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([this, WeakThis = AsWeak(), LocalUserNum, PlayerId = PlayerId.AsShared()]()
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				TriggerOnUnblockedPlayerCompleteDelegates(LocalUserNum, false, *PlayerId, TEXT(""), TEXT("This method is not supported"));
			}
		});

	return true;
}

bool FUserManagerEOS::QueryBlockedPlayers(const FUniqueNetId& UserId)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::QueryBlockedPlayers] This method is not supported."));

	EOSSubsystem->ExecuteNextTick([this, WeakThis = AsWeak(), UserId = UserId.AsShared()]()
		{
			if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
			{
				TriggerOnQueryBlockedPlayersCompleteDelegates(*UserId, false, TEXT("This method is not supported"));
			}
		});

	return true;
}

bool FUserManagerEOS::GetBlockedPlayers(const FUniqueNetId& UserId, TArray<TSharedRef<FOnlineBlockedPlayer>>& OutBlockedPlayers)
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::GetBlockedPlayers] This method is not supported."));

	return false;
}

void FUserManagerEOS::DumpBlockedPlayers() const
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::DumpBlockedPlayers] This method is not supported."));
}

void FUserManagerEOS::DumpRecentPlayers() const
{
	UE_LOG_ONLINE_FRIEND(Warning, TEXT("[FUserManagerEOS::DumpRecentPlayers] This method is not supported."));
}

bool FUserManagerEOS::HandleFriendsExec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if !UE_BUILD_SHIPPING

	bool bWasHandled = true;
	if (FParse::Command(&Cmd, TEXT("ReadFriendsList"))) /* ONLINE (EOS if using EOSPlus) FRIENDS ReadFriendsList 0 default/onlinePlayers/inGamePlayers/inGameAndSessionPlayers */
	{
		int LocalUserNum = FCString::Atoi(*FParse::Token(Cmd, false));

		FString FriendsList = FParse::Token(Cmd, false);

		ReadFriendsList(LocalUserNum, FriendsList, FOnReadFriendsListComplete());
	}
	else if (FParse::Command(&Cmd, TEXT("GetFriendsList"))) /* ONLINE (EOS if using EOSPlus) FRIENDS GetFriendsList 0 default/onlinePlayers/inGamePlayers/inGameAndSessionPlayers */
	{
		int LocalUserNum = FCString::Atoi(*FParse::Token(Cmd, false));

		FString FriendsList = FParse::Token(Cmd, false);

		TArray< TSharedRef<FOnlineFriend> > Friends;
		// Grab the friends data so we can print it out
		if (GetFriendsList(LocalUserNum, FriendsList, Friends))
		{
			UE_LOG_ONLINE_FRIEND(Log, TEXT("FUserManagerEOS::GetFriendsList returned %d friends"), Friends.Num());

			// Log each friend's data out
			for (int32 Index = 0; Index < Friends.Num(); Index++)
			{
				const FOnlineFriend& Friend = *Friends[Index];
				const FOnlineUserPresence& Presence = Friend.GetPresence();
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t%s has unique id (%s)"), *Friend.GetDisplayName(), *Friend.GetUserId()->ToDebugString());
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t Invite status (%s)"), EInviteStatus::ToString(Friend.GetInviteStatus()));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t Presence: %s"), *Presence.Status.StatusStr);
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t State: %s"), EOnlinePresenceState::ToString(Presence.Status.State));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsOnline (%s)"), Presence.bIsOnline ? TEXT("true") : TEXT("false"));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsPlaying (%s)"), Presence.bIsPlaying ? TEXT("true") : TEXT("false"));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsPlayingThisGame (%s)"), Presence.bIsPlayingThisGame ? TEXT("true") : TEXT("false"));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsJoinable (%s)"), Presence.bIsJoinable ? TEXT("true") : TEXT("false"));
				UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bHasVoiceSupport (%s)"), Presence.bHasVoiceSupport ? TEXT("true") : TEXT("false"));
			}
		}
	}
	else if (FParse::Command(&Cmd, TEXT("GetFriend"))) /* ONLINE (EOS if using EOSPlus) FRIENDS GetFriend 0 "FriendUserId|FullStr" default/onlinePlayers/inGamePlayers/inGameAndSessionPlayers */
	{
		int LocalUserNum = FCString::Atoi(*FParse::Token(Cmd, false));

		FString FriendUserIdStr = FParse::Token(Cmd, false);
		FUniqueNetIdEOSRef FriendEosId = FUniqueNetIdEOSRegistry::FindOrAdd(FriendUserIdStr).ToSharedRef();

		FString FriendsList = FParse::Token(Cmd, false);

		TSharedPtr<FOnlineFriend> Friend = GetFriend(LocalUserNum, *FriendEosId, FriendsList);
		if (Friend.IsValid())
		{
			const FOnlineUserPresence& Presence = Friend->GetPresence();
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t%s has unique id (%s)"), *Friend->GetDisplayName(), *Friend->GetUserId()->ToDebugString());
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t Invite status (%s)"), EInviteStatus::ToString(Friend->GetInviteStatus()));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t Presence: %s"), *Presence.Status.StatusStr);
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t State: %s"), EOnlinePresenceState::ToString(Presence.Status.State));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsOnline (%s)"), Presence.bIsOnline ? TEXT("true") : TEXT("false"));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsPlaying (%s)"), Presence.bIsPlaying ? TEXT("true") : TEXT("false"));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsPlayingThisGame (%s)"), Presence.bIsPlayingThisGame ? TEXT("true") : TEXT("false"));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bIsJoinable (%s)"), Presence.bIsJoinable ? TEXT("true") : TEXT("false"));
			UE_LOG_ONLINE_FRIEND(Log, TEXT("\t\t bHasVoiceSupport (%s)"), Presence.bHasVoiceSupport ? TEXT("true") : TEXT("false"));
		}
	}
	else if (FParse::Command(&Cmd, TEXT("IsFriend"))) /* ONLINE (EOS if using EOSPlus) FRIENDS IsFriend 0 "FriendUserId|FullStr" default/onlinePlayers/inGamePlayers/inGameAndSessionPlayers */
	{
		int LocalUserNum = FCString::Atoi(*FParse::Token(Cmd, false));

		FString FriendUserIdStr = FParse::Token(Cmd, false);
		FUniqueNetIdEOSRef FriendEosId = FUniqueNetIdEOSRegistry::FindOrAdd(FriendUserIdStr).ToSharedRef();

		FString FriendsList = FParse::Token(Cmd, false);

		bool bIsFriend = IsFriend(LocalUserNum, *FriendEosId, FriendsList);
		UE_LOG_ONLINE_FRIEND(Log, TEXT("UserId=%s bIsFriend=%s"), *FriendUserIdStr, *LexToString(bIsFriend));
	}
	else
	{
		UE_LOG_ONLINE_FRIEND(Warning, TEXT("Unknown FRIENDS command: %s"), *FParse::Token(Cmd, true));
		bWasHandled = false;
	}

	return bWasHandled;
#else
	return false;
#endif // !UE_BUILD_SHIPPING
}

// ~IOnlineFriends Interface

struct FPresenceStrings
{
	FPresenceStrings(const FString& InKey, const FString& InValue)
		: Key(*InKey), Value(*InValue)
	{
	}
	FTCHARToUTF8 Key;
	FTCHARToUTF8 Value;
};

struct FRichTextOptions :
	public EOS_PresenceModification_SetRawRichTextOptions
{
	FRichTextOptions() :
		EOS_PresenceModification_SetRawRichTextOptions()
	{
		ApiVersion = EOS_PRESENCE_SETRAWRICHTEXT_API_LATEST;
		RichText = RichTextAnsi;
	}
	char RichTextAnsi[EOS_PRESENCE_RICH_TEXT_MAX_VALUE_LENGTH];
};

typedef TEOSCallback<EOS_Presence_SetPresenceCompleteCallback, EOS_Presence_SetPresenceCallbackInfo, FUserManagerEOS> FSetPresenceCallback;

void FUserManagerEOS::SetPresence(const FUniqueNetId& UserId, const FOnlineUserPresenceStatus& Status, const FOnPresenceTaskCompleteDelegate& Delegate)
{
	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE(Error, TEXT("Can't SetPresence() for user (%s) since they are not logged in"), *EOSID.ToDebugString());
		return;
	}

	EOS_HPresenceModification ChangeHandle = nullptr;
	EOS_Presence_CreatePresenceModificationOptions Options = { };
	Options.ApiVersion = EOS_PRESENCE_CREATEPRESENCEMODIFICATION_API_LATEST;
	Options.LocalUserId = AccountId;
	EOS_Presence_CreatePresenceModification(EOSSubsystem->PresenceHandle, &Options, &ChangeHandle);
	if (ChangeHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to create a modification handle for setting presence"));
		return;
	}

	EOS_PresenceModification_SetStatusOptions StatusOptions = { };
	StatusOptions.ApiVersion = EOS_PRESENCE_SETSTATUS_API_LATEST;
	StatusOptions.Status = ToEOS_Presence_EStatus(Status.State);
	EOS_EResult SetStatusResult = EOS_PresenceModification_SetStatus(ChangeHandle, &StatusOptions);
	if (SetStatusResult != EOS_EResult::EOS_Success)
	{
		UE_LOG_ONLINE(Error, TEXT("EOS_PresenceModification_SetStatus() failed with result code (%d)"), (int32)SetStatusResult);
	}

	// Convert the status string as the rich text string
	FRichTextOptions TextOptions;
	FCStringAnsi::Strncpy(TextOptions.RichTextAnsi, TCHAR_TO_UTF8(*Status.StatusStr), EOS_PRESENCE_RICH_TEXT_MAX_VALUE_LENGTH);
	EOS_EResult SetRichTextResult = EOS_PresenceModification_SetRawRichText(ChangeHandle, &TextOptions);
	if (SetRichTextResult != EOS_EResult::EOS_Success)
	{
		UE_LOG_ONLINE(Error, TEXT("EOS_PresenceModification_SetRawRichText() failed with result code (%s)"), ANSI_TO_TCHAR(EOS_EResult_ToString(SetRichTextResult)));
	}

	TArray<FPresenceStrings, TInlineAllocator<EOS_PRESENCE_DATA_MAX_KEYS>> RawStrings;
	TArray<EOS_Presence_DataRecord, TInlineAllocator<EOS_PRESENCE_DATA_MAX_KEYS>> Records;
	int32 CurrentIndex = 0;
	// Loop through the properties building records
	for (FPresenceProperties::TConstIterator It(Status.Properties); It && CurrentIndex < EOS_PRESENCE_DATA_MAX_KEYS; ++It, ++CurrentIndex)
	{
		const FPresenceStrings& RawString = RawStrings.Emplace_GetRef(It.Key(), It.Value().ToString());

		EOS_Presence_DataRecord& Record = Records.Emplace_GetRef();
		Record.ApiVersion = EOS_PRESENCE_DATARECORD_API_LATEST;
		Record.Key = RawString.Key.Get();
		Record.Value = RawString.Value.Get();
	}
	EOS_PresenceModification_SetDataOptions DataOptions = { };
	DataOptions.ApiVersion = EOS_PRESENCE_SETDATA_API_LATEST;
	DataOptions.RecordsCount = Records.Num();
	DataOptions.Records = Records.GetData();
	EOS_EResult SetDataResult = EOS_PresenceModification_SetData(ChangeHandle, &DataOptions);
	if (SetDataResult != EOS_EResult::EOS_Success)
	{
		UE_LOG_ONLINE(Error, TEXT("EOS_PresenceModification_SetData() failed with result code (%s)"), *LexToString(SetDataResult));
	}

	FSetPresenceCallback* CallbackObj = new FSetPresenceCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, Delegate](const EOS_Presence_SetPresenceCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success && AccountIdToStringMap.Contains(Data->LocalUserId))
		{
			FUniqueNetIdEOSRef EOSID = FUniqueNetIdEOSRegistry::FindOrAdd(AccountIdToStringMap[Data->LocalUserId]).ToSharedRef();
			Delegate.ExecuteIfBound(*EOSID, true);
			return;
		}
		UE_LOG_ONLINE(Error, TEXT("SetPresence() failed with result code (%s)"), *LexToString(Data->ResultCode));
		Delegate.ExecuteIfBound(*FUniqueNetIdEOS::EmptyId(), false);
	};

	EOS_Presence_SetPresenceOptions PresOptions = { };
	PresOptions.ApiVersion = EOS_PRESENCE_SETPRESENCE_API_LATEST;
	PresOptions.LocalUserId = AccountId;
	PresOptions.PresenceModificationHandle = ChangeHandle;
	// Last step commit the changes
	EOS_Presence_SetPresence(EOSSubsystem->PresenceHandle, &PresOptions, CallbackObj, CallbackObj->GetCallbackPtr());
	EOS_PresenceModification_Release(ChangeHandle);
}

typedef TEOSCallback<EOS_Presence_OnQueryPresenceCompleteCallback, EOS_Presence_QueryPresenceCallbackInfo, FUserManagerEOS> FQueryPresenceCallback;

void FUserManagerEOS::QueryPresence(const FUniqueNetId& UserId, const FOnPresenceTaskCompleteDelegate& Delegate)
{
	if (DefaultLocalUser < 0)
	{
		UE_LOG_ONLINE(Error, TEXT("Can't QueryPresence() due to no users being signed in"));
		Delegate.ExecuteIfBound(UserId, false);
		return;
	}

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE(Error, TEXT("Can't QueryPresence(%s) for unknown unique net id"), *EOSID.ToDebugString());
		Delegate.ExecuteIfBound(UserId, false);
		return;
	}

	EOS_Presence_HasPresenceOptions HasOptions = { };
	HasOptions.ApiVersion = EOS_PRESENCE_HASPRESENCE_API_LATEST;
	HasOptions.LocalUserId = UserNumToAccountIdMap[DefaultLocalUser];
	HasOptions.TargetUserId = AccountId;
	EOS_Bool bHasPresence = EOS_Presence_HasPresence(EOSSubsystem->PresenceHandle, &HasOptions);
	if (bHasPresence == EOS_FALSE)
	{
		FQueryPresenceCallback* CallbackObj = new FQueryPresenceCallback(AsWeak());
		CallbackObj->CallbackLambda = [this, Delegate](const EOS_Presence_QueryPresenceCallbackInfo* Data)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success && EpicAccountIdToOnlineUserMap.Contains(Data->TargetUserId))
			{
				// Update the presence data to the most recent
				UpdatePresence(Data->TargetUserId);
				FOnlineUserPtr OnlineUser = EpicAccountIdToOnlineUserMap[Data->TargetUserId];
				Delegate.ExecuteIfBound(*OnlineUser->GetUserId(), true);
				return;
			}
			const FString& TargetUser = FUniqueNetIdEOSRegistry::FindOrAdd(Data->TargetUserId, nullptr)->ToString();
			UE_LOG_ONLINE(Error, TEXT("QueryPresence() for user (%s) failed with result code (%s)"), *TargetUser, *LexToString(Data->ResultCode));
			Delegate.ExecuteIfBound(*FUniqueNetIdEOS::EmptyId(), false);
		};

		// Query for updated presence
		EOS_Presence_QueryPresenceOptions Options = { };
		Options.ApiVersion = EOS_PRESENCE_QUERYPRESENCE_API_LATEST;
		Options.LocalUserId = HasOptions.LocalUserId;
		Options.TargetUserId = HasOptions.TargetUserId;
		EOS_Presence_QueryPresence(EOSSubsystem->PresenceHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
		return;
	}

	// Update the presence data to the most recent
	UpdatePresence(HasOptions.TargetUserId);
	// It's already present so trigger that it's done
	Delegate.ExecuteIfBound(UserId, true);
}

void FUserManagerEOS::UpdatePresence(EOS_EpicAccountId AccountId)
{
	EOS_Presence_Info* PresenceInfo = nullptr;
	EOS_Presence_CopyPresenceOptions Options = { };
	Options.ApiVersion = EOS_PRESENCE_COPYPRESENCE_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[DefaultLocalUser];
	Options.TargetUserId = AccountId;
	EOS_EResult CopyResult = EOS_Presence_CopyPresence(EOSSubsystem->PresenceHandle, &Options, &PresenceInfo);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		const FString& NetId = AccountIdToStringMap[AccountId];
		// Create it on demand if we don't have one yet
		if (!NetIdStringToOnlineUserPresenceMap.Contains(NetId))
		{
			FOnlineUserPresenceRef PresenceRef = MakeShareable(new FOnlineUserPresence());
			NetIdStringToOnlineUserPresenceMap.Emplace(NetId, PresenceRef);
		}

		FOnlineUserPresenceRef PresenceRef = NetIdStringToOnlineUserPresenceMap[NetId];
		const FString ProductId(UTF8_TO_TCHAR(PresenceInfo->ProductId));
		const FString ProdVersion(UTF8_TO_TCHAR(PresenceInfo->ProductVersion));
		const FString Platform(UTF8_TO_TCHAR(PresenceInfo->Platform));
		// Convert the presence data to our format
		PresenceRef->Status.State = ToEOnlinePresenceState(PresenceInfo->Status);
		PresenceRef->Status.StatusStr = UTF8_TO_TCHAR(PresenceInfo->RichText);
		PresenceRef->bIsOnline = PresenceRef->Status.State == EOnlinePresenceState::Online;
		PresenceRef->bIsPlaying = !ProductId.IsEmpty();
		PresenceRef->bIsPlayingThisGame = ProductId == EOSSubsystem->ProductId && ProdVersion == EOSSubsystem->EOSSDKManager->GetProductVersion();
//		PresenceRef->bIsJoinable = ???;
//		PresenceRef->bHasVoiceSupport = ???;
		PresenceRef->Status.Properties.Add(TEXT("ProductId"), ProductId);
		PresenceRef->Status.Properties.Add(TEXT("ProductVersion"), ProdVersion);
		PresenceRef->Status.Properties.Add(TEXT("Platform"), Platform);
		for (int32 Index = 0; Index < PresenceInfo->RecordsCount; Index++)
		{
			const EOS_Presence_DataRecord& Record = PresenceInfo->Records[Index];
			PresenceRef->Status.Properties.Add(Record.Key, UTF8_TO_TCHAR(Record.Value));
		}

		// Copy the presence if this is a friend that was updated, so that their data is in sync
		UpdateFriendPresence(NetId, PresenceRef);

		EOS_Presence_Info_Release(PresenceInfo);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to copy presence data with error code (%s)"), ANSI_TO_TCHAR(EOS_EResult_ToString(CopyResult)));
	}
}

void FUserManagerEOS::UpdateFriendPresence(const FString& FriendId, FOnlineUserPresenceRef Presence)
{
	for (TMap<int32, FFriendsListEOSRef>::TConstIterator It(LocalUserNumToFriendsListMap); It; ++It)
	{
		FFriendsListEOSRef FriendsList = It.Value();
		FOnlineFriendEOSPtr Friend = FriendsList->GetByNetIdString(FriendId);
		if (Friend.IsValid())
		{
			Friend->SetPresence(Presence);
		}
	}
}

EOnlineCachedResult::Type FUserManagerEOS::GetCachedPresence(const FUniqueNetId& UserId, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	if (NetIdStringToOnlineUserPresenceMap.Contains(EOSID.ToString()))
	{
		OutPresence = NetIdStringToOnlineUserPresenceMap[EOSID.ToString()];
		return EOnlineCachedResult::Success;
	}
	return EOnlineCachedResult::NotFound;
}

EOnlineCachedResult::Type FUserManagerEOS::GetCachedPresenceForApp(const FUniqueNetId&, const FUniqueNetId& UserId, const FString&, TSharedPtr<FOnlineUserPresence>& OutPresence)
{
	return GetCachedPresence(UserId, OutPresence);
}

bool FUserManagerEOS::QueryUserInfo(int32 LocalUserNum, const TArray<FUniqueNetIdRef>& UserIds)
{
	TArray<FString> UserEasIdsNeedingExternalMappings;
	UserEasIdsNeedingExternalMappings.Reserve(UserIds.Num());
	
	// Trigger a query for each user in the list
	for (const FUniqueNetIdRef& NetId : UserIds)
	{
		const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(*NetId);
		// Skip querying for local users since we already have that data
		if (StringToUserAccountMap.Contains(EOSID.ToString()))
		{
			continue;
		}
		// Check to see if we know about this user or not
		const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
		if (EOS_EpicAccountId_IsValid(AccountId) == EOS_TRUE)
		{
			// If the user is already registered, we'll update their user info
			if (EpicAccountIdToAttributeAccessMap.Contains(AccountId))
			{
				ReadUserInfo(LocalUserNum, AccountId);
			}
			else
			{
				// If the user is not registered, we'll add it and query their user info
				UserEasIdsNeedingExternalMappings.Add(LexToString(AccountId));

				// Registering the player will also query the user info data
				AddRemotePlayer(LocalUserNum, EOSID.ToString(), AccountId);
			}
		}
	}

	const FUniqueNetIdEOSPtr LocalId = GetLocalUniqueNetIdEOS(LocalUserNum);
	QueryExternalIdMappings(*LocalId, FExternalIdQueryOptions(), UserEasIdsNeedingExternalMappings, IgnoredMappingDelegate);
	
	return true;
}

typedef TEOSCallback<EOS_UserInfo_OnQueryUserInfoCallback, EOS_UserInfo_QueryUserInfoCallbackInfo, FUserManagerEOS> FReadUserInfoCallback;

void FUserManagerEOS::ReadUserInfo(int32 LocalUserNum, EOS_EpicAccountId EpicAccountId)
{
	FReadUserInfoCallback* CallbackObj = new FReadUserInfoCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, LocalUserNum, EpicAccountId](const EOS_UserInfo_QueryUserInfoCallbackInfo* Data)
	{
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			IAttributeAccessInterfaceRef AttributeAccessRef = EpicAccountIdToAttributeAccessMap[Data->TargetUserId];
			UpdateUserInfo(AttributeAccessRef, Data->LocalUserId, Data->TargetUserId);
		}

		// We mark this player as processed
		IsFriendQueryUserInfoOngoingForLocalUserMap[LocalUserNum].RemoveSwap(EpicAccountId, false);

		ProcessReadFriendsListComplete(LocalUserNum, true, TEXT(""));
	};

	EOS_UserInfo_QueryUserInfoOptions Options = { };
	Options.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
	Options.LocalUserId = UserNumToAccountIdMap[DefaultLocalUser];
	Options.TargetUserId = EpicAccountId;
	EOS_UserInfo_QueryUserInfo(EOSSubsystem->UserInfoHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	// We mark this player as pending for processing
	IsFriendQueryUserInfoOngoingForLocalUserMap.FindOrAdd(LocalUserNum).Add(EpicAccountId);
}

bool FUserManagerEOS::GetAllUserInfo(int32 LocalUserNum, TArray<TSharedRef<FOnlineUser>>& OutUsers)
{
	OutUsers.Reset();
	// Get remote users
	for (TMap<FString, FOnlineUserPtr>::TConstIterator It(NetIdStringToOnlineUserMap); It; ++It)
	{
		if (It.Value().IsValid())
		{
			OutUsers.Add(It.Value().ToSharedRef());
		}
	}
	// Get local users
	for (TMap<FString, FUserOnlineAccountEOSRef>::TConstIterator It(StringToUserAccountMap); It; ++It)
	{
		OutUsers.Add(It.Value());
	}
	return true;
}

TSharedPtr<FOnlineUser> FUserManagerEOS::GetUserInfo(int32 LocalUserNum, const FUniqueNetId& UserId)
{
	TSharedPtr<FOnlineUser> OnlineUser;
	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	if (NetIdStringToOnlineUserMap.Contains(EOSID.ToString()))
	{
		OnlineUser = NetIdStringToOnlineUserMap[EOSID.ToString()];
	}
	return OnlineUser;
}

struct FQueryByDisplayNameOptions :
	public EOS_UserInfo_QueryUserInfoByDisplayNameOptions
{
	FQueryByDisplayNameOptions() :
		EOS_UserInfo_QueryUserInfoByDisplayNameOptions()
	{
		ApiVersion = EOS_USERINFO_QUERYUSERINFOBYDISPLAYNAME_API_LATEST;
		DisplayName = DisplayNameAnsi;
	}
	char DisplayNameAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
};

typedef TEOSCallback<EOS_UserInfo_OnQueryUserInfoByDisplayNameCallback, EOS_UserInfo_QueryUserInfoByDisplayNameCallbackInfo, FUserManagerEOS> FQueryInfoByNameCallback;

bool FUserManagerEOS::QueryUserIdMapping(const FUniqueNetId& UserId, const FString& DisplayNameOrEmail, const FOnQueryUserMappingComplete& Delegate)
{
	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		UE_LOG_ONLINE(Error, TEXT("Specified local user (%s) is not known"), *EOSID.ToDebugString());
		Delegate.ExecuteIfBound(false, UserId, DisplayNameOrEmail, *FUniqueNetIdEOS::EmptyId(), FString::Printf(TEXT("Specified local user (%s) is not known"), *EOSID.ToDebugString()));
		return false;
	}
	int32 LocalUserNum = GetLocalUserNumFromUniqueNetId(UserId);

	FQueryInfoByNameCallback* CallbackObj = new FQueryInfoByNameCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, DisplayNameOrEmail, this, Delegate](const EOS_UserInfo_QueryUserInfoByDisplayNameCallbackInfo* Data)
	{
		EOS_EResult Result = Data->ResultCode;
		if (GetLoginStatus(LocalUserNum) != ELoginStatus::LoggedIn)
		{
			// Handle the user logging out while a read is in progress
			Result = EOS_EResult::EOS_InvalidUser;
		}

		FString ErrorString;
		bool bWasSuccessful = Result == EOS_EResult::EOS_Success;
		if (bWasSuccessful)
		{
			const FString NetIdStr = LexToString(Data->TargetUserId);
			FUniqueNetIdEOSPtr LocalUserId = UserNumToNetIdMap[DefaultLocalUser];
			if (!EpicAccountIdToOnlineUserMap.Contains(Data->TargetUserId))
			{
				// Registering the player will also query the presence/user info data
				AddRemotePlayer(LocalUserNum, NetIdStr, Data->TargetUserId);
			}

			Delegate.ExecuteIfBound(true, *LocalUserId, DisplayNameOrEmail, *FUniqueNetIdEOSRegistry::FindOrAdd(NetIdStr), ErrorString);
		}
		else
		{
			ErrorString = FString::Printf(TEXT("QueryUserIdMapping(%d, '%s') failed with EOS result code (%s)"), DefaultLocalUser, *DisplayNameOrEmail, ANSI_TO_TCHAR(EOS_EResult_ToString(Result)));
		}
		Delegate.ExecuteIfBound(false, *FUniqueNetIdEOS::EmptyId(), DisplayNameOrEmail, *FUniqueNetIdEOS::EmptyId(), ErrorString);
	};

	FQueryByDisplayNameOptions Options;
	FCStringAnsi::Strncpy(Options.DisplayNameAnsi, TCHAR_TO_UTF8(*DisplayNameOrEmail), EOS_OSS_STRING_BUFFER_LENGTH);
	Options.LocalUserId = AccountId;
	EOS_UserInfo_QueryUserInfoByDisplayName(EOSSubsystem->UserInfoHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());

	return true;
}

struct FQueryByStringIdsOptions :
	public EOS_Connect_QueryExternalAccountMappingsOptions
{
	FQueryByStringIdsOptions(const uint32 InNumStringIds, EOS_ProductUserId InLocalUserId) :
		EOS_Connect_QueryExternalAccountMappingsOptions()
	{
		PointerArray.AddZeroed(InNumStringIds);
		for (int32 Index = 0; Index < PointerArray.Num(); Index++)
		{
			PointerArray[Index] = new char[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH+1];
		}
		ApiVersion = EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_API_LATEST;
		AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
		ExternalAccountIds = (const char**)PointerArray.GetData();
		ExternalAccountIdCount = InNumStringIds;
		LocalUserId = InLocalUserId;
	}

	~FQueryByStringIdsOptions()
	{
		for (int32 Index = 0; Index < PointerArray.Num(); Index++)
		{
			delete [] PointerArray[Index];
		}
	}
	TArray<char*> PointerArray;
};

struct FGetAccountMappingOptions :
	public EOS_Connect_GetExternalAccountMappingsOptions
{
	FGetAccountMappingOptions() :
		EOS_Connect_GetExternalAccountMappingsOptions()
	{
		ApiVersion = EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST;
		AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
		TargetExternalUserId = AccountId;
	}
	char AccountId[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH+1];
};

typedef TEOSCallback<EOS_Connect_OnQueryExternalAccountMappingsCallback, EOS_Connect_QueryExternalAccountMappingsCallbackInfo, FUserManagerEOS> FQueryByStringIdsCallback;

bool FUserManagerEOS::QueryExternalIdMappings(const FUniqueNetId& UserId, const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, const FOnQueryExternalIdMappingsComplete& Delegate)
{
	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const EOS_EpicAccountId AccountId = EOSID.GetEpicAccountId();
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_FALSE)
	{
		Delegate.ExecuteIfBound(false, UserId, QueryOptions, ExternalIds, FString::Printf(TEXT("User (%s) is not logged in, so can't query external account ids"), *EOSID.ToDebugString()));
		return false;
	}

	if (ExternalIds.IsEmpty())
	{
		Delegate.ExecuteIfBound(false, UserId, QueryOptions, ExternalIds, FString::Printf(TEXT("List of ids to query is empty for User (%s), so can't query external account ids"), *EOSID.ToDebugString()));
		return false;
	}

	int32 LocalUserNum = GetLocalUserNumFromUniqueNetId(UserId);

	// Mark the queries as in progress
	IsPlayerQueryExternalMappingsOngoingForLocalUserMap.FindOrAdd(LocalUserNum).Append(ExternalIds);

	const EOS_ProductUserId LocalUserId = EOSID.GetProductUserId();
	const int32 NumBatches = (ExternalIds.Num() / EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_MAX_ACCOUNT_IDS) + 1;
	int32 QueryStart = 0;
	// Process queries in batches since there's a max that can be done at once
	for (int32 BatchCount = 0; BatchCount < NumBatches; BatchCount++)
	{
		const uint32 AmountToProcess = FMath::Min(ExternalIds.Num() - QueryStart, EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_MAX_ACCOUNT_IDS);
		TArray<FString> BatchIds;
		BatchIds.Empty(AmountToProcess);
		FQueryByStringIdsOptions Options(AmountToProcess, LocalUserId);
		// Build an options up per batch
		for (uint32 ProcessedCount = 0; ProcessedCount < AmountToProcess; ProcessedCount++, QueryStart++)
		{
			FCStringAnsi::Strncpy(Options.PointerArray[ProcessedCount], TCHAR_TO_UTF8(*ExternalIds[ProcessedCount]), EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH+1);
			BatchIds.Add(ExternalIds[ProcessedCount]);
		}
		FQueryByStringIdsCallback* CallbackObj = new FQueryByStringIdsCallback(AsWeak());
		CallbackObj->CallbackLambda = [LocalUserNum, QueryOptions, BatchIds, this, Delegate](const EOS_Connect_QueryExternalAccountMappingsCallbackInfo* Data)
		{
			EOS_EResult Result = Data->ResultCode;
			if (GetLoginStatus(LocalUserNum) != ELoginStatus::LoggedIn)
			{
				// Handle the user logging out while a read is in progress
				Result = EOS_EResult::EOS_InvalidUser;
			}

			FString ErrorString;
			FUniqueNetIdEOSPtr EOSID = FUniqueNetIdEOS::EmptyId();
			if (Result == EOS_EResult::EOS_Success)
			{
				EOSID = UserNumToNetIdMap[LocalUserNum];

				FGetAccountMappingOptions Options;
				Options.LocalUserId = UserNumToProductUserIdMap[DefaultLocalUser];
				// Get the product id for each epic account passed in
				for (const FString& StringId : BatchIds)
				{
					FCStringAnsi::Strncpy(Options.AccountId, TCHAR_TO_UTF8(*StringId), EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH + 1);
					EOS_ProductUserId ProductUserId = EOS_Connect_GetExternalAccountMapping(EOSSubsystem->ConnectHandle, &Options);
					if (EOS_ProductUserId_IsValid(ProductUserId) == EOS_TRUE)
					{
						EOS_EpicAccountId AccountId = EOS_EpicAccountId_FromString(Options.AccountId);
						UpdateRemotePlayerProductUserId(AccountId, ProductUserId);
					}
				}
			}
			else
			{
				ErrorString = FString::Printf(TEXT("EOS_Connect_QueryExternalAccountMappings() failed with result code (%s)"), ANSI_TO_TCHAR(EOS_EResult_ToString(Result)));
			}

			// Mark all queries as complete
			TArray<FString>& OngoingQueries = IsPlayerQueryExternalMappingsOngoingForLocalUserMap[LocalUserNum];
			for (const FString& StringId : BatchIds)
			{
				OngoingQueries.RemoveSwap(StringId, false);
			}

			const bool bWasSuccessful = Result == EOS_EResult::EOS_Success;
			Delegate.ExecuteIfBound(bWasSuccessful, *EOSID, QueryOptions, BatchIds, ErrorString);
		};

		EOS_Connect_QueryExternalAccountMappings(EOSSubsystem->ConnectHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	return true;
}

void FUserManagerEOS::GetExternalIdMappings(const FExternalIdQueryOptions& QueryOptions, const TArray<FString>& ExternalIds, TArray<FUniqueNetIdPtr>& OutIds)
{
	OutIds.Reset();
	for (const FString& AccountIdStr : ExternalIds)
	{
		OutIds.Add(GetExternalIdMapping(QueryOptions, AccountIdStr));
	}
}

FUniqueNetIdPtr FUserManagerEOS::GetExternalIdMapping(const FExternalIdQueryOptions& QueryOptions, const FString& ExternalId)
{
	FUniqueNetIdPtr NetId;
	EOS_EpicAccountId AccountId = EOS_EpicAccountId_FromString(TCHAR_TO_UTF8(*ExternalId));
	if (EOS_EpicAccountId_IsValid(AccountId) == EOS_TRUE && AccountIdToStringMap.Contains(AccountId))
	{
		const FString& NetIdStr = AccountIdToStringMap[AccountId];
		NetId = NetIdStringToOnlineUserMap[NetIdStr]->GetUserId();
	}
	return NetId;
}

#endif