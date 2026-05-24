#include "CombatComponent.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "TimerManager.h"

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

void UCombatComponent::Shoot(bool bTriggered) {
	if (!SpawnedWeapon) return;

	if (bTriggered) {
		GetWorld()->GetTimerManager().SetTimer(ShootTimer, FTimerDelegate::CreateLambda([this]() {
			if (!SpawnedWeapon || !Character) return;

			if (Character->IsLocallyControlled()) {
				PlayShootMontage();
			}

			SpawnedWeapon->Shoot();

			if (SpawnedWeapon->FireType == EFireType::EFT_SemiAuto) {
				GetWorld()->GetTimerManager().ClearTimer(ShootTimer);
			}
		}), SpawnedWeapon->ShootRate, true, 0.0f);
	}
	else {
		GetWorld()->GetTimerManager().ClearTimer(ShootTimer);
	}
}

void UCombatComponent::HandleAmmoChanged(int32 NewAmmo, int32 MagSize) {
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Ammo: %d/%d"), NewAmmo, MagSize));
}

void UCombatComponent::ResetAmmo() {
	for (EWeaponType Type : TEnumRange<EWeaponType>()) {
		if (GetWeaponClass(Type)) {
			GetWeaponClass(Type)->Reload();
		}
	}
}

void UCombatComponent::PlayShootMontage() {
	if (!Character || !SpawnedWeapon) return;
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
	if (!Character || !SpawnedWeapon) return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(SpawnedWeapon->EquipMontage)) {
		AnimInstance->Montage_Play(SpawnedWeapon->EquipMontage);
	}
}

float UCombatComponent::GetEquipMontageLength() {
	return SpawnedWeapon->EquipMontage->GetPlayLength();
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
