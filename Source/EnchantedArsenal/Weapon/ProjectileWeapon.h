#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

struct FTimerHandle;

UCLASS()
class ENCHANTEDARSENAL_API AProjectileWeapon : public AWeapon {
	GENERATED_BODY()

public:
	virtual void Shoot() override;
	virtual void StartShoot() override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	FTimerHandle ShootTimerHandle;

	UPROPERTY(EditAnywhere)
	float ShootRate;
};
