#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicComponent.generated.h"

class AArsenalCharacter;
class ASpell;
class USpellData;

UENUM(BlueprintType)
enum class ECastState : uint8 {
	Idle UMETA(DisplayName = "Idle"),
	Casting UMETA(DisplayName = "Casting"),
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

	void EquipSpell(USpellData* SpellData);
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

	UPROPERTY(Replicated)
	bool bCasting;
	
	UPROPERTY(ReplicatedUsing=OnRep_CastState)
	ECastState CastState = ECastState::Idle;
	UFUNCTION()
	void OnRep_CastState();
};