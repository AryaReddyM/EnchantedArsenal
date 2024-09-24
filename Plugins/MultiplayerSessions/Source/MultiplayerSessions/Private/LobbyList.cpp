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

void ULobbyList::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful) {
	UpdateLobbyList();
	GetWorld()->GetTimerManager().SetTimer(UpdateLobbyTimerHandle, this, &ULobbyList::UpdateLobbyList, 1.0F, true);

	HideMenu.Broadcast();
	SetVisibility(ESlateVisibility::Visible);
}

void ULobbyList::UpdateLobbyList() {
	LobbyContainer->ClearChildren();

	for (auto Result : MultiplayerSessionsSubsystem->LastSessionSearch->SearchResults) {
		if (LobbyName) {
			LobbyNameRef = CreateWidget<ULobbyName>(GetWorld(), LobbyName);

			LobbyNameRef->LobbyJoinButton->OnClicked.AddDynamic(this, &ULobbyList::JoinClickedSession);
		}

		Username = Result.Session.OwningUserName;

		LobbyNameRef->UpdateLobby(Username);

		LobbyContainer->AddChild(LobbyNameRef);
	}
}

void ULobbyList::JoinClickedSession() {
	for (auto Result : MultiplayerSessionsSubsystem->LastSessionSearch->SearchResults) {
		if (LobbyNameRef->LobbyNameText->GetText().ToString() == Username + "'s Lobby") {
			GEngine->AddOnScreenDebugMessage(-1, 15.0F, FColor::Green, "Joining Session");
			MultiplayerSessionsSubsystem->JoinSession(Result);
		}
		else {
			GEngine->AddOnScreenDebugMessage(-1, 15.0F, FColor::Red, "Join Failed");
		}
	}
}

void ULobbyList::BackButtonClicked() {
	ShowMenu.Broadcast();
	SetVisibility(ESlateVisibility::Collapsed);
}
