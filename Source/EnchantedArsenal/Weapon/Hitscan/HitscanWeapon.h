#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "HitscanWeapon.generated.h"

class USoundCue;

UCLASS()
class ENCHANTEDARSENAL_API AHitscanWeapon : public AWeapon {
	GENERATED_BODY()

public:
	virtual void Shoot() override;
	UFUNCTION(Server, Reliable)
	void ServerShoot(bool bHitSomething, const FVector_NetQuantize& ImpactPoint);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpactEffects(FVector_NetQuantize ImpactPoint);

	void ServerProcessShot(bool bHitSomething, const FVector& ImpactPoint);
	void LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& CrosshairHitResult);

	bool CheckForHeadshot(AActor* HitActor, FVector ImpactPoint);

protected:
	FTimerHandle ShootTimerHandle;
};
