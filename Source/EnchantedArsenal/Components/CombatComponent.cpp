#include "CombatComponent.h"

#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnchantedArsenal/PlayerController/ArsenalPlayerController.h"
#include "EnchantedArsenal/HUD/ArsenalHUD.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay() {
	Super::BeginPlay();

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);
}

void UCombatComponent::EquipWeapon(EWeaponType WeaponType) {
	if (Character == nullptr) return;

	AWeapon* TempWeapon = NewObject<AWeapon>(this, AWeapon::StaticClass());
	TempWeapon->SetWeaponType(WeaponType);
		
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		FActorSpawnParameters SpawnInfo;
		FTransform HandSocketTransform = HandSocket->GetSocketTransform(Character->GetMesh());

		SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(Weapon, HandSocketTransform, SpawnInfo);

		HandSocket->AttachActor(SpawnedWeapon, Character->GetMesh());
	}
		
	SpawnedWeapon->SetOwner(Character);
}


void UCombatComponent::SetAiming(bool bInAiming) {
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming) {
	bAiming = bInAiming;

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::Shoot(bool bTriggered) {
	bShootButtonPressed = bTriggered;
	
	if (bShootButtonPressed) {
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		ServerShoot(HitResult.ImpactPoint);
	}
}

void UCombatComponent::ServerShoot_Implementation(const FVector_NetQuantize& TraceHitTarget) {
	MultiShoot(TraceHitTarget);
}

void UCombatComponent::MultiShoot_Implementation(const FVector_NetQuantize& TraceHitTarget) {
	if (SpawnedWeapon == nullptr) return;

	if (Character) {
		Character->PlayShootMontage(bAiming);
		SpawnedWeapon->StartShoot(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult) {
	FVector2D ViewportSize;

	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

	FVector CrosshairWorldPos;
	FVector CrosshairWorldDir;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPos, CrosshairWorldDir);

	if (bScreenToWorld) {
		FVector Start = CrosshairWorldPos;
		FVector End = Start + CrosshairWorldDir * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime) {
	if (Character == nullptr ) return;

	PlayerController = PlayerController == nullptr ? Cast<AArsenalPlayerController>(Character->Controller) : PlayerController;

	if (PlayerController) {

		HUD = HUD == nullptr ? Cast<AArsenalHUD>(PlayerController->GetHUD()) : HUD;

		if (HUD) {
			FHUDPackage HUDPackage;

			if (SpawnedWeapon) {
				HUDPackage.CrosshairsCenter = SpawnedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsRight = SpawnedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsLeft = SpawnedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsUp = SpawnedWeapon->CrosshairsUp;
				HUDPackage.CrosshairsDown = SpawnedWeapon->CrosshairsDown;
			}
			else {
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsUp = nullptr;
				HUDPackage.CrosshairsDown = nullptr;
			}

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}
