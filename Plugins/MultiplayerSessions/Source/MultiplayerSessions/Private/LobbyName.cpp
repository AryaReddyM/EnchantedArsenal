// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyName.h"

void ULobbyName::UpdateLobby(FString Username) {
	LobbyNameText->SetText(FText::FromString(Username + "'s Lobby"));
}