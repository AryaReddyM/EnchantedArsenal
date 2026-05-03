// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
			
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);

		// EOS requires the player to be authenticated before any session
		// operation will succeed. Kick off login as soon as the menu opens.
		if (!MultiplayerSessionsSubsystem->IsLoggedIn())
		{
			MultiplayerSessionsSubsystem->Login();
		}
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton) {
		HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	}
	if (JoinButton) {
		JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Failed to create session!"));
		}
		if (HostButton) HostButton->SetIsEnabled(true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (LobbyMap.IsNull())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
				TEXT("LobbyMap is not set on this Menu widget. Open BP_Menu, set the LobbyMap property to your lobby map asset."));
		}
		if (HostButton) HostButton->SetIsEnabled(true);
		return;
	}

	// Resolve the asset reference to a package path like /Game/Maps/Gameplay/LobbyMap
	const FString MapPackagePath = LobbyMap.GetLongPackageName();
	const FString TravelURL = FString::Printf(TEXT("%s?listen"), *MapPackagePath);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow,
			FString::Printf(TEXT("Traveling to lobby: %s"), *TravelURL));
	}

	if (!World->ServerTravel(TravelURL))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
				FString::Printf(TEXT("ServerTravel failed. Check that '%s' is cooked into the build."), *MapPackagePath));
		}
		if (HostButton) HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	// LobbyList owns the actual list rendering. The Menu only needs to
	// re-enable its Join button when the search produced nothing usable,
	// so the user can try again.
	if (MultiplayerSessionsSubsystem == nullptr || !bWasSuccessful || SessionResults.Num() == 0) {
		if (JoinButton) JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MPSession] OnJoinSession Result=%d"), (int32)Result);
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("[Join] Result=%d"), (int32)Result));
	}

	if (Result != EOnJoinSessionCompleteResult::Success
		&& Result != EOnJoinSessionCompleteResult::AlreadyInSession) {
		if (JoinButton) JoinButton->SetIsEnabled(true);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) {
		UE_LOG(LogTemp, Error, TEXT("[MPSession] OnJoinSession: no OSS"));
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid()) {
		UE_LOG(LogTemp, Error, TEXT("[MPSession] OnJoinSession: no SessionInterface"));
		return;
	}

	FString Address;
	const bool bGotAddr = SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
	UE_LOG(LogTemp, Warning, TEXT("[MPSession] GetResolvedConnectString ok=%d Address=%s"),
		bGotAddr ? 1 : 0, *Address);
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
			FString::Printf(TEXT("[Join] addr ok=%d %s"), bGotAddr ? 1 : 0, *Address));
	}

	if (!bGotAddr || Address.IsEmpty()) {
		if (JoinButton) JoinButton->SetIsEnabled(true);
		return;
	}

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (PlayerController) {
		PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
	if (!MultiplayerSessionsSubsystem) return;

	if (!MultiplayerSessionsSubsystem->IsLoggedIn()) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
				TEXT("Not logged in yet. Retrying login..."));
		}
		MultiplayerSessionsSubsystem->Login();
		return;
	}

	HostButton->SetIsEnabled(false);
	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::JoinButtonClicked() {
	if (!MultiplayerSessionsSubsystem) return;

	if (!MultiplayerSessionsSubsystem->IsLoggedIn()) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
				TEXT("Not logged in yet. Retrying login..."));
		}
		MultiplayerSessionsSubsystem->Login();
		return;
	}

	JoinButton->SetIsEnabled(false);
	MultiplayerSessionsSubsystem->FindSessions(10000, MatchType);
}


void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void UMenu::CollapseMenu() {
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMenu::ShowMenu() {
	SetVisibility(ESlateVisibility::Visible);
	JoinButton->SetIsEnabled(true);
}
