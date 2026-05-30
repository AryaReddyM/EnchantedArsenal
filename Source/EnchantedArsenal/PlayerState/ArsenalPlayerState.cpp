#include "ArsenalPlayerState.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "GameFramework/Pawn.h"
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

bool AArsenalPlayerState::IsHostile(AActor* Instigator, AActor* Other) {
	if (!Other || Instigator == Other) return false;

	APawn* InstigPawn = Cast<APawn>(Instigator);
	APawn* OtherPawn = Cast<APawn>(Other);

	// Non-Pawns (floor, walls, props) are never enemies.
	if (!OtherPawn) return false;
	if (InstigPawn == OtherPawn) return false;

	AArsenalPlayerState* InstigPS = InstigPawn ? InstigPawn->GetPlayerState<AArsenalPlayerState>() : nullptr;
	AArsenalPlayerState* OtherPS = OtherPawn->GetPlayerState<AArsenalPlayerState>();

	// A Pawn with no team affiliation (e.g. an AI enemy with no PlayerState) is treated as hostile.
	if (!InstigPS || !OtherPS) return true;

	return InstigPS->GetTeam() != OtherPS->GetTeam();
}
