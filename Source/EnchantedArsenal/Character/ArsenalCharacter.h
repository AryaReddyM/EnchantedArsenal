#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArsenalCharacter.generated.h"

class UPhysicsData;
class UPhysicalAnimationComponent;
struct FPhysicalAnimationData;
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
class AArsenalPlayerState;
class UImage;
enum class ETeam : uint8;

UENUM(BlueprintType)
enum class EAttackType : uint8 {
	EAT_Initial UMETA(DisplayName = "Initial Type"),
	EAT_Unarmed UMETA(DisplayName = "Unarmed"),
	EAT_Weapon UMETA(DisplayName = "Weapon"),
	EAT_Magic UMETA(DisplayName = "Magic"),

	EAT_MAX UMETA(DisplayName = "DefaultMax")
};

////////////////////////////////////// Forward Declarations //////////////////////////////////////

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
	
	// When Game Starts
	virtual void BeginPlay() override;

	// Every Game Tick
	virtual void Tick(float DeltaTime) override;
	
	virtual void PossessedBy(AController* NewController) override;

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
	
	void Reload();
	UFUNCTION(Server, Reliable)
	void ServerReload();

	void Aim();
	void AimReleased();
	
	void Shoot();
	void StopShoot();

	// Utility Functions
	UFUNCTION()
	void HandleDeath(AActor* Damager);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdoll();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEndRagdoll();
	
	void ResetPlayer(APlayerController* PC, APawn* Spectator);

	void AddRecoil(float Min, float Max);

	void SetAiming(bool bInAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bInAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAttackType(EAttackType NewType);
	
	FHitResult TraceUnderCrosshairs();
	
	void SetTeamColor(ETeam Team);
	void SetSpawnPoint();

	void OnPlayerStateInit();

	UFUNCTION()
	void HandleTeamScoreChanged(ETeam Team, float NewScore);

	UFUNCTION(Client, Unreliable)
	void ClientShowDamageIndicator(AArsenalCharacter* Victim, float Damage);

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* HipCameraPosComp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* AimCameraPosComp;

	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	// Headshot Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HeadshotBoxCollisionComp;

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	
	// Physics Animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UPhysicalAnimationComponent* PhysComp;
	
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
	
	// Reload
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

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
	
	// HUD
	UPROPERTY(EditAnywhere)
	UUserWidget* HUD;
	
	// Damage Indicator
	UPROPERTY(EditAnywhere)
	UUserWidget* DamageIndicator;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageAccumulateWindow = 1.0f;

	// Damage Tally for Indicator
	struct FDamageTally {
		float Total = 0.f; 
		float LastTime = 0.f;
	};
	TMap<TWeakObjectPtr<AArsenalCharacter>, FDamageTally> DamageTallies;
	
	TArray<TWeakObjectPtr<class UWidgetComponent>> ActiveDamageWidgets;

	// POV when Default
	UPROPERTY(EditAnywhere, Category = "Aim")
	float HipCameraBoomLength = 300.0f;

	// Lerp time in between ADS and Default
	UPROPERTY(EditAnywhere, Category = "Aim")
	float ADSSpeed = 10.0f;

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
	bool bIsAiming;
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.0f;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.0f;

	// Attack Type
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, ReplicatedUsing = OnRep_AttackType)
	EAttackType AttackType = EAttackType::EAT_Unarmed;
	UFUNCTION()
	void OnRep_AttackType();
	
	// Crosshair Variables
	float CurrentVisualSpread;
	float TargetVisualSpread;
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float InterpSpeed = 15.f;
	
	// Teams
	UPROPERTY(EditAnywhere, Category = "Teams")
	TArray<UMaterialInterface*> BlueMaterials;

	UPROPERTY(EditAnywhere, Category = "Teams")
	TArray<UMaterialInterface*> RedMaterials;

	UPROPERTY(EditAnywhere, Category = "Teams")
	TArray<UMaterialInterface*> NoMaterials;

	int BluePlayers = 0;
	int RedPlayers = 0;
	
	UPROPERTY()
	AArsenalPlayerState* ArsenalPlayerState;

	virtual void OnRep_PlayerState() override;
	
	// Equip
	bool bIsEquipping = false;
	FTimerHandle EquipTimerHandle;
	
	// Reload
	UPROPERTY(ReplicatedUsing=OnRep_bIsReloading)
	bool bIsReloading = false;
	UFUNCTION()
	void OnRep_bIsReloading();
	FTimerHandle ReloadTimerHandle;
	
	// Death
	FTimerHandle DeathTimer;
	UPROPERTY(EditAnywhere, Category = "Death")
	float DeathDelay = 10.0f;
	
	// Physics Animation
	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	UPhysicsData* PhysicsData;
};