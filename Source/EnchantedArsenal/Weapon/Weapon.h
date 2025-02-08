// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetWeaponMesh);

UENUM(BlueprintType)
enum class EWeaponType : uint8 {
	EWT_Initial UMETA(DisplayName = "Initial Type"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
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
	
	virtual void Shoot(const FVector& HitTarget);

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
};
