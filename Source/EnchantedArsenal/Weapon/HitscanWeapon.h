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
	void ServerShoot(bool bHitSomething, const FVector_NetQuantize& ImpactPoint);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpactEffects(FVector_NetQuantize ImpactPoint);

	void ServerProcessShot(bool bHitSomething, const FVector& ImpactPoint);
	void LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& CrosshairHitResult);

	bool CheckForHeadshot(AActor* HitActor, FVector ImpactPoint);

protected:
	FTimerHandle ShootTimerHandle;

	UPROPERTY(EditAnywhere)
	float ShootRate;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlashParticles;
	FVector MuzzleLocation;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	float Damage;

	UPROPERTY(EditAnywhere)
	float HeadshotDamage;

	UPROPERTY(EditAnywhere)
	float RecoilMin;

	UPROPERTY(EditAnywhere)
	float RecoilMax;
};
