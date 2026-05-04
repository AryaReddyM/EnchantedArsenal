#include "TeamsGameMode.h"
#include "EnchantedArsenal/GameState/ArsenalGameState.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode() {
	GameStateClass = AArsenalGameState::StaticClass();
	PlayerStateClass = AArsenalPlayerState::StaticClass();
}

void ATeamsGameMode::GenericPlayerInitialization(AController* Controller) {
	Super::GenericPlayerInitialization(Controller);
	
	AArsenalGameState* ArsenalGameState = Cast<AArsenalGameState>(UGameplayStatics::GetGameState(this));
	if (ArsenalGameState) {

		AArsenalPlayerState* PlayerState = Controller->GetPlayerState<AArsenalPlayerState>();

		if (PlayerState && PlayerState->GetTeam() == ETeam::ET_NoTeam) {
			if (ArsenalGameState->RedTeam.Num() >= ArsenalGameState->BlueTeam.Num()) {
				ArsenalGameState->BlueTeam.AddUnique(PlayerState);
				PlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
			else {
				ArsenalGameState->RedTeam.AddUnique(PlayerState);
				PlayerState->SetTeam(ETeam::ET_RedTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting) {
	AArsenalGameState* ArsenalGameState = Cast<AArsenalGameState>(UGameplayStatics::GetGameState(this));
	AArsenalPlayerState* ArsenalPlayerState = Exiting->GetPlayerState<AArsenalPlayerState>();

	if (ArsenalGameState && ArsenalPlayerState) {
		if (ArsenalGameState->BlueTeam.Contains(ArsenalPlayerState)) {
			ArsenalGameState->BlueTeam.Remove(ArsenalPlayerState);
		}

		if (ArsenalGameState->RedTeam.Contains(ArsenalPlayerState)) {
			ArsenalGameState->RedTeam.Remove(ArsenalPlayerState);
		}
	}

}

void ATeamsGameMode::HandleMatchHasStarted() {
	Super::HandleMatchHasStarted();

	AArsenalGameState* ArsenalGameState = Cast<AArsenalGameState>(UGameplayStatics::GetGameState(this));

	if (ArsenalGameState) {
		for (auto PS : ArsenalGameState->PlayerArray) {
			AArsenalPlayerState* PlayerState = Cast<AArsenalPlayerState>(PS.Get());

			if (PlayerState && PlayerState->GetTeam() == ETeam::ET_NoTeam) {
				if (ArsenalGameState->RedTeam.Num() >= ArsenalGameState->BlueTeam.Num()) {
					ArsenalGameState->BlueTeam.AddUnique(PlayerState);
					PlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
				else {
					ArsenalGameState->RedTeam.AddUnique(PlayerState);
					PlayerState->SetTeam(ETeam::ET_RedTeam);
				}
			}
		}
	}
}