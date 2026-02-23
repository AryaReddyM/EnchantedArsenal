#include "CombatComponent.h"

#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
	DOREPLIFETIME(UCombatComponent, bShooting);
	DOREPLIFETIME(UCombatComponent, SemiShotCounter);
}

void UCombatComponent::BeginPlay() {
	Super::BeginPlay();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(EWeaponType WeaponType) {
	if (!Character || !Character->HasAuthority()) return;

	UnequipWeapon();

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

	PlayEquipMontage();
}

void UCombatComponent::UnequipWeapon() {
	if (!Character || !Character->HasAuthority()) return;

	if (SpawnedWeapon) {
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}

	bShooting = false;
	SemiShotCounter = 0;
}

void UCombatComponent::Shoot(bool bTriggered) {
	if (!SpawnedWeapon) return;

	bShooting = bTriggered;
	ServerShoot(bTriggered);
}

void UCombatComponent::ServerShoot_Implementation(bool bTriggered) {
	MultiShoot(bTriggered);

	bShooting = bTriggered;
}

void UCombatComponent::MultiShoot_Implementation(bool bTriggered) {
	if (!SpawnedWeapon || !Character) return;

	if (Character->GetMesh()->GetAnimInstance()->Montage_IsPlaying(SpawnedWeapon->EquipMontage) || !bTriggered) return;

	if (bIsRecentlyEquipped) {
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const float EquipDelay = SpawnedWeapon ? SpawnedWeapon->EquipDelay : 0.f;

		if (CurrentTime - LastEquipTime < EquipDelay) return;

		bIsRecentlyEquipped = false;
	}

	if (SpawnedWeapon->FireType == EFireType::EFT_Auto) {
		PlayShootMontage();
	}

	SpawnedWeapon->Shoot();
}

FHitResult UCombatComponent::TraceUnderCrosshairs() {
	FHitResult TraceHitResult;

	if (!Character) return FHitResult();

	if (!Character->IsLocallyControlled()) return FHitResult();

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
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
	Params.AddIgnoredActor(Character);
	if (SpawnedWeapon) Params.AddIgnoredActor(SpawnedWeapon);

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility, Params);

	if (!TraceHitResult.bBlockingHit) {
		TraceHitResult.ImpactPoint = End;
	}

	return TraceHitResult;
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
		MulticastResetSemiCounter();
	}
}

void UCombatComponent::PlayShootMontage() {
	if (!Character || !SpawnedWeapon) return;

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	if (SpawnedWeapon->FireType == EFireType::EFT_SemiAuto) {
		AnimInstance->Montage_Play(SpawnedWeapon->ShootMontage);
		return;
	}

	if (!AnimInstance->Montage_IsPlaying(SpawnedWeapon->ShootMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->ShootMontage);
	}
}

void UCombatComponent::PlayEquipMontage() {
	if (!Character || !SpawnedWeapon) return;

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	if (!AnimInstance->Montage_IsPlaying(SpawnedWeapon->EquipMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->EquipMontage);
	}
}

void UCombatComponent::ServerResetSemiCounter_Implementation() {
	MulticastResetSemiCounter();
}

bool UCombatComponent::ServerResetSemiCounter_Validate() {
	return true;
}

void UCombatComponent::MulticastResetSemiCounter_Implementation() {
	SemiShotCounter = 0;
}

void UCombatComponent::OnRep_SpawnedWeapon() {
	if (!Character) return;

	if (!SpawnedWeapon) {
		bShooting = false;
		SemiShotCounter = 0;
		return;
	}

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) return;

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedWeapon->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);

	if (SpawnedWeapon->GripPoint) {
		const FTransform GripRel = SpawnedWeapon->GripPoint->GetRelativeTransform();
		SpawnedWeapon->SetActorRelativeTransform(GripRel.Inverse());
	}

	PlayEquipMontage();
}

void UCombatComponent::OnRep_SemiShotCounter() {
}
