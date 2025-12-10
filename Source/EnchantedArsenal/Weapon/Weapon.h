#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetWeaponMesh);

UENUM(BlueprintType)
enum class EWeaponType : uint8 {
	EWT_Initial UMETA(DisplayName = "Initial Type"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SMG UMETA(DisplayName = "Sub-Machine Gun"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	
	EWT_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class ENCHANTEDARSENAL_API AWeapon : public AActor {
	GENERATED_BODY()
	
public:
	AWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void Shoot(); 
	virtual void StartShoot();
	virtual void StopShoot();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", Replicated)
	EWeaponType WeaponType;

	UPROPERTY(BlueprintAssignable)
	FSetWeaponMesh SetWeaponMesh;

public:
	void SetWeaponType(EWeaponType InWeaponState);

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh;  }

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsUp;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsDown;

	float LastFireTime = -1000.f;
};
