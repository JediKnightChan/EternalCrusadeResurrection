// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemEOS.h"
#include "OnlineSubsystemEOSPrivate.h"
#include "OnlineSubsystemUtils.h"
#include "UserManagerEOS.h"
#include "OnlineSessionEOS.h"
#include "OnlineStatsEOS.h"
#include "OnlineLeaderboardsEOS.h"
#include "OnlineAchievementsEOS.h"
#include "OnlineTitleFileEOS.h"
#include "OnlineUserCloudEOS.h"
#include "OnlineStoreEOS.h"
#include "EOSSettings.h"
#include "EOSShared.h"
#include "IEOSSDKManager.h"
#include "SocketSubsystemEOSUtils_OnlineSubsystemEOS.h"

#include "Features/IModularFeatures.h"
#include "Misc/App.h"
#include "Misc/NetworkVersion.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"

#if WITH_EOS_SDK

// Missing defines
#define EOS_ENCRYPTION_KEY_MAX_LENGTH 64
#define EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN (EOS_ENCRYPTION_KEY_MAX_LENGTH + 1)

#if WITH_EOS_RTC

#include "EOSVoiceChatFactory.h"
#include "EOSVoiceChatUser.h"
#include "VoiceChatResult.h"
#include "VoiceChatErrors.h"

/** Class that blocks login/logout for the OSS EOS managed IVoiceChatUser interfaces. */
class FOnlineSubsystemEOSVoiceChatUserWrapper : public IVoiceChatUser
{
public:
	FOnlineSubsystemEOSVoiceChatUserWrapper(FEOSVoiceChatUser& InVoiceChatUser) : VoiceChatUser(InVoiceChatUser) {}
	~FOnlineSubsystemEOSVoiceChatUserWrapper() = default;

	// ~Begin IVoiceChatUser
	virtual void SetSetting(const FString& Name, const FString& Value) override { VoiceChatUser.SetSetting(Name, Value); }
	virtual FString GetSetting(const FString& Name) override { return VoiceChatUser.GetSetting(Name); }
	virtual void SetAudioInputVolume(float Volume) override { VoiceChatUser.SetAudioInputVolume(Volume); }
	virtual void SetAudioOutputVolume(float Volume) override { VoiceChatUser.SetAudioOutputVolume(Volume); }
	virtual float GetAudioInputVolume() const override { return VoiceChatUser.GetAudioInputVolume(); }
	virtual float GetAudioOutputVolume() const override { return VoiceChatUser.GetAudioOutputVolume(); }
	virtual void SetAudioInputDeviceMuted(bool bIsMuted) override { VoiceChatUser.SetAudioInputDeviceMuted(bIsMuted); }
	virtual void SetAudioOutputDeviceMuted(bool bIsMuted) override { VoiceChatUser.SetAudioOutputDeviceMuted(bIsMuted); }
	virtual bool GetAudioInputDeviceMuted() const override { return VoiceChatUser.GetAudioInputDeviceMuted(); }
	virtual bool GetAudioOutputDeviceMuted() const override { return VoiceChatUser.GetAudioOutputDeviceMuted(); }
	virtual TArray<FVoiceChatDeviceInfo> GetAvailableInputDeviceInfos() const override { return VoiceChatUser.GetAvailableInputDeviceInfos(); }
	virtual TArray<FVoiceChatDeviceInfo> GetAvailableOutputDeviceInfos() const override { return VoiceChatUser.GetAvailableOutputDeviceInfos(); }
	virtual FOnVoiceChatAvailableAudioDevicesChangedDelegate& OnVoiceChatAvailableAudioDevicesChanged() override { return VoiceChatUser.OnVoiceChatAvailableAudioDevicesChanged(); }
	virtual void SetInputDeviceId(const FString& InputDeviceId) override { VoiceChatUser.SetInputDeviceId(InputDeviceId); }
	virtual void SetOutputDeviceId(const FString& OutputDeviceId) override { VoiceChatUser.SetOutputDeviceId(OutputDeviceId); }
	virtual FVoiceChatDeviceInfo GetInputDeviceInfo() const override { return VoiceChatUser.GetInputDeviceInfo(); }
	virtual FVoiceChatDeviceInfo GetOutputDeviceInfo() const override { return VoiceChatUser.GetOutputDeviceInfo(); }
	virtual FVoiceChatDeviceInfo GetDefaultInputDeviceInfo() const override { return VoiceChatUser.GetDefaultInputDeviceInfo(); }
	virtual FVoiceChatDeviceInfo GetDefaultOutputDeviceInfo() const override { return VoiceChatUser.GetDefaultOutputDeviceInfo(); }
	virtual void Login(FPlatformUserId PlatformId, const FString& PlayerName, const FString& Credentials, const FOnVoiceChatLoginCompleteDelegate& Delegate) override
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: IVoiceChatUser::Login called on OSS EOS managed VoiceChatUser interface."));
		checkNoEntry();
		Delegate.ExecuteIfBound(PlayerName, VoiceChat::Errors::NotPermitted());
	}
	virtual void Logout(const FOnVoiceChatLogoutCompleteDelegate& Delegate) override
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: IVoiceChatUser::Logout called on OSS EOS managed VoiceChatUser interface."));
		checkNoEntry();
		Delegate.ExecuteIfBound(VoiceChatUser.GetLoggedInPlayerName(), VoiceChat::Errors::NotPermitted());
	}
	virtual bool IsLoggingIn() const override { return VoiceChatUser.IsLoggingIn(); }
	virtual bool IsLoggedIn() const override { return VoiceChatUser.IsLoggedIn(); }
	virtual FOnVoiceChatLoggedInDelegate& OnVoiceChatLoggedIn() override { return VoiceChatUser.OnVoiceChatLoggedIn(); }
	virtual FOnVoiceChatLoggedOutDelegate& OnVoiceChatLoggedOut() override { return VoiceChatUser.OnVoiceChatLoggedOut(); }
	virtual FString GetLoggedInPlayerName() const override { return VoiceChatUser.GetLoggedInPlayerName(); }
	virtual void BlockPlayers(const TArray<FString>& PlayerNames) override { VoiceChatUser.BlockPlayers(PlayerNames); }
	virtual void UnblockPlayers(const TArray<FString>& PlayerNames) override { return VoiceChatUser.UnblockPlayers(PlayerNames); }
	virtual void JoinChannel(const FString& ChannelName, const FString& ChannelCredentials, EVoiceChatChannelType ChannelType, const FOnVoiceChatChannelJoinCompleteDelegate& Delegate, TOptional<FVoiceChatChannel3dProperties> Channel3dProperties = TOptional<FVoiceChatChannel3dProperties>()) override { VoiceChatUser.JoinChannel(ChannelName, ChannelCredentials, ChannelType, Delegate, Channel3dProperties); }
	virtual void LeaveChannel(const FString& ChannelName, const FOnVoiceChatChannelLeaveCompleteDelegate& Delegate) override { VoiceChatUser.LeaveChannel(ChannelName, Delegate); }
	virtual FOnVoiceChatChannelJoinedDelegate& OnVoiceChatChannelJoined() override { return VoiceChatUser.OnVoiceChatChannelJoined(); }
	virtual FOnVoiceChatChannelExitedDelegate& OnVoiceChatChannelExited() override { return VoiceChatUser.OnVoiceChatChannelExited(); }
	virtual FOnVoiceChatCallStatsUpdatedDelegate& OnVoiceChatCallStatsUpdated() override { return VoiceChatUser.OnVoiceChatCallStatsUpdated(); }
	virtual void Set3DPosition(const FString& ChannelName, const FVector& SpeakerPosition, const FVector& ListenerPosition, const FVector& ListenerForwardDirection, const FVector& ListenerUpDirection) override { VoiceChatUser.Set3DPosition(ChannelName, SpeakerPosition, ListenerPosition, ListenerForwardDirection, ListenerUpDirection); }
	virtual TArray<FString> GetChannels() const override { return VoiceChatUser.GetChannels(); }
	virtual TArray<FString> GetPlayersInChannel(const FString& ChannelName) const override { return VoiceChatUser.GetPlayersInChannel(ChannelName); }
	virtual EVoiceChatChannelType GetChannelType(const FString& ChannelName) const override { return VoiceChatUser.GetChannelType(ChannelName); }
	virtual FOnVoiceChatPlayerAddedDelegate& OnVoiceChatPlayerAdded() override { return VoiceChatUser.OnVoiceChatPlayerAdded(); }
	virtual FOnVoiceChatPlayerRemovedDelegate& OnVoiceChatPlayerRemoved() override { return VoiceChatUser.OnVoiceChatPlayerRemoved(); }
	virtual bool IsPlayerTalking(const FString& PlayerName) const override { return VoiceChatUser.IsPlayerTalking(PlayerName); }
	virtual FOnVoiceChatPlayerTalkingUpdatedDelegate& OnVoiceChatPlayerTalkingUpdated() override { return VoiceChatUser.OnVoiceChatPlayerTalkingUpdated(); }
	virtual void SetPlayerMuted(const FString& PlayerName, bool bMuted) override { VoiceChatUser.SetPlayerMuted(PlayerName, bMuted); }
	virtual bool IsPlayerMuted(const FString& PlayerName) const override { return VoiceChatUser.IsPlayerMuted(PlayerName); }
	virtual void SetChannelPlayerMuted(const FString& ChannelName, const FString& PlayerName, bool bMuted) override { VoiceChatUser.SetChannelPlayerMuted(ChannelName, PlayerName, bMuted); }
	virtual bool IsChannelPlayerMuted(const FString& ChannelName, const FString& PlayerName) const override { return VoiceChatUser.IsChannelPlayerMuted(ChannelName, PlayerName); }
	virtual FOnVoiceChatPlayerMuteUpdatedDelegate& OnVoiceChatPlayerMuteUpdated() override { return VoiceChatUser.OnVoiceChatPlayerMuteUpdated(); }
	virtual void SetPlayerVolume(const FString& PlayerName, float Volume) override { VoiceChatUser.SetPlayerVolume(PlayerName, Volume); }
	virtual float GetPlayerVolume(const FString& PlayerName) const override { return VoiceChatUser.GetPlayerVolume(PlayerName); }
	virtual FOnVoiceChatPlayerVolumeUpdatedDelegate& OnVoiceChatPlayerVolumeUpdated() override { return VoiceChatUser.OnVoiceChatPlayerVolumeUpdated(); }
	virtual void TransmitToAllChannels() override { VoiceChatUser.TransmitToAllChannels(); }
	virtual void TransmitToNoChannels() override { VoiceChatUser.TransmitToNoChannels(); }
	virtual void TransmitToSpecificChannel(const FString& ChannelName) override { VoiceChatUser.TransmitToSpecificChannel(ChannelName); }
	virtual EVoiceChatTransmitMode GetTransmitMode() const override { return VoiceChatUser.GetTransmitMode(); }
	virtual FString GetTransmitChannel() const override { return VoiceChatUser.GetTransmitChannel(); }
	virtual FDelegateHandle StartRecording(const FOnVoiceChatRecordSamplesAvailableDelegate::FDelegate& Delegate) override { return VoiceChatUser.StartRecording(Delegate); }
	virtual void StopRecording(FDelegateHandle Handle) override { VoiceChatUser.StopRecording(Handle); }
	virtual FDelegateHandle RegisterOnVoiceChatAfterCaptureAudioReadDelegate(const FOnVoiceChatAfterCaptureAudioReadDelegate::FDelegate& Delegate) override { return VoiceChatUser.RegisterOnVoiceChatAfterCaptureAudioReadDelegate(Delegate); }
	virtual void UnregisterOnVoiceChatAfterCaptureAudioReadDelegate(FDelegateHandle Handle) override { VoiceChatUser.UnregisterOnVoiceChatAfterCaptureAudioReadDelegate(Handle); }
	virtual FDelegateHandle RegisterOnVoiceChatBeforeCaptureAudioSentDelegate(const FOnVoiceChatBeforeCaptureAudioSentDelegate::FDelegate& Delegate) override { return VoiceChatUser.RegisterOnVoiceChatBeforeCaptureAudioSentDelegate(Delegate); }
	virtual void UnregisterOnVoiceChatBeforeCaptureAudioSentDelegate(FDelegateHandle Handle) override { VoiceChatUser.UnregisterOnVoiceChatBeforeCaptureAudioSentDelegate(Handle); }
	virtual FDelegateHandle RegisterOnVoiceChatBeforeRecvAudioRenderedDelegate(const FOnVoiceChatBeforeRecvAudioRenderedDelegate::FDelegate& Delegate) override { return VoiceChatUser.RegisterOnVoiceChatBeforeRecvAudioRenderedDelegate(Delegate); }
	virtual void UnregisterOnVoiceChatBeforeRecvAudioRenderedDelegate(FDelegateHandle Handle) override { VoiceChatUser.UnregisterOnVoiceChatBeforeRecvAudioRenderedDelegate(Handle); }
	virtual FString InsecureGetLoginToken(const FString& PlayerName) override { return VoiceChatUser.InsecureGetLoginToken(PlayerName); }
	virtual FString InsecureGetJoinToken(const FString& ChannelName, EVoiceChatChannelType ChannelType, TOptional<FVoiceChatChannel3dProperties> Channel3dProperties = TOptional<FVoiceChatChannel3dProperties>()) override { return VoiceChatUser.InsecureGetJoinToken(ChannelName, ChannelType, Channel3dProperties); }
	// ~End IVoiceChatUser

	FEOSVoiceChatUser& VoiceChatUser;
};

#endif // WITH_EOS_RTC

/** Class that holds the strings for the call duration */
struct FEOSPlatformOptions :
	public EOS_Platform_Options
{
	FEOSPlatformOptions() :
		EOS_Platform_Options()
	{
		ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		ProductId = ProductIdAnsi;
		SandboxId = SandboxIdAnsi;
		DeploymentId = DeploymentIdAnsi;
		ClientCredentials.ClientId = ClientIdAnsi;
		ClientCredentials.ClientSecret = ClientSecretAnsi;
		CacheDirectory = CacheDirectoryAnsi;
		EncryptionKey = EncryptionKeyAnsi;
	}

	char ClientIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char ClientSecretAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char ProductIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char SandboxIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char DeploymentIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char CacheDirectoryAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char EncryptionKeyAnsi[EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN];
};

FPlatformEOSHelpersPtr FOnlineSubsystemEOS::EOSHelpersPtr;

void FOnlineSubsystemEOS::ModuleInit()
{
	LLM_SCOPE(ELLMTag::RealTimeCommunications);

	EOSHelpersPtr = MakeShareable(new FPlatformEOSHelpers());

	const FName EOSSharedModuleName = TEXT("EOSShared");
	if (!FModuleManager::Get().IsModuleLoaded(EOSSharedModuleName))
	{
		FModuleManager::Get().LoadModuleChecked(EOSSharedModuleName);
	}
	IEOSSDKManager* EOSSDKManager = IEOSSDKManager::Get();
	if (!EOSSDKManager)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: Missing IEOSSDKManager modular feature."));
		return;
	}

	EOS_EResult InitResult = EOSSDKManager->Initialize();
	if (InitResult != EOS_EResult::EOS_Success)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to initialize the EOS SDK with result code (%d)"), InitResult);
		return;
	}
}

void FOnlineSubsystemEOS::ModuleShutdown()
{
#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
		ensure(Interface.IsUnique()); \
		Interface = nullptr; \
	}

	DESTRUCT_INTERFACE(EOSHelpersPtr);

#undef DESTRUCT_INTERFACE
}

/** Common method for creating the EOS platform */
bool FOnlineSubsystemEOS::PlatformCreate()
{
	FString ArtifactName;
	FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
	// Find the settings for this artifact
	FEOSArtifactSettings ArtifactSettings;
	if (!UEOSSettings::GetSettingsForArtifact(ArtifactName, ArtifactSettings))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS::PlatformCreate() failed to find artifact settings object for artifact (%s)"), *ArtifactName);
		return false;
	}

	// Create platform instance
	FEOSPlatformOptions PlatformOptions;
	FCStringAnsi::Strncpy(PlatformOptions.ClientIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ClientId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.ClientSecretAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ClientSecret), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.ProductIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ProductId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.SandboxIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.SandboxId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.DeploymentIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.DeploymentId), EOS_OSS_STRING_BUFFER_LENGTH);
	PlatformOptions.bIsServer = IsRunningDedicatedServer() ? EOS_TRUE : EOS_FALSE;
	PlatformOptions.Reserved = nullptr;
	FEOSSettings EOSSettings = UEOSSettings::GetSettings();
	uint64 OverlayFlags = 0;
	if (!EOSSettings.bEnableOverlay)
	{
		OverlayFlags |= EOS_PF_DISABLE_OVERLAY;
	}
	if (!EOSSettings.bEnableSocialOverlay)
	{
		OverlayFlags |= EOS_PF_DISABLE_SOCIAL_OVERLAY;
	}
#if WITH_EDITOR
	if (!EOSSettings.bEnableEditorOverlay)
	{
		OverlayFlags |= EOS_PF_LOADING_IN_EDITOR;
	}
#endif

	// Don't allow the overlay to be used in the editor when running PIE.
	const bool bEditorOverlayAllowed = EOSSettings.bEnableEditorOverlay && InstanceName == FOnlineSubsystemImpl::DefaultInstanceName;
	const bool bOverlayAllowed = IsRunningGame() || bEditorOverlayAllowed;

	PlatformOptions.Flags = bOverlayAllowed ? OverlayFlags : EOS_PF_DISABLE_OVERLAY;
	// Make the cache directory be in the user's writable area

	const FString CacheDir = EOSSDKManager->GetCacheDirBase() / ArtifactName / EOSSettings.CacheDir;
	FCStringAnsi::Strncpy(PlatformOptions.CacheDirectoryAnsi, TCHAR_TO_UTF8(*CacheDir), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.EncryptionKeyAnsi, TCHAR_TO_UTF8(*ArtifactSettings.EncryptionKey), EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN);

#if WITH_EOS_RTC
	EOS_Platform_RTCOptions RtcOptions = { 0 };
	RtcOptions.ApiVersion = EOS_PLATFORM_RTCOPTIONS_API_LATEST;
	RtcOptions.PlatformSpecificOptions = nullptr;
	PlatformOptions.RTCOptions = &RtcOptions;
#endif

	EOSPlatformHandle = EOSHelpersPtr->CreatePlatform(PlatformOptions);
	if (EOSPlatformHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS::PlatformCreate() failed to init EOS platform"));
		return false;
	}
	
	return true;
}

bool FOnlineSubsystemEOS::Init()
{
	// Determine if we are the default and if we're the platform OSS
	FString DefaultOSS;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("DefaultPlatformService"), DefaultOSS, GEngineIni);
	FString PlatformOSS;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("NativePlatformService"), PlatformOSS, GEngineIni);
	bIsDefaultOSS = DefaultOSS == TEXT("EOS");
	bIsPlatformOSS = PlatformOSS == TEXT("EOS");

	// Check for being launched by EGS
	bWasLaunchedByEGS = FParse::Param(FCommandLine::Get(), TEXT("EpicPortal"));
	FEOSSettings EOSSettings = UEOSSettings::GetSettings();
	if (!IsRunningDedicatedServer() && IsRunningGame() && !bWasLaunchedByEGS && EOSSettings.bShouldEnforceBeingLaunchedByEGS)
	{
		FString ArtifactName;
		FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemEOS::Init() relaunching artifact (%s) via the store"), *ArtifactName);
		FPlatformProcess::LaunchURL(*FString::Printf(TEXT("com.epicgames.launcher://store/product/%s?action=launch&silent=true"), *ArtifactName), nullptr, nullptr);
		FPlatformMisc::RequestExit(false);
		return false;
	}

	EOSSDKManager = IEOSSDKManager::Get();
	if (!EOSSDKManager)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS::Init() failed to get EOSSDKManager interface"));
		return false;
	}

	if (!PlatformCreate())
	{
		return false;
	}

	// Get handles for later use
	AuthHandle = EOS_Platform_GetAuthInterface(*EOSPlatformHandle);
	if (AuthHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get auth handle"));
		return false;
	}
	UserInfoHandle = EOS_Platform_GetUserInfoInterface(*EOSPlatformHandle);
	if (UserInfoHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get user info handle"));
		return false;
	}
	UIHandle = EOS_Platform_GetUIInterface(*EOSPlatformHandle);
	if (UIHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get UI handle"));
		return false;
	}
	FriendsHandle = EOS_Platform_GetFriendsInterface(*EOSPlatformHandle);
	if (FriendsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get friends handle"));
		return false;
	}
	PresenceHandle = EOS_Platform_GetPresenceInterface(*EOSPlatformHandle);
	if (PresenceHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get presence handle"));
		return false;
	}
	ConnectHandle = EOS_Platform_GetConnectInterface(*EOSPlatformHandle);
	if (ConnectHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get connect handle"));
		return false;
	}
	SessionsHandle = EOS_Platform_GetSessionsInterface(*EOSPlatformHandle);
	if (SessionsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get sessions handle"));
		return false;
	}
	StatsHandle = EOS_Platform_GetStatsInterface(*EOSPlatformHandle);
	if (StatsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get stats handle"));
		return false;
	}
	LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(*EOSPlatformHandle);
	if (LeaderboardsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get leaderboards handle"));
		return false;
	}
	MetricsHandle = EOS_Platform_GetMetricsInterface(*EOSPlatformHandle);
	if (MetricsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get metrics handle"));
		return false;
	}
	AchievementsHandle = EOS_Platform_GetAchievementsInterface(*EOSPlatformHandle);
	if (AchievementsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get achievements handle"));
		return false;
	}
	// Disable ecom if not part of EGS
	if (bWasLaunchedByEGS)
	{
		EcomHandle = EOS_Platform_GetEcomInterface(*EOSPlatformHandle);
		if (EcomHandle == nullptr)
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get ecom handle"));
			return false;
		}
		StoreInterfacePtr = MakeShareable(new FOnlineStoreEOS(this));
	}
	TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(*EOSPlatformHandle);
	if (TitleStorageHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get title storage handle"));
		return false;
	}
	PlayerDataStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(*EOSPlatformHandle);
	if (PlayerDataStorageHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to init EOS platform, couldn't get player data storage handle"));
		return false;
	}

	SocketSubsystem = MakeShareable(new FSocketSubsystemEOS(EOSPlatformHandle, MakeShareable(new FSocketSubsystemEOSUtils_OnlineSubsystemEOS(*this))));
	check(SocketSubsystem);
	FString ErrorMessage;
	if (!SocketSubsystem->Init(ErrorMessage))
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineSubsystemEOS::Init] Unable to initialize Socket Subsystem. Error=[%s]"), *ErrorMessage);
	}

	// We set the product id
	FString ArtifactName;
	FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
	FEOSArtifactSettings ArtifactSettings;
	if (UEOSSettings::GetSettingsForArtifact(ArtifactName, ArtifactSettings))
	{
		ProductId = ArtifactSettings.ProductId;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineSubsystemEOS::Init] Failed to find artifact settings object for artifact (%s). ProductIdAnsi not set."), *ArtifactName);
	}

	UserManager = MakeShareable(new FUserManagerEOS(this));
	UserManager->Init();
	SessionInterfacePtr = MakeShareable(new FOnlineSessionEOS(this));
	// Set the bucket id to use for all sessions based upon the name and version to avoid upgrade issues
	SessionInterfacePtr->Init(EOSSDKManager->GetProductName() + TEXT("_") + FString::FromInt(GetBuildUniqueId()));
	StatsInterfacePtr = MakeShareable(new FOnlineStatsEOS(this));
	LeaderboardsInterfacePtr = MakeShareable(new FOnlineLeaderboardsEOS(this));
	AchievementsInterfacePtr = MakeShareable(new FOnlineAchievementsEOS(this));
	TitleFileInterfacePtr = MakeShareable(new FOnlineTitleFileEOS(this));
	UserCloudInterfacePtr = MakeShareable(new FOnlineUserCloudEOS(this));

	// We initialized ok so we can tick
	StartTicker();

	return true;
}

bool FOnlineSubsystemEOS::Shutdown()
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("FOnlineSubsystemEOS::Shutdown()"));

	// EOS-22677 workaround: Make sure tick is called at least once before shutting down.
	if (EOSPlatformHandle)
	{
		EOS_Platform_Tick(*EOSPlatformHandle);
	}

	StopTicker();

	if (SocketSubsystem)
	{
		SocketSubsystem->Shutdown();
		SocketSubsystem = nullptr;
	}

	// Release our ref to the interfaces. May still exist since they can be aggregated
	UserManager = nullptr;
	SessionInterfacePtr = nullptr;
	StatsInterfacePtr = nullptr;
	LeaderboardsInterfacePtr = nullptr;
	AchievementsInterfacePtr = nullptr;
	StoreInterfacePtr = nullptr;
	TitleFileInterfacePtr = nullptr;
	UserCloudInterfacePtr = nullptr;

#if WITH_EOS_RTC
	for (TPair<FUniqueNetIdRef, FOnlineSubsystemEOSVoiceChatUserWrapperRef>& Pair : LocalVoiceChatUsers)
	{
		FOnlineSubsystemEOSVoiceChatUserWrapperRef& VoiceChatUserWrapper = Pair.Value;
		VoiceChatInterface->ReleaseUser(&VoiceChatUserWrapper->VoiceChatUser);
	}
	LocalVoiceChatUsers.Reset();
	VoiceChatInterface = nullptr;
#endif

	EOSPlatformHandle = nullptr;

	return FOnlineSubsystemImpl::Shutdown();
}

bool FOnlineSubsystemEOS::Tick(float DeltaTime)
{
	if (!bTickerStarted)
	{
		return true;
	}

	SessionInterfacePtr->Tick(DeltaTime);
	FOnlineSubsystemImpl::Tick(DeltaTime);

	return true;
}

bool FOnlineSubsystemEOS::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}

	bool bWasHandled = false;
	if (UserManager != nullptr && FParse::Command(&Cmd, TEXT("FRIENDS")))
	{
		bWasHandled = UserManager->HandleFriendsExec(InWorld, Cmd, Ar);
	}
	else if (StoreInterfacePtr != nullptr && FParse::Command(&Cmd, TEXT("ECOM")))
	{
		bWasHandled = StoreInterfacePtr->HandleEcomExec(InWorld, Cmd, Ar);
	}
	else if (TitleFileInterfacePtr != nullptr && FParse::Command(&Cmd, TEXT("TITLEFILE")))
	{
		bWasHandled = TitleFileInterfacePtr->HandleTitleFileExec(InWorld, Cmd, Ar);
	}
	else if (UserCloudInterfacePtr != nullptr && FParse::Command(&Cmd, TEXT("USERCLOUD")))
	{
		bWasHandled = UserCloudInterfacePtr->HandleUserCloudExec(InWorld, Cmd, Ar);
	}
	else
	{
		bWasHandled = false;
	}

	return bWasHandled;
}

void FOnlineSubsystemEOS::ReloadConfigs(const TSet<FString>& ConfigSections)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemEOS::ReloadConfigs"));

	// There is currently no granular reloading, so just restart the subsystem to pick up new config.
	const bool bWasInitialized = EOSPlatformHandle != nullptr;
	const bool bConfigChanged = ConfigSections.Find(GetDefault<UEOSSettings>()->GetClass()->GetPathName()) != nullptr;
	const bool bRestartRequired = bWasInitialized && bConfigChanged;

	if (bRestartRequired)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemEOS::ReloadConfigs: Restarting subsystem to pick up changes."));
		PreUnload();
		Shutdown();
	}

	// Notify user code so that overrides may be applied.
	TriggerOnConfigChangedDelegates(ConfigSections);

	// Reload config objects.
	if (bConfigChanged)
	{
		GetMutableDefault<UEOSSettings>()->ReloadConfig();
	}

	if (bRestartRequired)
	{
		Init();
	}
}

FString FOnlineSubsystemEOS::GetAppId() const
{
	return TEXT("");
}

FText FOnlineSubsystemEOS::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemEOS", "OnlineServiceName", "EOS");
}

FOnlineSubsystemEOS::FOnlineSubsystemEOS(FName InInstanceName) :
	IOnlineSubsystemEOS(EOS_SUBSYSTEM, InInstanceName)
	, EOSSDKManager(nullptr)
	, AuthHandle(nullptr)
	, UIHandle(nullptr)
	, FriendsHandle(nullptr)
	, UserInfoHandle(nullptr)
	, PresenceHandle(nullptr)
	, ConnectHandle(nullptr)
	, SessionsHandle(nullptr)
	, StatsHandle(nullptr)
	, LeaderboardsHandle(nullptr)
	, MetricsHandle(nullptr)
	, AchievementsHandle(nullptr)
	, EcomHandle(nullptr)
	, TitleStorageHandle(nullptr)
	, PlayerDataStorageHandle(nullptr)
	, UserManager(nullptr)
	, SessionInterfacePtr(nullptr)
	, LeaderboardsInterfacePtr(nullptr)
	, AchievementsInterfacePtr(nullptr)
	, StoreInterfacePtr(nullptr)
	, TitleFileInterfacePtr(nullptr)
	, UserCloudInterfacePtr(nullptr)
	, bWasLaunchedByEGS(false)
	, bIsDefaultOSS(false)
	, bIsPlatformOSS(false)
{
	StopTicker();
}

IOnlineSessionPtr FOnlineSubsystemEOS::GetSessionInterface() const
{
	return SessionInterfacePtr;
}

IOnlineFriendsPtr FOnlineSubsystemEOS::GetFriendsInterface() const
{
	return UserManager;
}

IOnlineSharedCloudPtr FOnlineSubsystemEOS::GetSharedCloudInterface() const
{
	UE_LOG_ONLINE(Error, TEXT("Shared Cloud Interface Requested"));
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemEOS::GetUserCloudInterface() const
{
	return UserCloudInterfacePtr;
}

IOnlineEntitlementsPtr FOnlineSubsystemEOS::GetEntitlementsInterface() const
{
	UE_LOG_ONLINE(Error, TEXT("Entitlements Interface Requested"));
	return nullptr;
};

IOnlineLeaderboardsPtr FOnlineSubsystemEOS::GetLeaderboardsInterface() const
{
	return LeaderboardsInterfacePtr;
}

IOnlineVoicePtr FOnlineSubsystemEOS::GetVoiceInterface() const
{
	return nullptr;
}

IOnlineExternalUIPtr FOnlineSubsystemEOS::GetExternalUIInterface() const
{
	return UserManager;
}

IOnlineIdentityPtr FOnlineSubsystemEOS::GetIdentityInterface() const
{
	return UserManager;
}

IOnlineTitleFilePtr FOnlineSubsystemEOS::GetTitleFileInterface() const
{
	return TitleFileInterfacePtr;
}

IOnlineStoreV2Ptr FOnlineSubsystemEOS::GetStoreV2Interface() const
{
	return StoreInterfacePtr;
}

IOnlinePurchasePtr FOnlineSubsystemEOS::GetPurchaseInterface() const
{
	return StoreInterfacePtr;
}

IOnlineAchievementsPtr FOnlineSubsystemEOS::GetAchievementsInterface() const
{
	return AchievementsInterfacePtr;
}

IOnlineUserPtr FOnlineSubsystemEOS::GetUserInterface() const
{
	return UserManager;
}

IOnlinePresencePtr FOnlineSubsystemEOS::GetPresenceInterface() const
{
	return UserManager;
}

IOnlineStatsPtr FOnlineSubsystemEOS::GetStatsInterface() const
{
	return StatsInterfacePtr;
}

IVoiceChatUser* FOnlineSubsystemEOS::GetVoiceChatUserInterface(const FUniqueNetId& LocalUserId)
{
	IVoiceChatUser* Result = nullptr;

#if WITH_EOS_RTC
	if (!VoiceChatInterface)
	{
		if (FEOSVoiceChatFactory* EOSVoiceChatFactory = FEOSVoiceChatFactory::Get())
		{
			VoiceChatInterface = EOSVoiceChatFactory->CreateInstanceWithPlatform(EOSPlatformHandle);
		}
	}

	if(VoiceChatInterface && UserManager->IsLocalUser(LocalUserId))
	{
		if (FOnlineSubsystemEOSVoiceChatUserWrapperRef* WrapperPtr = LocalVoiceChatUsers.Find(LocalUserId.AsShared()))
		{
			Result = &WrapperPtr->Get();
		}
		else
		{
			FEOSVoiceChatUser* VoiceChatUser = static_cast<FEOSVoiceChatUser*>(VoiceChatInterface->CreateUser());
			VoiceChatUser->Login(UserManager->GetPlatformUserIdFromUniqueNetId(LocalUserId), LexToString(FUniqueNetIdEOS::Cast(LocalUserId).GetProductUserId()), FString(), FOnVoiceChatLoginCompleteDelegate());

			const FOnlineSubsystemEOSVoiceChatUserWrapperRef& Wrapper = LocalVoiceChatUsers.Emplace(LocalUserId.AsShared(), MakeShared<FOnlineSubsystemEOSVoiceChatUserWrapper, ESPMode::ThreadSafe>(*VoiceChatUser));
			Result = &Wrapper.Get();
		}
	}
#endif // WITH_EOS_RTC

	return Result;
}

FEOSVoiceChatUser* FOnlineSubsystemEOS::GetEOSVoiceChatUserInterface(const FUniqueNetId& LocalUserId)
{
	FEOSVoiceChatUser* Result = nullptr;
#if WITH_EOS_RTC
	if (IVoiceChatUser* Wrapper = GetVoiceChatUserInterface(LocalUserId))
	{
		Result = &static_cast<FOnlineSubsystemEOSVoiceChatUserWrapper*>(Wrapper)->VoiceChatUser;
	}
#endif
	return Result;
}

void FOnlineSubsystemEOS::ReleaseVoiceChatUserInterface(const FUniqueNetId& LocalUserId)
{
#if WITH_EOS_RTC
	if (VoiceChatInterface)
	{
		if (FOnlineSubsystemEOSVoiceChatUserWrapperRef* WrapperPtr = LocalVoiceChatUsers.Find(LocalUserId.AsShared()))
		{
			VoiceChatInterface->ReleaseUser(&(**WrapperPtr).VoiceChatUser);
			LocalVoiceChatUsers.Remove(LocalUserId.AsShared());
		}
	}
#endif
}

#endif // WITH_EOS_SDK
