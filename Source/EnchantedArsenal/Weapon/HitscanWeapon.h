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
	UFUNCTION(Server, Reliable)
	void ServerShoot(const FVector_NetQuantize& TraceEnd);

	void ServerProcessShot(const FVector& TraceStart, const FVector& TraceEnd);
	void LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd);

protected:
	FTimerHandle ShootTimerHandle;

	UPROPERTY(EditAnywhere)
	float ShootRate;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
};
