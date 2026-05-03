// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete)) {

}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType) {
	if (!IsValidSessionInterface()) {
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr) {
		// A session already exists — destroy it first, then let
		// OnDestroySessionComplete re-call CreateSession.
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
		return;
	}

	LastMatchType = MatchType;

	// Store the delegate in a FDelegateHandle so we can later remove it from the delegate list
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	const FString OSSName = IOnlineSubsystem::Get() ? IOnlineSubsystem::Get()->GetSubsystemName().ToString() : FString();
	const bool bIsNullOSS = OSSName == TEXT("NULL");

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = bIsNullOSS;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	FString HostName;
	if (!IdentityInterface.IsValid()) {
		if (IOnlineSubsystem* Sub = IOnlineSubsystem::Get()) {
			IdentityInterface = Sub->GetIdentityInterface();
		}
	}
	if (IdentityInterface.IsValid()) {
		HostName = IdentityInterface->GetPlayerNickname(0);
	}
	if (HostName.IsEmpty()) {
		HostName = TEXT("Unknown");
	}
	LastSessionSettings->Set(FName("HostName"), HostName, EOnlineDataAdvertisementType::ViaOnlineService);

	UE_LOG(LogTemp, Warning, TEXT("[MPSession] CreateSession: OSS=%s bIsLAN=%d BuildUniqueId=%d NumConn=%d MatchType=%s"),
		*OSSName, bIsNullOSS ? 1 : 0, LastSessionSettings->BuildUniqueId, NumPublicConnections, *MatchType);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	if (!NetId.IsValid()) {
		// Most common cause: EOS login hasn't completed (or failed). Without
		// a valid net id, dereferencing it crashes with an IsValid() assert.
		UE_LOG(LogTemp, Error, TEXT("[MPSession] CreateSession aborted: no valid local UniqueNetId. Are you logged in?"));
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("[Create] Cannot host: not logged in to EOS."));
		}
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionInterface->CreateSession(*NetId, NAME_GameSession, *LastSessionSettings)) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults, FString MatchType) {
	if (!IsValidSessionInterface()) {
		return;
	}

	// Remember the filter so refresh calls (e.g. from LobbyList) can reuse it
	// without having to re-supply it.
	if (!MatchType.IsEmpty()) {
		LastMatchType = MatchType;
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	const FString OSSName = IOnlineSubsystem::Get() ? IOnlineSubsystem::Get()->GetSubsystemName().ToString() : FString();
	const bool bIsNullOSS = OSSName == TEXT("NULL");

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = bIsNullOSS;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	if (!LastMatchType.IsEmpty()) {
		LastSessionSearch->QuerySettings.Set(FName("MatchType"), LastMatchType, EOnlineComparisonOp::Equals);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MPSession] FindSessions: OSS=%s bIsLAN=%d BuildUniqueId=%d MaxResults=%d MatchType=%s"),
		*OSSName, bIsNullOSS ? 1 : 0, GetBuildUniqueId(), MaxSearchResults, *LastMatchType);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	if (!NetId.IsValid()) {
		UE_LOG(LogTemp, Error, TEXT("[MPSession] FindSessions aborted: no valid local UniqueNetId. Are you logged in?"));
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("[Find] Cannot search: not logged in to EOS."));
		}
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	if (!SessionInterface->FindSessions(*NetId, LastSessionSearch.ToSharedRef())) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult) {
	if (!SessionInterface.IsValid()) {
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	if (!NetId.IsValid()) {
		UE_LOG(LogTemp, Error, TEXT("[MPSession] JoinSession aborted: no valid local UniqueNetId. Are you logged in?"));
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
				TEXT("[Join] Cannot join: not logged in to EOS."));
		}
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	if (!SessionInterface->JoinSession(*NetId, NAME_GameSession, SessionResult)) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession() {
	if (!SessionInterface.IsValid()) {
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession)) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession() {
	if (!IsValidSessionInterface()) {
		MultiplayerOnStartSessionComplete.Broadcast(false);
		return;
	}

	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	if (!SessionInterface->StartSession(NAME_GameSession)) {
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

bool UMultiplayerSessionsSubsystem::IsValidSessionInterface() {
	if (!SessionInterface) {
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (Subsystem) {
			SessionInterface = Subsystem->GetSessionInterface();
		}
	}
	return SessionInterface.IsValid();
}

void UMultiplayerSessionsSubsystem::Login() {
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) {
		MultiplayerOnLoginComplete.Broadcast(false);
		return;
	}

	IdentityInterface = Subsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid()) {
		MultiplayerOnLoginComplete.Broadcast(false);
		return;
	}

	if (IsLoggedIn()) {
		MultiplayerOnLoginComplete.Broadcast(true);
		return;
	}

	LoginCompleteDelegateHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(
		0, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnLoginComplete));

	FOnlineAccountCredentials Creds;
	Creds.Type = TEXT("accountportal"); 
	Creds.Id = TEXT("");
	Creds.Token = TEXT("");

	if (!IdentityInterface->Login(0, Creds)) {
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegateHandle);
		MultiplayerOnLoginComplete.Broadcast(false);
	}
}

bool UMultiplayerSessionsSubsystem::IsLoggedIn() const {
	if (!IdentityInterface.IsValid()) {
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (!Subsystem) return false;
		const_cast<UMultiplayerSessionsSubsystem*>(this)->IdentityInterface = Subsystem->GetIdentityInterface();
		if (!IdentityInterface.IsValid()) return false;
	}
	return IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UMultiplayerSessionsSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error) {
	if (IdentityInterface.IsValid()) {
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MPSession] Login complete. bSuccess=%d Error=%s"),
		bWasSuccessful ? 1 : 0, *Error);

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, bWasSuccessful ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("[Login] bSuccess=%d %s"), bWasSuccessful ? 1 : 0, *Error));
	}

	MultiplayerOnLoginComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	const FString SubName = IOnlineSubsystem::Get() ? IOnlineSubsystem::Get()->GetSubsystemName().ToString() : TEXT("<none>");

	UE_LOG(LogTemp, Warning,
		TEXT("[MPSession] CreateSession complete. bSuccess=%d OSS=%s MatchType=%s"),
		bWasSuccessful ? 1 : 0, *SubName, *LastMatchType);

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("[Create] bSuccess=%d OSS=%s MatchType=%s"),
				bWasSuccessful ? 1 : 0, *SubName, *LastMatchType));
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	TArray<FOnlineSessionSearchResult> Filtered;
	int32 RawCount = 0;
	int32 RejectedMatchType = 0;

	if (LastSessionSearch.IsValid()) {
		RawCount = LastSessionSearch->SearchResults.Num();
		Filtered.Reserve(RawCount);

		for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults) {
			FString FoundMatchType;
			Result.Session.SessionSettings.Get(FName("MatchType"), FoundMatchType);

			UE_LOG(LogTemp, Warning, TEXT("[MPSession] Raw result: Owner=%s MatchType=%s"),
				*Result.Session.OwningUserName,
				*FoundMatchType);

			if (!LastMatchType.IsEmpty() && FoundMatchType != LastMatchType) {
				++RejectedMatchType;
				continue;
			}

			Filtered.Add(Result);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[MPSession] FindSessions complete. bSuccess=%d Raw=%d ExpectMatchType=%s -> Kept=%d (RejectedMatchType=%d)"),
		bWasSuccessful ? 1 : 0,
		RawCount,
		*LastMatchType,
		Filtered.Num(),
		RejectedMatchType);

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("[Find] Raw=%d Kept=%d (MatchType=%s)"),
				RawCount, Filtered.Num(), *LastMatchType));
	}

	MultiplayerOnFindSessionsComplete.Broadcast(Filtered, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	if (SessionInterface) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	if (bWasSuccessful && bCreateSessionOnDestroy) {
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}
	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}
