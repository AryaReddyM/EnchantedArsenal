#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ArsenalGameState.generated.h"

enum class ETeam : uint8;
class AArsenalPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreChanged, ETeam, Team, float, NewScore);

UCLASS()
class ENCHANTEDARSENAL_API AArsenalGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetTeamScore(ETeam TeamToSet, float ScoreToAdd);
	float GetTeamScore(ETeam TeamToGet) const;

	TArray<AArsenalPlayerState*> RedTeam;
	TArray<AArsenalPlayerState*> BlueTeam;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.0F;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.0F;

	UFUNCTION()
	void OnRep_BlueTeamScore();

	UFUNCTION()
	void OnRep_RedTeamScore();

	UPROPERTY(BlueprintAssignable)
	FOnTeamScoreChanged OnTeamScoreChanged;
};