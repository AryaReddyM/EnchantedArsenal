#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ArsenalPlayerState.generated.h"

class AArsenalCharacter;

UENUM(BlueprintType)
enum class ETeam : uint8 {
	ET_BlueTeam UMETA(DisplayName = "BlueTeam"),
	ET_RedTeam UMETA(DisplayName = "RedTeam"),
	ET_NoTeam UMETA(DisplayName = "NoTeam"),

	ET_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class ENCHANTEDARSENAL_API AArsenalPlayerState : public APlayerState {
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	AArsenalCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
	void SetTeam(ETeam TeamToSet);

	static bool IsHostile(AActor* Instigator, AActor* Other);
};