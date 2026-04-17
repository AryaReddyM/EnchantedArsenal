#include "ArsenalCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnchantedArsenal/Components/MagicComponent.h"
#include "Kismet/GameplayStatics.h"

////////////////////////////////////// Function Definitions //////////////////////////////////////

//////////////// Constructor ////////////////
AArsenalCharacter::AArsenalCharacter() {
	// Enables Ticking
	PrimaryActorTick.bCanEverTick = true;

	// Enables Replication
	bReplicates = true;
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);

	// Create Components
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm Component"));
	SpringArmComp->SetupAttachment(CapsuleComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComp->SetupAttachment(SpringArmComp);
	
	HipCameraPosComp = CreateDefaultSubobject<UArrowComponent>(TEXT("Hip Camera Position Component"));
	HipCameraPosComp->SetupAttachment(SpringArmComp);
	
	AimCameraPosComp = CreateDefaultSubobject<UArrowComponent>(TEXT("Aim Camera Position Component"));
	AimCameraPosComp->SetupAttachment(GetMesh(), "head");
	
	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	CombatComp->SetIsReplicated(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("Health Component"));
	HealthComp->SetIsReplicated(true);

	MagicComp = CreateDefaultSubobject<UMagicComponent>(TEXT("Magic Component"));
	MagicComp->SetIsReplicated(true);

	HeadshotBoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Headshot Box Collision Component"));
	HeadshotBoxCollisionComp->SetupAttachment(CapsuleComp);
}

//////////////// Replication ////////////////
void AArsenalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalCharacter, CombatComp);
	DOREPLIFETIME(AArsenalCharacter, HealthComp);
	DOREPLIFETIME(AArsenalCharacter, bAiming);
	DOREPLIFETIME(AArsenalCharacter, AttackType);
}

//////////////// Component Initialization ////////////////
void AArsenalCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	// Give Components a Reference to Character
	if (CombatComp) {
		CombatComp->Character = this;
	}
}

//////////////// BeginPlay ////////////////
void AArsenalCharacter::BeginPlay() {
	Super::BeginPlay();

	// Add the Enhanced Input mapping context for the local player
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Start In Default Move Speed
	MoveComp->MaxWalkSpeed = BaseWalkSpeed;

	// Adds HandleDeath Function to OnDeath Delegate in HealthComponent
	HealthComp->OnDeath.AddDynamic(this, &AArsenalCharacter::HandleDeath);
}

//////////////// Tick ////////////////
void AArsenalCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Recoil Smoothing
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

	// ADS Smoothing
	if (IsAiming()) {
		FVector CameraLocation = FMath::VInterpTo(CameraComp->GetComponentLocation(), AimCameraPosComp->GetComponentLocation(), DeltaTime, ADSSpeed);
		FRotator CameraRotation = FMath::RInterpTo(CameraComp->GetComponentRotation(), AimCameraPosComp->GetComponentRotation(), DeltaTime, ADSSpeed);
		CameraComp->SetWorldTransform(FTransform(CameraRotation.Quaternion(), CameraLocation));
	}
	else {
		FVector CameraLocation = FMath::VInterpTo(CameraComp->GetComponentLocation(), HipCameraPosComp->GetComponentLocation(), DeltaTime, ADSSpeed);
		FRotator CameraRotation = FMath::RInterpTo(CameraComp->GetComponentRotation(), HipCameraPosComp->GetComponentRotation(), DeltaTime, ADSSpeed);
		CameraComp->SetWorldTransform(FTransform(CameraRotation.Quaternion(), CameraLocation));
	}
}

//////////////// Input ////////////////
void AArsenalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Movement + Camera
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Look);

		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);

		// Weapon equips
		EnhancedInputComponent->BindAction(EquipRifleAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Rifle);
		EnhancedInputComponent->BindAction(EquipSMGAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_SMG);
		EnhancedInputComponent->BindAction(EquipShotgunAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Shotgun);
		EnhancedInputComponent->BindAction(EquipPistolAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Pistol);

		// Spell equips
		EnhancedInputComponent->BindAction(EquipBoulderAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipSpell, ESpellType::EST_Boulder);
		EnhancedInputComponent->BindAction(EquipSpikerAdderAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipSpell, ESpellType::EST_SpikeAdder);

		// Aim
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Aim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArsenalCharacter::AimReleased);

		// Shoot
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Shoot);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AArsenalCharacter::ShootStarted);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AArsenalCharacter::ShootReleased);
	}
}

////////////////////////////////////// Input Functions //////////////////////////////////////

//////////////// Move ////////////////
void AArsenalCharacter::Move(const FInputActionValue& Value) {
	FVector2D MoveAxisVector = Value.Get<FVector2D>();

	AddMovementInput(GetActorRightVector(), MoveAxisVector.X);
	AddMovementInput(GetActorForwardVector(), MoveAxisVector.Y);
}

//////////////// Look ////////////////
void AArsenalCharacter::Look(const FInputActionValue& Value) {
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X * -0.4F);
	AddControllerPitchInput(LookAxisVector.Y * 0.4F);
}

////////////////////////////////////// Combat / Magic Functions //////////////////////////////////////

//////////////// EquipWeapon ////////////////
void AArsenalCharacter::EquipWeapon(EWeaponType WeaponType) {
	if (!CombatComp) return;

	if (HasAuthority()) {
		// Server: Set Attack Type and Equip Immediately
		ServerSetAttackType(EAttackType::EAT_Weapon);
		CombatComp->bIsRecentlyEquipped = true;
		CombatComp->EquipWeapon(WeaponType);
	}
	else {
		// Client: Ask The Server to Equip
		ServerEquipWeapon(WeaponType);
	}
}

//////////////// ServerEquipWeapon ////////////////
void AArsenalCharacter::ServerEquipWeapon_Implementation(EWeaponType WeaponType) {
	if (!CombatComp) return;

	ServerSetAttackType(EAttackType::EAT_Weapon);

	// Already Equipped this Weapon -> Return
	if (CombatComp->SpawnedWeapon && CombatComp->SpawnedWeapon->WeaponType == WeaponType) return;

	CombatComp->bIsRecentlyEquipped = true;
	CombatComp->EquipWeapon(WeaponType);
}

//////////////// EquipSpell ////////////////
void AArsenalCharacter::EquipSpell(ESpellType SpellType) {
	if (!MagicComp || MagicComp->IsSpellOnCooldown(SpellType)) return;

	if (HasAuthority()) {
		ServerSetAttackType(EAttackType::EAT_Magic);
		MagicComp->EquipSpell(SpellType);
	}
	else {
		ServerEquipSpell(SpellType);
	}
}

//////////////// ServerEquipSpell (RPC) ////////////////
void AArsenalCharacter::ServerEquipSpell_Implementation(ESpellType SpellType) {
	if (!MagicComp) return;

	ServerSetAttackType(EAttackType::EAT_Magic);
	MagicComp->EquipSpell(SpellType);
}

//////////////// Aim / AimReleased ////////////////
void AArsenalCharacter::Aim() {
	switch (AttackType) {
	case EAttackType::EAT_Unarmed:
		// Unarmed: No Aiming
		break;
	default:
		// Everything Else: Allow Aiming
		SetAiming(true);
	}
}

void AArsenalCharacter::AimReleased() {
	SetAiming(false);
}

//////////////// Shoot / ShootStarted / ShootReleased ////////////////
void AArsenalCharacter::Shoot() {
	// Shoots for Corresponding Attack Type
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (CombatComp) {
			CombatComp->Shoot(true);
		}
		break;
	case EAttackType::EAT_Magic:
		if (MagicComp) {
			MagicComp->Cast(true);
		}
		break;
	default:
		break;
	}
}

void AArsenalCharacter::ShootStarted() {
	// Only Resets Semi Counter (Used for Semi-Auto) if AttackType is a Weapon
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
	// Stops Shooting for Corresponding Attack Type
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (CombatComp) {
			CombatComp->Shoot(false);
		}
		break;
	case EAttackType::EAT_Magic:
		if (MagicComp) {
			MagicComp->Cast(false);
		}
		break;
	default:
		break;
	}
}

////////////////////////////////////// Utility Functions //////////////////////////////////////

//////////////// HandleDeath ////////////////
void AArsenalCharacter::HandleDeath() {
	// Destroys Character
	Destroy();

	// Destroys Character Equipment
	if (CombatComp) CombatComp->UnequipWeapon();
	if (MagicComp)  MagicComp->UnequipSpell();
}

//////////////// AddRecoil ////////////////
void AArsenalCharacter::AddRecoil(float Min, float Max) {
	TargetRecoilPitch += FMath::RandRange(Min, Max);
	TargetRecoilYaw += FMath::RandRange(Min, Max);
}

//////////////// SetAiming / ServerSetAiming ////////////////
void AArsenalCharacter::SetAiming(bool bInAiming) {
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);

	MoveComp->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void AArsenalCharacter::ServerSetAiming_Implementation(bool bInAiming) {
	bAiming = bInAiming;

	MoveComp->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

//////////////// ServerSetAttackType ////////////////
void AArsenalCharacter::ServerSetAttackType_Implementation(EAttackType NewType) {
	if (AttackType == NewType) return;

	AttackType = NewType;

	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (MagicComp) MagicComp->UnequipSpell();
		break;
	case EAttackType::EAT_Magic:
		if (CombatComp) CombatComp->UnequipWeapon();
		break;
	default:
		if (CombatComp) CombatComp->UnequipWeapon();
		if (MagicComp)  MagicComp->UnequipSpell();
		break;
	}
}

FHitResult AArsenalCharacter::TraceUnderCrosshairs() {
	FHitResult TraceHitResult;

	if (!IsLocallyControlled()) return FHitResult();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return FHitResult();

	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return FHitResult();

	const FVector2D CrosshairLocation(SizeX * 0.5f, SizeY * 0.5f);

	FVector CrosshairWorldPos;
	FVector CrosshairWorldDir;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC, CrosshairLocation, CrosshairWorldPos, CrosshairWorldDir))
		return FHitResult();

	const FVector Start = CrosshairWorldPos;
	const FVector End = Start + CrosshairWorldDir * TRACE_LENGTH;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
    
	if (CombatComp && CombatComp->SpawnedWeapon) Params.AddIgnoredActor(CombatComp->SpawnedWeapon);
    
	if (MagicComp && MagicComp->HeldSpell) {
		Params.AddIgnoredActor(MagicComp->HeldSpell);
	}

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility, Params);
    
	if (!TraceHitResult.bBlockingHit) {
		TraceHitResult.ImpactPoint = End;
	}

	return TraceHitResult;
}

////////////////////////////////////// Getters //////////////////////////////////////

//////////////// GetWeapon ////////////////
AWeapon* AArsenalCharacter::GetWeapon() {
	if (CombatComp == nullptr) return nullptr;

	return CombatComp->SpawnedWeapon;
}

//////////////// IsWeaponEquipped ////////////////
bool AArsenalCharacter::IsWeaponEquipped() {
	return (CombatComp && CombatComp->SpawnedWeapon);
}

//////////////// IsAiming ////////////////
bool AArsenalCharacter::IsAiming() {
	return bAiming;
}

//////////////// IsShooting ////////////////
bool AArsenalCharacter::IsShooting() {
	return (CombatComp && CombatComp->bShooting);
}

////////////////////////////////////// Replication Notifies //////////////////////////////////////

//////////////// OnRep_AttackType ////////////////
void AArsenalCharacter::OnRep_AttackType() {
}