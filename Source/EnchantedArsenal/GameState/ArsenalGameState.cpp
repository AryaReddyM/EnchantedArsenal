#include "ArsenalGameState.h"

#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "Net/UnrealNetwork.h"

void AArsenalGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalGameState, BlueTeamScore);
	DOREPLIFETIME(AArsenalGameState, RedTeamScore);
}

void AArsenalGameState::SetTeamScore(ETeam TeamToSet, float ScoreToAdd) {
	switch (TeamToSet) {
	case ETeam::ET_BlueTeam:
		BlueTeamScore += ScoreToAdd;
		OnTeamScoreChanged.Broadcast(ETeam::ET_BlueTeam, BlueTeamScore);
		break;
	case ETeam::ET_RedTeam:
		RedTeamScore += ScoreToAdd;
		OnTeamScoreChanged.Broadcast(ETeam::ET_RedTeam, RedTeamScore);
		break;
	default:
		break;
	}
}

float AArsenalGameState::GetTeamScore(ETeam TeamToGet) const {
	return TeamToGet == ETeam::ET_BlueTeam ? BlueTeamScore : RedTeamScore;
}

void AArsenalGameState::OnRep_BlueTeamScore() {
	OnTeamScoreChanged.Broadcast(ETeam::ET_BlueTeam, BlueTeamScore);
}

void AArsenalGameState::OnRep_RedTeamScore() {
	OnTeamScoreChanged.Broadcast(ETeam::ET_RedTeam, RedTeamScore);
}