#include "ArsenalGameState.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "Net/UnrealNetwork.h"

void AArsenalGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalGameState, BlueTeamScore);
	DOREPLIFETIME(AArsenalGameState, RedTeamScore);
}

void AArsenalGameState::OnRep_BlueTeamScore() {
}

void AArsenalGameState::OnRep_RedTeamScore() {
}