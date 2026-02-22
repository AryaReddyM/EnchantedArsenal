#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicComponent.generated.h"

class AArsenalCharacter;
class ASpell;
enum class ESpellType : uint8;

enum class ECastState : uint8 {
	Idle UMETA(DisplayName = "Idle"),
	Windup UMETA(DisplayName = "Windup"),
	Casting UMETA(DisplayName = "Casting"),
	Recovery UMETA(DisplayName = "Recovery"),
	Cooldown UMETA(DisplayName = "Cooldown")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ENCHANTEDARSENAL_API UMagicComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UMagicComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipSpell(ESpellType SpellType);
	void UnequipSpell();

	void Cast(bool bTriggered);
	UFUNCTION(Server, Reliable)
	void ServerCast(bool bTriggered);
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast(bool bTriggered);

	AArsenalCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnedSpell)
	ASpell* SpawnedSpell;
	UFUNCTION()
	void OnRep_SpawnedSpell();

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASpell> Boulder;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASpell> SpikeAdder;

	UPROPERTY(Replicated)
	bool bCasting;
};