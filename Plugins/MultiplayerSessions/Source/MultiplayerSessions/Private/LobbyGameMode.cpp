// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer) {
	Super::PostLogin(NewPlayer);

	if (bTravelInProgress) return;
	if (!GameState) return;

	const int32 NumberOfPlayers = GameState->PlayerArray.Num();
	if (NumberOfPlayers < RequiredPlayersToStart) return;

	bTravelInProgress = true;

	UGameInstance* GameInstance = GetGameInstance();
	UMultiplayerSessionsSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>() : nullptr;

	if (Subsystem) {
		Subsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ALobbyGameMode::OnSessionStarted);
		Subsystem->StartSession();
	}
	else {
		TravelToMatch();
	}
}

void ALobbyGameMode::OnSessionStarted(bool bWasSuccessful) {
	if (UGameInstance* GameInstance = GetGameInstance()) {
		if (UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>()) {
			Subsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ALobbyGameMode::OnSessionStarted);
		}
	}

	TravelToMatch();
}

void ALobbyGameMode::TravelToMatch() {
	UWorld* World = GetWorld();
	if (!World) return;

	bUseSeamlessTravel = true;
	World->ServerTravel(FString::Printf(TEXT("%s?listen"), *MatchMapPath));
}
