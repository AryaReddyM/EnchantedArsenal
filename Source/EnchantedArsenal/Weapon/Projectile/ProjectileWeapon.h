#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

struct FTimerHandle;

UCLASS()
class ENCHANTEDARSENAL_API AProjectileWeapon : public AWeapon {
	GENERATED_BODY()

public:
	virtual void Shoot() override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	FTimerHandle ShootTimerHandle;
};
