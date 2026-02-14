#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000;

class AArsenalCharacter;
class AArsenalPlayerController;
class AArsenalHUD;
class AWeapon;
enum class EWeaponType : uint8;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ENCHANTEDARSENAL_API UCombatComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	friend AArsenalCharacter;
protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(EWeaponType WeaponType);
	void DestroyWeapon();
	
	void Shoot(bool bTriggered);
	UFUNCTION(Server, Reliable)
	void ServerShoot(bool bTriggered);
	UFUNCTION(NetMulticast, Reliable)
	void MultiShoot(bool bTriggered);

	FHitResult TraceUnderCrosshairs();

	void SetSemiCounter(int Counter);
	UFUNCTION(Server, Reliable)
	void ServerSetSemiCounter(int32 NewCounter);

	void ResetSemiCounter();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerResetSemiCounter();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetSemiCounter();

	void PlayShootMontage();
	void PlayEquipMontage();

	AArsenalCharacter* Character;
	AArsenalPlayerController* PlayerController;
	AArsenalHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnedWeapon)
	AWeapon* SpawnedWeapon;
	UFUNCTION()
	void OnRep_SpawnedWeapon();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> Rifle;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> Shotgun;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> Pistol;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> SMG;

	UPROPERTY(Replicated)
	bool bShooting;

	UPROPERTY(ReplicatedUsing = OnRep_SemiShotCounter)
	int SemiShotCounter = 0;

	UFUNCTION()
	void OnRep_SemiShotCounter();

	bool bIsRecentlyEquipped = false;
};
