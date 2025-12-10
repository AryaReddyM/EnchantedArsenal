#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Weapon/Hitscan/HitscanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API AShotgun : public AHitscanWeapon {
	GENERATED_BODY()
	
public:
	virtual void Shoot() override;

	float Pellets = 9;
	TArray<FVector> HitLocations;
	float PelletAngle = 3.0f;
};
