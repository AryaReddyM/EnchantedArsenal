#include "ArsenalPlayerState.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Net/UnrealNetwork.h"

void AArsenalPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalPlayerState, Team);
}

void AArsenalPlayerState::OnRep_Team() {
	Character = Character == nullptr ? Cast<AArsenalCharacter>(GetPawn()) : Character;
	if (Character) {
		Character->SetTeamColor(Team);
	}
}

void AArsenalPlayerState::SetTeam(ETeam TeamToSet) {
	Team = TeamToSet;

	Character = Character == nullptr ? Cast<AArsenalCharacter>(GetPawn()) : Character;
	if (Character) {
		Character->SetTeamColor(Team);
	}
}