#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArsenalCharacter.generated.h"

class UBoxComponent;
class UCombatComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class UAnimMontage;
class USpringArmComponent;
class UCameraComponent;
class UInputComponent;
class AWeapon;
enum class EWeaponType : uint8;

UCLASS()
class ENCHANTEDARSENAL_API AArsenalCharacter : public ACharacter {
	GENERATED_BODY()

public:
	////////////////////////////////////// Initalize Functions //////////////////////////////////////
	AArsenalCharacter();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	void EquipWeapon(EWeaponType WeaponType);
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(EWeaponType WeaponType);

	void Aim();
	void AimReleased();
	void Shoot();
	void ShootStarted();
	void ShootReleased();

	UFUNCTION()
	void HandleDeath();

	void AddRecoil(float Min, float Max);

	void SetAiming(bool bInAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bInAiming);

	AWeapon* GetWeapon();
	bool IsWeaponEquipped();
	bool IsAiming();
	bool IsShooting();

	////////////////////////////////////// Initalize Variables //////////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HeadshotBoxCollisionComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SkeletalMeshComp = FindComponentByClass<USkeletalMeshComponent>();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"), Replicated)
	UCombatComponent* CombatComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"), Replicated)
	UHealthComponent* HealthComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipRifleAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipSMGAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipShotgunAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipPistolAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ShootAction;

	UPROPERTY(EditAnywhere, Category = "Aim")
	float HipCameraBoomLength = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Aim")
	float AimCameraBoomLength = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Aim")
	float ADSTime = 0.5f;

	UPROPERTY(EditAnywhere, Category = Movement)
	float IdleWalkRunInterpSpeed;

	UPROPERTY(EditAnywhere, Category = Health)
	float MaxHealth = 250;

	float CurrentRecoilPitch = 0.f;
	float TargetRecoilPitch = 0.f;

	float CurrentRecoilYaw = 0.f;
	float TargetRecoilYaw = 0.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float RecoilInterpSpeed = 12.f;

	UPROPERTY(Replicated)
	bool bAiming;
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;
};
