#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Engine/TimerHandle.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API AProjectileWeapon : public AWeapon {
	GENERATED_BODY()

public:
	virtual void Shoot(const FVector HitTarget) override;
	virtual void StartShoot(const FVector& HitTarget) override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	FTimerHandle ShootTimerHandle;

	UPROPERTY(EditAnywhere)
	float ShootRate;
};
