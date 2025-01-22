#include "ArsenalCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Components/CombatComponent.h"

AArsenalCharacter::AArsenalCharacter() {
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm Component"));
	SpringArmComp->SetupAttachment(CapsuleComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComp->SetupAttachment(SpringArmComp);

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	CombatComp->SetIsReplicated(true);
}

void AArsenalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AArsenalCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	if (CombatComp) {
		CombatComp->Character = this;
	}
}

void AArsenalCharacter::BeginPlay() {
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AArsenalCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AArsenalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipRifle);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Aim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArsenalCharacter::AimReleased);
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

void AArsenalCharacter::Aim() {
	if (CombatComp) {
		CombatComp->SetAiming(true);
	}
}

void AArsenalCharacter::AimReleased() {
	if (CombatComp) {
		CombatComp->SetAiming(false);
	}
}

void AArsenalCharacter::ServerEquipRifle_Implementation() {
	if (CombatComp) {
		CombatComp->EquipWeapon(EWeaponType::EWT_Rifle);
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
