#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArsenalCharacter.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8 {
	EAT_Initial UMETA(DisplayName = "Initial Type"),
	EAT_Unarmed UMETA(DisplayName = "Unarmed"),
	EAT_Weapon UMETA(DisplayName = "Weapon"),
	EAT_Magic UMETA(DisplayName = "Magic"),

	EAT_MAX UMETA(DisplayName = "DefaultMax")
};

////////////////////////////////////// Forward Declarations //////////////////////////////////////

class UBoxComponent;
class UCombatComponent;
class UHealthComponent;
class UMagicComponent;
class UInputAction;
class UInputMappingContext;
class UAnimMontage;
class USpringArmComponent;
class UCameraComponent;
class UInputComponent;
class AWeapon;
class ASpell;
class USpellData;
enum class EWeaponType : uint8;
enum class ESpellType : uint8;

UCLASS()
class ENCHANTEDARSENAL_API AArsenalCharacter : public ACharacter {
	GENERATED_BODY()

public:
	////////////////////////////////////// Function Declarations //////////////////////////////////////

	// Constructor
	AArsenalCharacter();
	
	// Replication Function
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Components Are Initialized and Ready to Use
	virtual void PostInitializeComponents() override;
	
protected:
	// When Game Starts
	virtual void BeginPlay() override;

public:	
	// Every Game Tick
	virtual void Tick(float DeltaTime) override;

	// Player Input Function
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Input Functions
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	void EquipWeapon(EWeaponType WeaponType);
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(EWeaponType WeaponType);

	void EquipSpell(ESpellType SpellType);
	UFUNCTION(Server, Reliable)
	void ServerEquipSpell(ESpellType SpellType);

	void Aim();
	void AimReleased();
	void Shoot();
	void ShootStarted();
	void ShootReleased();

	// Utility Functions
	UFUNCTION()
	void HandleDeath();

	void AddRecoil(float Min, float Max);

	void SetAiming(bool bInAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bInAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAttackType(EAttackType NewType);

	// Getters
	AWeapon* GetWeapon();
	bool IsWeaponEquipped();
	bool IsAiming();
	bool IsShooting();

	////////////////////////////////////// Variable Declarations //////////////////////////////////////

	//////////////// Components ////////////////

	// Camera Boom
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	// Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	// Headshot Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HeadshotBoxCollisionComp;

	// Mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SkeletalMeshComp = FindComponentByClass<USkeletalMeshComponent>();

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

	// Weapon Combat
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"), Replicated)
	UCombatComponent* CombatComp;
	
	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"), Replicated)
	UHealthComponent* HealthComp;

	// Magic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"), Replicated)
	UMagicComponent* MagicComp;

	//////////////// Input ////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	
	// Move (W, A, S, D)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	// Look Around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	// Jump
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	////// Weapons //////
	
	// Equip Rifle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipRifleAction;

	// Equip SMG
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipSMGAction;

	// Equip Shotgun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipShotgunAction;

	// Equip Pistol
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipPistolAction;

	////// Spells //////

	// Equip Boulder
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipBoulderAction;

	// Equip Spiker Adder
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipSpikerAdderAction;

	// Aim
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	// Shoot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ShootAction;

	//////////////// Utilities ////////////////

	// POV when Default
	UPROPERTY(EditAnywhere, Category = "Aim")
	float HipCameraBoomLength = 300.0f;

	// POV when ADS
	UPROPERTY(EditAnywhere, Category = "Aim")
	float AimCameraBoomLength = 150.0f;

	// Lerp time in between ADS and Default
	UPROPERTY(EditAnywhere, Category = "Aim")
	float ADSTime = 0.5f;

	// Helps Calculate Speed for Animation
	UPROPERTY(EditAnywhere, Category = Movement)
	float IdleWalkRunInterpSpeed;

	// Recoil Variables
	float CurrentRecoilPitch = 0.f;
	float TargetRecoilPitch = 0.f;

	float CurrentRecoilYaw = 0.f;
	float TargetRecoilYaw = 0.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float RecoilInterpSpeed = 12.f;

	// Aim Variables
	UPROPERTY(Replicated)
	bool bAiming;
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;

	// Attack Type
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, ReplicatedUsing = OnRep_AttackType)
	EAttackType AttackType = EAttackType::EAT_Unarmed;
	UFUNCTION()
	void OnRep_AttackType();
};
