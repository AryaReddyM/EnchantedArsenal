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
#include "ArsenalAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Components/BoxComponent.h"

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

	HeadshotBoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Headshot Box Collision Component"));
	HeadshotBoxCollisionComp->SetupAttachment(CapsuleComp);
}

void AArsenalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalCharacter, CombatComp);
	DOREPLIFETIME(AArsenalCharacter, HealthComp);
}

void AArsenalCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	if (CombatComp) {
		CombatComp->Character = this;
	}

	if (HealthComp) {
		HealthComp->MaxHealth = MaxHealth;
		HealthComp->CurrentHealth = MaxHealth;
	}

	if (CombatComp) {
		if (HasAuthority()) {
			CombatComp->EquipWeapon(EWeaponType::EWT_Rifle);
		}
		else {
			ServerEquipRifle();
		}
	}
}

void AArsenalCharacter::BeginPlay() {
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

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
		
		EnhancedInputComponent->BindAction(EquipRifleAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipRifle);
		EnhancedInputComponent->BindAction(EquipSMGAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipSMG);
		EnhancedInputComponent->BindAction(EquipShotgunAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipShotgun);
		EnhancedInputComponent->BindAction(EquipPistolAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipPistol);

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

void AArsenalCharacter::EquipRifle() {
	if (CombatComp) {
		if (HasAuthority()) {
			CombatComp->EquipWeapon(EWeaponType::EWT_Rifle);
		}
		else {
			ServerEquipRifle();
		}
	}
}
void AArsenalCharacter::ServerEquipRifle_Implementation() {
	if (CombatComp) {
		CombatComp->EquipWeapon(EWeaponType::EWT_Rifle);
	}
}

void AArsenalCharacter::EquipSMG() {
	if (CombatComp) {
		if (HasAuthority()) {
			CombatComp->EquipWeapon(EWeaponType::EWT_SMG);
		}
		else {
			ServerEquipSMG();
		}
	}
}
void AArsenalCharacter::ServerEquipSMG_Implementation() {
	if (CombatComp) {
		CombatComp->EquipWeapon(EWeaponType::EWT_SMG);
	}
}

void AArsenalCharacter::EquipShotgun() {
	if (CombatComp) {
		if (HasAuthority()) {
			CombatComp->EquipWeapon(EWeaponType::EWT_Shotgun);
		}
		else {
			ServerEquipShotgun();
		}
	}
}
void AArsenalCharacter::ServerEquipShotgun_Implementation() {
	if (CombatComp) {
		CombatComp->EquipWeapon(EWeaponType::EWT_Shotgun);
	}
}


void AArsenalCharacter::EquipPistol() {
	if (CombatComp) {
		if (HasAuthority()) {
			CombatComp->EquipWeapon(EWeaponType::EWT_Pistol);
		}
		else {
			ServerEquipPistol();
		}
	}
}
void AArsenalCharacter::ServerEquipPistol_Implementation() {
	if (CombatComp) {
		CombatComp->EquipWeapon(EWeaponType::EWT_Pistol);
	}
}

void AArsenalCharacter::Aim() {
	if (CombatComp && CombatComp->SpawnedWeapon) {
		CombatComp->SetAiming(true);
	}
}

void AArsenalCharacter::AimReleased() {
	if (CombatComp && CombatComp->SpawnedWeapon) {
		CombatComp->SetAiming(false);
	}
}

void AArsenalCharacter::Shoot() {
	if (CombatComp && CombatComp->CanShoot()) {
		CombatComp->Shoot(true);
	}
}

void AArsenalCharacter::ShootStarted() {
	if (IsLocallyControlled() && CombatComp) {
		CombatComp->ResetSemiCounter();
	}
}

void AArsenalCharacter::ShootReleased() {
	if (CombatComp) {
		CombatComp->Shoot(false);
	}
}

bool AArsenalCharacter::IsWeaponEquipped() {
	return (CombatComp && CombatComp->SpawnedWeapon);
}

bool AArsenalCharacter::IsAiming() {

	return (CombatComp && CombatComp->bAiming);
}

AWeapon* AArsenalCharacter::GetWeapon() {
	if (CombatComp == nullptr) return nullptr;

	return CombatComp->SpawnedWeapon;
}

void AArsenalCharacter::PlayShootMontage(EWeaponType WeaponType) {
	if (!CombatComp || !CombatComp->SpawnedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	UAnimMontage* CurrentMontage = nullptr;

	switch (WeaponType) {
	case EWeaponType::EWT_Rifle:
		CurrentMontage = ShootAutoMontage;
		break;
	case EWeaponType::EWT_SMG:     
		CurrentMontage = ShootAutoMontage;
		break;
	case EWeaponType::EWT_Shotgun: 
		CurrentMontage = ShootShotgunMontage;
		break;
	case EWeaponType::EWT_Pistol:  
		CurrentMontage = ShootPistolMontage;
		break;
	default:
		break;
	}

	if (!CurrentMontage) {
		return;
	}

	if (CombatComp->SpawnedWeapon->WeaponType == EWeaponType::EWT_Shotgun || CombatComp->SpawnedWeapon->WeaponType == EWeaponType::EWT_Pistol) {
		AnimInstance->Montage_Play(CurrentMontage);
		return;
	}

	if (!AnimInstance->Montage_IsPlaying(CurrentMontage)) {
		AnimInstance->Montage_Play(CurrentMontage);
	}
}


//void AArsenalCharacter::PlayEquipMontage(EWeaponType WeaponType) {
//	if (CombatComp == nullptr || CombatComp->SpawnedWeapon == nullptr) return;
//
//	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
//
//	const bool bHasAim = ShootWeaponMontage && ShootWeaponMontage->GetSectionIndex(FName("RifleAim")) != INDEX_NONE;
//	const bool bHasHip = ShootWeaponMontage && ShootWeaponMontage->GetSectionIndex(FName("RifleHip")) != INDEX_NONE;
//
//	if (AnimInstance && ShootWeaponMontage) {
//
//		FName SectionName;
//
//		if (bAiming) {
//			SectionName = FName("RifleAim");
//		}
//		else {
//			SectionName = FName("RifleHip");
//		}
//
//		if (!AnimInstance->Montage_IsPlaying(ShootWeaponMontage)) {
//			AnimInstance->Montage_Play(ShootWeaponMontage);
//			AnimInstance->Montage_JumpToSection(SectionName);
//		}
//	}
//}

void AArsenalCharacter::HandleDeath() {
	Destroy();
	CombatComp->DestroyWeapon();
}

void AArsenalCharacter::AddRecoil(float Min, float Max) {
	TargetRecoilPitch += FMath::RandRange(Min, Max);
	TargetRecoilYaw += FMath::RandRange(Min, Max);
}

