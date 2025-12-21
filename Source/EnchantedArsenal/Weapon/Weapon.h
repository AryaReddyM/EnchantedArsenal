#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetWeaponMesh);

UENUM(BlueprintType)
enum class EFireType : uint8 {
	EWT_Initial UMETA(DisplayName = "Initial Type"),
	EWT_Auto UMETA(DisplayName = "Automatic"),
	EWT_SemiAuto UMETA(DisplayName = "Semi-Automatic"),

	EWT_MAX UMETA(DisplayName = "DefaultMax")
};

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

	void SetWeaponType(EWeaponType InWeaponState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* GripPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", ReplicatedUsing = OnRep_WeaponType)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", ReplicatedUsing = OnRep_FireType)
	EFireType FireType;

	UFUNCTION()
	void OnRep_WeaponType();

	UFUNCTION()
	void OnRep_FireType();

	UPROPERTY(BlueprintAssignable)
	FSetWeaponMesh SetWeaponMesh;

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

	float LastFireTime = -1000.f;

	UPROPERTY(EditAnywhere)
	float EquipDelay;
};
