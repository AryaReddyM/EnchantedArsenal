#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimMontage.h"
#include "MagicComponent.generated.h"

class AArsenalCharacter;
class ASpell;
class USpellData;

UENUM(BlueprintType)
enum class ESpellType : uint8 {
	EST_None UMETA(DisplayName = "None"),
	EST_Boulder UMETA(DisplayName = "Boulder"),
	EST_SpikeAdder UMETA(DisplayName = "Spike Adder"),
	EST_FireWall UMETA(DisplayName = "Fire Wall"),
	EST_MAX UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class ECastState : uint8 {
	ECS_Idle UMETA(DisplayName = "Idle"),
	ECS_Casting UMETA(DisplayName = "Casting")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ENCHANTEDARSENAL_API UMagicComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UMagicComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipSpell(ESpellType SpellType);
	void UnequipSpell();
	void Cast();

	UFUNCTION(Server, Reliable)
	void ServerCast();

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast();
	
	UFUNCTION(Client, Reliable)
	void ClientResetCastState();

	UFUNCTION(Server, Reliable)
	void ServerReleaseSpell(FVector_NetQuantize LaunchLocation, FVector_NetQuantizeNormal LaunchDir);

	bool IsSpellOnCooldown(ESpellType SpellType) const;

	virtual void BeginPlay() override;

	void SpawnHeldSpell();
	void AttachHeldSpell();
	AArsenalCharacter* GetCharacter() const;
	USpellData* GetSpellDataForType(ESpellType SpellType) const;
	
	void PlayCastMontage();
	void PlayEquipMontage();
	
	float GetCastMontageLength();
	float GetEquipMontageLength();
	
	UTexture2D* GetSpellIconForType(ESpellType Type);

	UFUNCTION()
	void OnCastNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void OnCastMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void RequestRelease();

	bool bHasPendingCast = false;

	UPROPERTY(Replicated)
	ECastState CastState = ECastState::ECS_Idle;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedSpellType)
	ESpellType EquippedSpellType = ESpellType::EST_None;

	UFUNCTION()
	void OnRep_EquippedSpellType();

	UPROPERTY()
	USpellData* SpellData;

	UPROPERTY(ReplicatedUsing=OnRep_HeldSpell)
	ASpell* HeldSpell;

	UFUNCTION()
	void OnRep_HeldSpell();

	UPROPERTY(EditAnywhere, Category = "Spells")
	USpellData* Boulder;

	UPROPERTY(EditAnywhere, Category = "Spells")
	USpellData* SpikeAdder;
	
	UPROPERTY(EditAnywhere, Category = "Spells")
	USpellData* FireWall;

	TMap<ESpellType, float> SpellCooldownDurations;
	TMap<ESpellType, FTimerHandle> CooldownTimers;
};
