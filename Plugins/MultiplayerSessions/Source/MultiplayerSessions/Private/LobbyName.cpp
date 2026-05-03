// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyName.h"
#include "LobbyList.h"

void ULobbyName::Setup(const FOnlineSessionSearchResult& InResult, ULobbyList* InOwner) {
	Result = InResult;
	Owner = InOwner;

	if (LobbyNameText) {
		FString DisplayName;
		Result.Session.SessionSettings.Get(FName("HostName"), DisplayName);
		if (DisplayName.IsEmpty()) {
			DisplayName = Result.Session.OwningUserName;
		}
		if (DisplayName.IsEmpty()) {
			DisplayName = TEXT("Unknown");
		}
		LobbyNameText->SetText(FText::FromString(DisplayName + TEXT("'s Lobby")));
	}

	if (LobbyJoinButton) {
		LobbyJoinButton->OnClicked.RemoveDynamic(this, &ULobbyName::OnJoinClicked);
		LobbyJoinButton->OnClicked.AddDynamic(this, &ULobbyName::OnJoinClicked);
	}
}

void ULobbyName::OnJoinClicked() {
	if (Owner) {
		Owner->JoinSpecificSession(Result);
	}
}
