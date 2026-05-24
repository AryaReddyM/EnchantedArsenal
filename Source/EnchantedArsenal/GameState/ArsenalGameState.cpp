#include "ArsenalGameState.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "Net/UnrealNetwork.h"

class UTextBlock;

void AArsenalGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalGameState, BlueTeamScore);
	DOREPLIFETIME(AArsenalGameState, RedTeamScore);
}

void AArsenalGameState::SetTeamScore(ETeam TeamToSet, float ScoreToAdd) {
	switch (TeamToSet) {
	case ETeam::ET_BlueTeam:
		BlueTeamScore += ScoreToAdd;
		UpdateTeamScoreUI(ETeam::ET_BlueTeam, BlueTeamScore);
		break;
	case ETeam::ET_RedTeam:
		RedTeamScore += ScoreToAdd;
		UpdateTeamScoreUI(ETeam::ET_RedTeam, RedTeamScore);
		break;
	default:
		break;
	}
}

float AArsenalGameState::GetTeamScore(ETeam TeamToGet) const {
	return TeamToGet == ETeam::ET_BlueTeam ? BlueTeamScore : RedTeamScore;
}

void AArsenalGameState::UpdateTeamScoreUI(ETeam TeamToUpdate, float NewScore) {
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	AArsenalCharacter* Character = Cast<AArsenalCharacter>(PC->GetPawn());
	if (!Character || !Character->HUD) return;

	FString WidgetName = (TeamToUpdate == ETeam::ET_BlueTeam) ? "BlueScoreText" : "RedScoreText";
	FString LabelText = (TeamToUpdate == ETeam::ET_BlueTeam) ? "Blue Score: {0}" : "Red Score: {0}";
    
	if (UTextBlock* ScoreTextBlock = Cast<UTextBlock>(Character->HUD->GetWidgetFromName(*WidgetName))) {
		ScoreTextBlock->SetText(FText::Format(FText::FromString(LabelText), FMath::FloorToInt(NewScore)));
	}
}

void AArsenalGameState::OnRep_BlueTeamScore() {
	UpdateTeamScoreUI(ETeam::ET_BlueTeam, BlueTeamScore);
}

void AArsenalGameState::OnRep_RedTeamScore() {
	UpdateTeamScoreUI(ETeam::ET_RedTeam, RedTeamScore);
}