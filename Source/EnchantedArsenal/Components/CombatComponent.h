#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AArsenalCharacter;
class AArsenalPlayerController;
class AArsenalHUD;
class AWeapon;
enum class EWeaponType : uint8;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ENCHANTEDARSENAL_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()
	friend AArsenalCharacter;

public:	
	UCombatComponent();
	
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(EWeaponType WeaponType);
	void UnequipWeapon();

	void Shoot();
	void StopShoot();
	void FireOneShot();

	UFUNCTION()
	void HandleAmmoChanged(int32 NewAmmo, int32 MaxAmmo);
	void ResetAmmo();

	void PlayShootMontage();
	void PlayEquipMontage();
	void PlayReloadMontage();
	
	float GetEquipMontageLength();
	float GetReloadMontageLength();
	AWeapon* GetWeaponClass(EWeaponType Type) const;

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

	FTimerHandle ShootTimer;
	float LastShootTime = -1000.f;
	TMap<EWeaponType, int> AmmoReserve;
};
