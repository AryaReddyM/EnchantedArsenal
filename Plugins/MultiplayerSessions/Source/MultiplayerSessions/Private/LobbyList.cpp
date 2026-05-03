#include "LobbyList.h"

void ULobbyList::MenuSetup() {
	AddToViewport();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance) {
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ULobbyList::OnFindSessions);
	}
}

bool ULobbyList::Initialize() {
	if (!Super::Initialize()) {
		return false;
	}

	if (BackButton) {
		BackButton->OnClicked.AddDynamic(this, &ULobbyList::BackButtonClicked);
	}
	return true;
}

void ULobbyList::NativeDestruct() {
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(UpdateLobbyTimerHandle);
	}
	Super::NativeDestruct();
}

void ULobbyList::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful) {
	RebuildList(SessionResults);

	HideMenu.Broadcast();
	SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().SetTimer(UpdateLobbyTimerHandle, this, &ULobbyList::RequestRefresh, 2.0F, false);
	}
}

void ULobbyList::RebuildList(const TArray<FOnlineSessionSearchResult>& SessionResults) {
	if (!LobbyContainer) return;

	LobbyContainer->ClearChildren();

	if (!LobbyName) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0F, FColor::Red, TEXT("LobbyList: LobbyName widget class not set"));
		}
		return;
	}

	for (const FOnlineSessionSearchResult& Result : SessionResults) {
		ULobbyName* Row = CreateWidget<ULobbyName>(GetWorld(), LobbyName);
		if (!Row) continue;

		Row->Setup(Result, this);
		LobbyContainer->AddChild(Row);
	}
}

void ULobbyList::RequestRefresh() {
	if (GetVisibility() != ESlateVisibility::Visible) return;
	if (!MultiplayerSessionsSubsystem) return;

	MultiplayerSessionsSubsystem->FindSessions(10000);
}

void ULobbyList::JoinSpecificSession(const FOnlineSessionSearchResult& Result) {
	if (!MultiplayerSessionsSubsystem) return;

	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(UpdateLobbyTimerHandle);
	}

	MultiplayerSessionsSubsystem->JoinSession(Result);
}

void ULobbyList::BackButtonClicked() {
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(UpdateLobbyTimerHandle);
	}

	ShowMenu.Broadcast();
	SetVisibility(ESlateVisibility::Collapsed);
}
