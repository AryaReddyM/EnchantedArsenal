#include "MagicComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "TimerManager.h"
#include "Engine/World.h"

UMagicComponent::UMagicComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UMagicComponent::BeginPlay() {
	Super::BeginPlay();
	
	SpellCooldownDurations.Add(ESpellType::EST_Boulder, Boulder ? Boulder->Cooldown : 1.f);
	SpellCooldownDurations.Add(ESpellType::EST_SpikeAdder, SpikeAdder ? SpikeAdder->Cooldown : 1.f);
}

void UMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMagicComponent, HeldSpell);
	DOREPLIFETIME(UMagicComponent, CastState);
	DOREPLIFETIME(UMagicComponent, EquippedSpellType);
}

bool UMagicComponent::IsSpellOnCooldown(ESpellType SpellType) const {
	return CooldownTimers.Contains(SpellType) && GetWorld()->GetTimerManager().IsTimerActive(CooldownTimers[SpellType]);
}

void UMagicComponent::EquipSpell(ESpellType SpellType) {
	if (!GetCharacter() || !GetCharacter()->HasAuthority()) return;
	if (EquippedSpellType == SpellType && HeldSpell) return;

	UnequipSpell();

	SpellData = GetSpellDataForType(SpellType);
	if (!SpellData) return;

	EquippedSpellType = SpellType;
	
	if (!IsSpellOnCooldown(SpellType)) {
		SpawnHeldSpell();
	}
}

void UMagicComponent::UnequipSpell() {
	if (HeldSpell) {
		HeldSpell->Destroy();
		HeldSpell = nullptr;
	}
	EquippedSpellType = ESpellType::EST_None;
	SpellData = nullptr;
}

void UMagicComponent::Cast(bool bTriggered) {
	if (!bTriggered || !HeldSpell || CastState != ECastState::ECS_Idle) return;
	if (IsSpellOnCooldown(EquippedSpellType)) return;

	FVector SpawnLoc = HeldSpell->GetActorLocation();
	const FHitResult Hit = GetCharacter()->TraceUnderCrosshairs();
	FVector Target = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
	FVector LaunchDir = (Target - SpawnLoc).GetSafeNormal();

	ServerCast(true, SpawnLoc, LaunchDir);
}

void UMagicComponent::ServerCast_Implementation(bool bTriggered, FVector_NetQuantize LaunchLocation, FVector_NetQuantizeNormal LaunchDir) {
	if (!HeldSpell || IsSpellOnCooldown(EquippedSpellType)) return;

	ESpellType CastSpellType = EquippedSpellType;
	FTimerHandle& Handle = CooldownTimers.FindOrAdd(CastSpellType);
	GetWorld()->GetTimerManager().SetTimer(Handle, [this, CastSpellType]() {
		if (EquippedSpellType == CastSpellType && !HeldSpell) {
			SpawnHeldSpell();
			if (GetCharacter()) GetCharacter()->AttackType = EAttackType::EAT_Magic;
		}
	}, SpellCooldownDurations.FindRef(CastSpellType), false);

	if (GetCharacter()) GetCharacter()->AttackType = EAttackType::EAT_Unarmed;

	MultiCast(bTriggered, LaunchLocation, LaunchDir);
}

void UMagicComponent::MultiCast_Implementation(bool bTriggered, FVector_NetQuantize LaunchLocation, FVector_NetQuantizeNormal LaunchDir) {
	if (!GetCharacter() || !HeldSpell) return;

	HeldSpell->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeldSpell->SetActorLocationAndRotation(LaunchLocation, LaunchDir.Rotation());
	HeldSpell->CollisionIgnoreOwner();
	HeldSpell->SetHeldMode(false);

	if (GetCharacter()->HasAuthority()) {
		HeldSpell->LaunchInDirection(LaunchDir);
		HeldSpell = nullptr;
	}
}

void UMagicComponent::SpawnHeldSpell() {
	if (!GetCharacter() || !GetCharacter()->HasAuthority() || !SpellData) return;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetCharacter();

	HeldSpell = GetWorld()->SpawnActor<ASpell>(SpellData->Spell, FTransform::Identity, Params);
	if (HeldSpell) {
		HeldSpell->SpellType = EquippedSpellType;
		HeldSpell->Data = SpellData;
		HeldSpell->InitFromData();
		HeldSpell->SetHeldMode(true);
		HeldSpell->CollisionIgnoreOwner();
		AttachHeldSpell();
	}
}

void UMagicComponent::AttachHeldSpell() {
	if (HeldSpell && GetCharacter()) {
		HeldSpell->AttachToComponent(GetCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("RightHandSocket"));
	}
}

void UMagicComponent::OnRep_EquippedSpellType() {
	SpellData = GetSpellDataForType(EquippedSpellType);
}

void UMagicComponent::OnRep_HeldSpell() {
	if (HeldSpell) {
		HeldSpell->SetHeldMode(true);
		HeldSpell->CollisionIgnoreOwner();
		AttachHeldSpell();
	}
}

AArsenalCharacter* UMagicComponent::GetCharacter() const {
	return ::Cast<AArsenalCharacter>(GetOwner());
}

USpellData* UMagicComponent::GetSpellDataForType(ESpellType SpellType) const {
	if (SpellType == ESpellType::EST_Boulder) return Boulder;
	if (SpellType == ESpellType::EST_SpikeAdder) return SpikeAdder;
	return nullptr;
}
