#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Weapon/Hitscan/HitscanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API AShotgun : public AHitscanWeapon {
	GENERATED_BODY()
	
public:
	virtual void Shoot() override;

	UFUNCTION(Server, Reliable)
	void ServerShotgunFire(FVector_NetQuantize CameraLoc, FVector_NetQuantize CrosshairImpact);

	void AuthoritativeShotgunFire(const FVector& CameraLoc, const FVector& CrosshairImpact);

	UPROPERTY(EditAnywhere)
	float Pellets;

	UPROPERTY(EditAnywhere)
	float PelletAngle;
};
