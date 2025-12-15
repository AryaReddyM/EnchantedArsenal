#include "CombatComponent.h"

#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnchantedArsenal/PlayerController/ArsenalPlayerController.h"
#include "EnchantedArsenal/HUD/ArsenalHUD.h"
#include "EnchantedArsenal/Weapon/Weapon.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, SemiShotCounter);
}

void UCombatComponent::BeginPlay() {
	Super::BeginPlay();

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(EWeaponType WeaponType) {
	if (!Character || !GetOwner()->HasAuthority()) return;

	if (SpawnedWeapon) {
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;

		FTransform SpawnTransform = HandSocket->GetSocketTransform(Character->GetMesh());
		AWeapon* NewWeapon = nullptr;

		switch (WeaponType) {
		case EWeaponType::EWT_Rifle:
			NewWeapon = GetWorld()->SpawnActor<AWeapon>(Rifle, SpawnTransform, SpawnParams);
			break;
		case EWeaponType::EWT_Shotgun:
			NewWeapon = GetWorld()->SpawnActor<AWeapon>(Shotgun, SpawnTransform, SpawnParams);
			break;
		case EWeaponType::EWT_SMG:
			NewWeapon = GetWorld()->SpawnActor<AWeapon>(SMG, SpawnTransform, SpawnParams);
			break;
		case EWeaponType::EWT_Pistol:
			NewWeapon = GetWorld()->SpawnActor<AWeapon>(Pistol, SpawnTransform, SpawnParams);
			break;
		default:
			break;
		}

		if (NewWeapon) {
			SpawnedWeapon = NewWeapon;
			HandSocket->AttachActor(SpawnedWeapon, Character->GetMesh());
			SpawnedWeapon->SetOwner(Character);
		}
	}
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
	if (!SpawnedWeapon) return;

	bShootButtonPressed = bTriggered;

	if (bShootButtonPressed) {
		ServerShoot();
	}
}

void UCombatComponent::ServerShoot_Implementation() {
	MultiShoot();
}

void UCombatComponent::MultiShoot_Implementation() {
	if (!SpawnedWeapon) return;

	if (Character) {
		if (SpawnedWeapon->FireType == EFireType::EWT_Auto) {
			Character->PlayShootMontage(bAiming);
		}
		SpawnedWeapon->Shoot();
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

void UCombatComponent::SetSemiCounter(int Counter) {
	if (!SpawnedWeapon) return;

	if (GetOwnerRole() < ROLE_Authority) {
		ServerSetSemiCounter(Counter);
	}
	else {
		SemiShotCounter = Counter;
		OnRep_SemiShotCounter();
	}
}

void UCombatComponent::ServerSetSemiCounter_Implementation(int32 NewCounter) {
	SemiShotCounter = NewCounter;
	OnRep_SemiShotCounter();
}

void UCombatComponent::ResetSemiCounter() {
	if (GetOwnerRole() < ROLE_Authority) {
		ServerResetSemiCounter();
	}
	else {
		SemiShotCounter = 0;
		OnRep_SemiShotCounter();
	}
}

void UCombatComponent::ServerResetSemiCounter_Implementation() {
	SemiShotCounter = 0;
	OnRep_SemiShotCounter();
}

bool UCombatComponent::ServerResetSemiCounter_Validate() {
	return true;
}

bool UCombatComponent::CanShoot() {
	if (!SpawnedWeapon) return false;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	return (CurrentTime - LastEquipTime) >= SpawnedWeapon->EquipDelay;
}

void UCombatComponent::OnRep_SpawnedWeapon() {
	if (Character && SpawnedWeapon) {
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket) {
			HandSocket->AttachActor(SpawnedWeapon, Character->GetMesh());
		}
		SpawnedWeapon->SetOwner(Character);
	}
}

void UCombatComponent::OnRep_SemiShotCounter() {
}
