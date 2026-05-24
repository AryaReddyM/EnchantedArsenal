#include "CombatComponent.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay() {
	Super::BeginPlay();
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
}

void UCombatComponent::EquipWeapon(EWeaponType WeaponType) {
	if (!Character || !Character->HasAuthority()) return;

	UnequipWeapon();

	TSubclassOf<AWeapon> WeaponClass = nullptr;
	switch (WeaponType) {
	case EWeaponType::EWT_Rifle: WeaponClass = Rifle;
		break;
	case EWeaponType::EWT_Shotgun: WeaponClass = Shotgun;
		break;
	case EWeaponType::EWT_SMG: WeaponClass = SMG;
		break;
	case EWeaponType::EWT_Pistol: WeaponClass = Pistol;
		break;
	default: return;
	}

	if (!WeaponClass) return;

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;
	SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FTransform::Identity, Params);

	if (SpawnedWeapon) {
		const FName HandSocket(TEXT("RightHandSocket"));
		SpawnedWeapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform,
										 HandSocket);

		if (SpawnedWeapon->GripPoint) {
			SpawnedWeapon->SetActorRelativeTransform(SpawnedWeapon->GripPoint->GetRelativeTransform().Inverse());
		}

		if (int* CachedAmmo = AmmoReserve.Find(WeaponType)) {
			SpawnedWeapon->CurrentAmmo = *CachedAmmo;
		}

		PlayEquipMontage();
		SpawnedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::HandleAmmoChanged);
		HandleAmmoChanged(SpawnedWeapon->CurrentAmmo, SpawnedWeapon->MagSize);
	}
}

void UCombatComponent::UnequipWeapon() {
	if (!Character || !Character->HasAuthority()) return;

	if (SpawnedWeapon) {
		AmmoReserve.Add(SpawnedWeapon->WeaponType, SpawnedWeapon->CurrentAmmo);
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}

	GetWorld()->GetTimerManager().ClearTimer(ShootTimer);
}

void UCombatComponent::Shoot() {
	if (!SpawnedWeapon || !Character) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastShootTime < SpawnedWeapon->ShootRate) return;

	FireOneShot();

	if (SpawnedWeapon->FireType == EFireType::EFT_Auto) {
		GetWorld()->GetTimerManager().SetTimer(ShootTimer, this, &UCombatComponent::FireOneShot, SpawnedWeapon->ShootRate, true);
	}
}

void UCombatComponent::StopShoot() {
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(ShootTimer);
	}
}

void UCombatComponent::FireOneShot() {
	if (!SpawnedWeapon || !Character) {
		StopShoot();
		return;
	}

	if (Character->IsLocallyControlled()) {
		PlayShootMontage();
	}

	SpawnedWeapon->Shoot();
	LastShootTime = GetWorld()->GetTimeSeconds();
}

void UCombatComponent::HandleAmmoChanged(int32 NewAmmo, int32 MaxAmmo) {
	if (!Character || !Character->IsLocallyControlled() || !Character->HUD) return;

	if (UTextBlock* AmmoText = Cast<UTextBlock>(Character->HUD->GetWidgetFromName("AmmoText"))) {
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), NewAmmo, MaxAmmo)));
	}
}

void UCombatComponent::ResetAmmo() {
	for (EWeaponType Type : TEnumRange<EWeaponType>()) {
		if (GetWeaponClass(Type)) {
			GetWeaponClass(Type)->Reload();
		}
	}
}

void UCombatComponent::PlayShootMontage() {
	if (!Character || !SpawnedWeapon || !SpawnedWeapon->ShootMontage) return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	if (SpawnedWeapon->FireType == EFireType::EFT_SemiAuto) {
		AnimInstance->Montage_Play(SpawnedWeapon->ShootMontage);
	}
	else if (!AnimInstance->Montage_IsPlaying(SpawnedWeapon->ShootMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->ShootMontage);
	}
}

void UCombatComponent::PlayEquipMontage() {
	if (!Character || !SpawnedWeapon || !SpawnedWeapon->EquipMontage) return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(SpawnedWeapon->EquipMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->EquipMontage);
	}
}

void UCombatComponent::PlayReloadMontage() {
	if (!Character || !SpawnedWeapon || !SpawnedWeapon->ReloadMontage) return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(SpawnedWeapon->ReloadMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->ReloadMontage);
	}
}

float UCombatComponent::GetEquipMontageLength() {
	if (!SpawnedWeapon || !SpawnedWeapon->EquipMontage) return 0.01f;
	return FMath::Max(SpawnedWeapon->EquipMontage->GetPlayLength(), 0.01f);
}

float UCombatComponent::GetReloadMontageLength() {
	if (!SpawnedWeapon || !SpawnedWeapon->ReloadMontage) return 0.01f;
	return FMath::Max(SpawnedWeapon->ReloadMontage->GetPlayLength(), 0.01f);
}

AWeapon* UCombatComponent::GetWeaponClass(EWeaponType Type) const {
	switch (Type) {
	case EWeaponType::EWT_Rifle:   
		return Rifle->GetDefaultObject<AWeapon>();
	case EWeaponType::EWT_Shotgun: 
		return Shotgun->GetDefaultObject<AWeapon>();
	case EWeaponType::EWT_SMG:     
		return SMG->GetDefaultObject<AWeapon>();
	case EWeaponType::EWT_Pistol:  
		return Pistol->GetDefaultObject<AWeapon>();
	default:                       
		return nullptr;
	}
}

void UCombatComponent::OnRep_SpawnedWeapon() {
	if (!Character || !SpawnedWeapon) return;

	SpawnedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::HandleAmmoChanged);
	HandleAmmoChanged(SpawnedWeapon->CurrentAmmo, SpawnedWeapon->MagSize);

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedWeapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
	                                 HandSocket);

	if (SpawnedWeapon->GripPoint) {
		SpawnedWeapon->SetActorRelativeTransform(SpawnedWeapon->GripPoint->GetRelativeTransform().Inverse());
	}
	PlayEquipMontage();
}
