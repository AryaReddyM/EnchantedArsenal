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

	void SetAiming(bool bInAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bInAiming);
	
	void Shoot(bool bTriggered);
	UFUNCTION(Server, Reliable)
	void ServerShoot();
	UFUNCTION(NetMulticast, Reliable)
	void MultiShoot();

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetSemiCounter(int Counter);
	UFUNCTION(Server, Reliable)
	void ServerSetSemiCounter(int32 NewCounter);

	void ResetSemiCounter();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerResetSemiCounter();

	bool CanShoot();

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
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;

	bool bShootButtonPressed;

	float LastEquipTime = -1000.f;

	UPROPERTY(ReplicatedUsing = OnRep_SemiShotCounter)
	int SemiShotCounter = 0;

	UFUNCTION()
	void OnRep_SemiShotCounter();
};
