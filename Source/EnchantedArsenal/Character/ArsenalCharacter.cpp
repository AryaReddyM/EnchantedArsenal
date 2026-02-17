#include "ArsenalCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnchantedArsenal/Components/MagicComponent.h"

AArsenalCharacter::AArsenalCharacter() {
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm Component"));
	SpringArmComp->SetupAttachment(CapsuleComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComp->SetupAttachment(SpringArmComp);

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	CombatComp->SetIsReplicated(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("Health Component"));
	HealthComp->SetIsReplicated(true);

	MagicComp = CreateDefaultSubobject<UMagicComponent>(TEXT("Magic Component"));
	MagicComp->SetIsReplicated(true);

	HeadshotBoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Headshot Box Collision Component"));
	HeadshotBoxCollisionComp->SetupAttachment(CapsuleComp);
}

void AArsenalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalCharacter, CombatComp);
	DOREPLIFETIME(AArsenalCharacter, HealthComp);
	DOREPLIFETIME(AArsenalCharacter, bAiming);
	DOREPLIFETIME(AArsenalCharacter, AttackType);
}

void AArsenalCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	if (CombatComp) {
		CombatComp->Character = this;
	}

	if (MagicComp) {
		MagicComp->Character = this;
	}
}

void AArsenalCharacter::BeginPlay() {
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	MoveComp->MaxWalkSpeed = BaseWalkSpeed;

	HealthComp->OnDeath.AddDynamic(this, &AArsenalCharacter::HandleDeath);
}

void AArsenalCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	float NewRecoilPitch = FMath::FInterpTo(CurrentRecoilPitch, TargetRecoilPitch, DeltaTime, RecoilInterpSpeed);
	float NewRecoilYaw = FMath::FInterpTo(CurrentRecoilYaw, TargetRecoilYaw, DeltaTime, RecoilInterpSpeed);

	float DeltaPitch = NewRecoilPitch - CurrentRecoilPitch;
	float DeltaYaw = NewRecoilYaw - CurrentRecoilYaw;

	AddControllerPitchInput(DeltaPitch);
	AddControllerYawInput(DeltaYaw);

	CurrentRecoilPitch = NewRecoilPitch;
	CurrentRecoilYaw = NewRecoilYaw;

	if (FMath::IsNearlyEqual(CurrentRecoilPitch, TargetRecoilPitch, 0.01f)) {
		CurrentRecoilPitch = TargetRecoilPitch = 0.f;
	}

	if (FMath::IsNearlyEqual(CurrentRecoilYaw, TargetRecoilYaw, 0.01f)) {
		CurrentRecoilYaw = TargetRecoilYaw = 0.f;
	}

	float TargetBoomLength = IsAiming() ? AimCameraBoomLength : HipCameraBoomLength;
	SpringArmComp->TargetArmLength = FMath::FInterpTo(SpringArmComp->TargetArmLength, TargetBoomLength, DeltaTime, ADSTime);
}

void AArsenalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		
		EnhancedInputComponent->BindAction(EquipRifleAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Rifle);
		EnhancedInputComponent->BindAction(EquipSMGAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_SMG);
		EnhancedInputComponent->BindAction(EquipShotgunAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Shotgun);
		EnhancedInputComponent->BindAction(EquipPistolAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Pistol);

		EnhancedInputComponent->BindAction(EquipBoulderAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipSpell, ESpellType::EST_Boulder);
		EnhancedInputComponent->BindAction(EquipSpikerAdderAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipSpell, ESpellType::EST_SpikeAdder);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Aim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArsenalCharacter::AimReleased);

		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Shoot);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AArsenalCharacter::ShootStarted);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AArsenalCharacter::ShootReleased);
	}
}

void AArsenalCharacter::Move(const FInputActionValue& Value) {
	FVector2D MoveAxisVector = Value.Get<FVector2D>();

	AddMovementInput(GetActorRightVector(), MoveAxisVector.X);
	AddMovementInput(GetActorForwardVector(), MoveAxisVector.Y);
}

void AArsenalCharacter::Look(const FInputActionValue& Value) {
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X * -0.4F);
	AddControllerPitchInput(LookAxisVector.Y * 0.4F);
}

void AArsenalCharacter::EquipWeapon(EWeaponType WeaponType) {
	if (!CombatComp) return;

	AttackType = EAttackType::EAT_Weapon;

	if (HasAuthority()) {
		if (CombatComp->SpawnedWeapon && CombatComp->SpawnedWeapon->WeaponType == WeaponType) return;
		CombatComp->bIsRecentlyEquipped = true;
		CombatComp->EquipWeapon(WeaponType);
	}
	else {
		ServerEquipWeapon(WeaponType);
	}
}

void AArsenalCharacter::ServerEquipWeapon_Implementation(EWeaponType WeaponType) {
	if (!CombatComp) return;

	if (CombatComp->SpawnedWeapon && CombatComp->SpawnedWeapon->WeaponType == WeaponType) return;

	AttackType = EAttackType::EAT_Weapon;

	CombatComp->bIsRecentlyEquipped = true;

	CombatComp->EquipWeapon(WeaponType);
}

void AArsenalCharacter::EquipSpell(ESpellType SpellType) {
	if (!MagicComp) return;

	AttackType = EAttackType::EAT_Magic;

	if (HasAuthority()) {
		if (MagicComp->SpawnedSpell && MagicComp->SpawnedSpell->SpellType == SpellType) return;
		MagicComp->EquipSpell(SpellType);
	}
	else {
		ServerEquipSpell(SpellType);
	}
}

void AArsenalCharacter::ServerEquipSpell_Implementation(ESpellType SpellType) {
	if (!MagicComp) return;

	if (MagicComp->SpawnedSpell && MagicComp->SpawnedSpell->SpellType == SpellType) return;

	AttackType = EAttackType::EAT_Magic;

	MagicComp->EquipSpell(SpellType);
}

void AArsenalCharacter::Aim() {
	if (CombatComp->SpawnedWeapon) {
		SetAiming(true);
	}
}

void AArsenalCharacter::AimReleased() {
	if (CombatComp->SpawnedWeapon) {
		SetAiming(false);
	}
}

void AArsenalCharacter::Shoot() {
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (CombatComp) {
			CombatComp->Shoot(true);
		}
		break;
	default:
		break;
	}
}

void AArsenalCharacter::ShootStarted() {
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (IsLocallyControlled() && CombatComp) {
			CombatComp->ResetSemiCounter();
		}
		break;
	default:
		break;
	}
}

void AArsenalCharacter::ShootReleased() {
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (CombatComp) {
			CombatComp->Shoot(false);
		}
		break;
	default:
		break;
	}
}

void AArsenalCharacter::HandleDeath() {
	Destroy();
	CombatComp->DestroyWeapon();
}

void AArsenalCharacter::AddRecoil(float Min, float Max) {
	TargetRecoilPitch += FMath::RandRange(Min, Max);
	TargetRecoilYaw += FMath::RandRange(Min, Max);
}

void AArsenalCharacter::SetAiming(bool bInAiming) {
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);

	MoveComp->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void AArsenalCharacter::ServerSetAiming_Implementation(bool bInAiming) {
	bAiming = bInAiming;

	MoveComp->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

AWeapon* AArsenalCharacter::GetWeapon() {
	if (CombatComp == nullptr) return nullptr;

	return CombatComp->SpawnedWeapon;
}

bool AArsenalCharacter::IsWeaponEquipped() {
	return (CombatComp && CombatComp->SpawnedWeapon);
}

bool AArsenalCharacter::IsAiming() {
	return bAiming;
}

bool AArsenalCharacter::IsShooting() {
	return (CombatComp && CombatComp->bShooting);
}

void AArsenalCharacter::OnRep_AttackType() {
}
