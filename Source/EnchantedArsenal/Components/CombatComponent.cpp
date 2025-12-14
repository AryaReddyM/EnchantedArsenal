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
	if (Character == nullptr) return;

	AWeapon* TempWeapon = NewObject<AWeapon>(this, AWeapon::StaticClass());
	TempWeapon->SetWeaponType(WeaponType);
		
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		FActorSpawnParameters SpawnInfo;
		FTransform HandSocketTransform = HandSocket->GetSocketTransform(Character->GetMesh());

		switch (WeaponType) {
		case EWeaponType::EWT_Rifle:
			if (SpawnedWeapon) SpawnedWeapon->Destroy();

			SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(Rifle, HandSocketTransform, SpawnInfo);
			break;
		case EWeaponType::EWT_Shotgun:
			if (SpawnedWeapon) SpawnedWeapon->Destroy();

			SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(Shotgun, HandSocketTransform, SpawnInfo);
			break;
		case EWeaponType::EWT_SMG:
			if (SpawnedWeapon) SpawnedWeapon->Destroy();

			SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(SMG, HandSocketTransform, SpawnInfo);
			break;
		}

		HandSocket->AttachActor(SpawnedWeapon, Character->GetMesh  ());
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
		ServerShoot();
	}
}

void UCombatComponent::ServerShoot_Implementation() {
	MultiShoot();
}

void UCombatComponent::MultiShoot_Implementation() {
	if (SpawnedWeapon == nullptr) return;

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

	SpawnedWeapon->SemiShotCounter = Counter;
}