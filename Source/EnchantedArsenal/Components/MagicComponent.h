#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicComponent.generated.h"

class AArsenalCharacter;
class ASpell;
enum class ESpellType : uint8;

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

	AArsenalCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnedSpell)
	ASpell* SpawnedSpell;
	UFUNCTION()
	void OnRep_SpawnedSpell();


	UPROPERTY(EditAnywhere)
	TSubclassOf<ASpell> Boulder;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASpell> SpikeAdder;
};