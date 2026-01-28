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
	DOREPLIFETIME(UCombatComponent, bShooting);
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
	if (!Character || !Character->HasAuthority()) return;

	if (SpawnedWeapon) {
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}

	TSubclassOf<AWeapon> WeaponClass = nullptr;
	switch (WeaponType) {
	case EWeaponType::EWT_Rifle:   WeaponClass = Rifle;   break;
	case EWeaponType::EWT_Shotgun: WeaponClass = Shotgun; break;
	case EWeaponType::EWT_SMG:     WeaponClass = SMG;     break;
	case EWeaponType::EWT_Pistol:  WeaponClass = Pistol;  break;
	default: return;
	}
	if (!WeaponClass) return;

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FTransform::Identity, Params);
	if (!NewWeapon) return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh || !NewWeapon->GripPoint) {
		NewWeapon->Destroy();
		return;
	}

	const FName HandSocket(TEXT("RightHandSocket"));
	NewWeapon->AttachToComponent(CharMesh, FAttachmentTransformRules::KeepRelativeTransform, HandSocket);

	const FTransform GripRelativeTransform = NewWeapon->GripPoint->GetRelativeTransform();
	NewWeapon->SetActorRelativeTransform(GripRelativeTransform.Inverse());

	SpawnedWeapon = NewWeapon;

	Character->PlayEquipMontage();
}

void UCombatComponent::DestroyWeapon() {
	if (SpawnedWeapon) SpawnedWeapon->Destroy();
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

	bShooting = bTriggered;
	ServerShoot(bTriggered);

	if (Character && SpawnedWeapon->FireType != EFireType::EFT_SemiAuto) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bShooting ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerShoot_Implementation(bool bTriggered) {
	MultiShoot(bTriggered);

	bShooting = bTriggered;
	if (Character && SpawnedWeapon->FireType != EFireType::EFT_SemiAuto) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bShooting ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::MultiShoot_Implementation(bool bTriggered) {
	if (!SpawnedWeapon || !Character) return;

	if (Character->GetMesh()->GetAnimInstance()->Montage_IsPlaying(SpawnedWeapon->EquipMontage) || !bTriggered) return;

	if (bIsRecentlyEquipped) {
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		float LastEquipTime = -1000.0f;

		if (CurrentTime - LastEquipTime < 20.0f) return;

		LastEquipTime = CurrentTime;
	}

	if (SpawnedWeapon->FireType == EFireType::EFT_Auto) {
		Character->PlayShootMontage();
	}

	SpawnedWeapon->Shoot();
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult) {
	TraceHitResult = FHitResult();

	if (!Character) return;

	if (!Character->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return;

	const FVector2D CrosshairLocation(SizeX * 0.5f, SizeY * 0.5f);

	FVector CrosshairWorldPos;
	FVector CrosshairWorldDir;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC, CrosshairLocation, CrosshairWorldPos, CrosshairWorldDir))
		return;

	const FVector Start = CrosshairWorldPos;
	const FVector End = Start + CrosshairWorldDir * TRACE_LENGTH;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	if (SpawnedWeapon) Params.AddIgnoredActor(SpawnedWeapon);

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility, Params);

	if (!TraceHitResult.bBlockingHit) {
		TraceHitResult.ImpactPoint = End;
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

void UCombatComponent::OnRep_SpawnedWeapon() {
	if (!Character || !SpawnedWeapon || !SpawnedWeapon->GripPoint) return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) return;

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedWeapon->AttachToComponent(CharMesh, FAttachmentTransformRules::KeepRelativeTransform, HandSocket);

	const FTransform GripRelativeTransform = SpawnedWeapon->GripPoint->GetRelativeTransform();
	SpawnedWeapon->SetActorRelativeTransform(GripRelativeTransform.Inverse());

	Character->PlayEquipMontage();
}

void UCombatComponent::OnRep_SemiShotCounter() {
}
