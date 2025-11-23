#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitscanWeapon.generated.h"

class USoundCue;

UCLASS()
class ENCHANTEDARSENAL_API AHitscanWeapon : public AWeapon {
	GENERATED_BODY()

public:
	virtual void Shoot() override;
	virtual void StartShoot() override;

protected:
	FTimerHandle ShootTimerHandle;

	UPROPERTY(EditAnywhere)
	float ShootRate;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
};
