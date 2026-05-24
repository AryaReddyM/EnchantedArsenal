#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UTexture2D;
class USoundCue;
class UParticleSystem;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetWeaponMesh);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, NewAmmo, int32, MagSize);

UENUM(BlueprintType)
enum class EFireType : uint8 {
	EFT_Initial UMETA(DisplayName = "Initial Type"),
	EFT_Auto UMETA(DisplayName = "Automatic"),
	EFT_SemiAuto UMETA(DisplayName = "Semi-Automatic"),

	EFT_MAX UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8 {
	EWT_Unarmed UMETA(DisplayName = "Unarmed"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SMG UMETA(DisplayName = "Sub-Machine Gun"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	
	EWT_MAX UMETA(DisplayName = "DefaultMax")
};
ENUM_RANGE_BY_COUNT(EWeaponType, EWeaponType::EWT_MAX);

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
	
	void Reload();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USceneComponent* BaseRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* GripPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", ReplicatedUsing = OnRep_WeaponType)
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

	UPROPERTY(EditAnywhere)
	float ShootRate;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlashParticles;

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

	UPROPERTY(EditAnywhere)
	float EquipDelay = 2.0f;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ShootMontage;
	UPROPERTY(EditAnywhere)
	UAnimMontage* EquipMontage;
	
	UPROPERTY(EditDefaultsOnly)
	int32 MagSize = 30;

	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnAmmoChanged;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo)
	int32 CurrentAmmo;
	UFUNCTION()
	void OnRep_CurrentAmmo();

	bool TryConsumeAmmo(int AmmoAmount);
	
	UPROPERTY(EditAnywhere)
	float CrosshairWeaponBaseSpread = 15.f;
	UPROPERTY(EditAnywhere)
	float CrosshairMovementEffect = 5.f;
	UPROPERTY(EditAnywhere)
	float CrosshairInterpSpeed = 15.f;
};