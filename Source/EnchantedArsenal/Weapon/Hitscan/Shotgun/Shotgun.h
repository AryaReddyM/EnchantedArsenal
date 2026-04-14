#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Weapon/Hitscan/HitscanWeapon.h"
#include "Shotgun.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API AShotgun : public AHitscanWeapon {
	GENERATED_BODY()
	
public:
	virtual void Shoot() override;

	UPROPERTY(EditAnywhere)
	float Pellets;

	UPROPERTY(EditAnywhere)
	float PelletAngle;
};
