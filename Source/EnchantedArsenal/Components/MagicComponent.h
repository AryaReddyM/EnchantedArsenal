#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicComponent.generated.h"

class ASpellVisual;
class AArsenalCharacter;
class ASpell;
class USpellData;
class USpellInstance;

UENUM(BlueprintType)
enum class ESpellType : uint8 {
	EST_None UMETA(DisplayName = "None"),
	EST_Boulder UMETA(DisplayName = "Boulder"),
	EST_SpikeAdder UMETA(DisplayName = "Spike Adder"),
	EST_MAX UMETA(DisplayName = "DefaultMax")
};

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

	void EquipSpell(ESpellType SpellType);
	void UnequipSpell();

	void Cast(bool bTriggered);
	UFUNCTION(Server, Reliable)
	void ServerCast(bool bTriggered);
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast(bool bTriggered);

	USpellData* GetSpellDataForType(ESpellType SpellType) const;

	AArsenalCharacter* Character;
	
	UPROPERTY(Replicated)
	bool bCasting;
	
	UPROPERTY(ReplicatedUsing=OnRep_CastState)
	ECastState CastState = ECastState::Idle;
	UFUNCTION()
	void OnRep_CastState();

	UPROPERTY(ReplicatedUsing=OnRep_EquippedSpellType)
	ESpellType EquippedSpellType = ESpellType::EST_None;
	UFUNCTION()
	void OnRep_EquippedSpellType();
	
	UPROPERTY()
	USpellInstance* ActiveSpell;
	
	UPROPERTY(ReplicatedUsing=OnRep_SpawnedSpell)
	ASpell* SpawnedSpell;
	UFUNCTION()
	void OnRep_SpawnedSpell();
	
	UPROPERTY(ReplicatedUsing=OnRep_SpawnedVisual)
	ASpellVisual* SpawnedVisual;
	UFUNCTION()
	void OnRep_SpawnedVisual();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spells")
	USpellData* Boulder;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spells")
	USpellData* SpikeAdder;

	float LastCastTime = -1000.f;
};