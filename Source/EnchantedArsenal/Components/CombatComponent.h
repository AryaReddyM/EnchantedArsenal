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

	AArsenalCharacter* Character;
	AArsenalPlayerController* PlayerController;
	AArsenalHUD* HUD;

	UPROPERTY(Replicated)
	AWeapon* SpawnedWeapon;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> Rifle;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> SMG;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;

	bool bShootButtonPressed;
};
