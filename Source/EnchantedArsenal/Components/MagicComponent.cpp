#include "MagicComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"

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

	bHasPendingCast = false;
	CastState = ECastState::ECS_Idle;
}

void UMagicComponent::Cast() {
	if (!HeldSpell || CastState != ECastState::ECS_Idle) return;
	if (IsSpellOnCooldown(EquippedSpellType)) return;

	// Aim is recomputed at release time (when the notify fires), so nothing is captured here.
	ServerCast();
}

void UMagicComponent::ServerCast_Implementation() {
	if (!HeldSpell || IsSpellOnCooldown(EquippedSpellType) || CastState != ECastState::ECS_Idle) return;

	CastState = ECastState::ECS_Casting;

	if (GetCharacter()) GetCharacter()->AttackType = EAttackType::EAT_Unarmed;

	MultiCast();
}

void UMagicComponent::MultiCast_Implementation() {
	if (!GetCharacter() || !HeldSpell) return;

	bHasPendingCast = true;

	PlayCastMontage();

	// Fallback: with no cast montage there's no notify to wait for, so release immediately.
	if (!HeldSpell->CastMontage) {
		RequestRelease();
	}
}

void UMagicComponent::SpawnHeldSpell() {
	if (!GetCharacter() || !GetCharacter()->HasAuthority() || !SpellData) return;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetCharacter();

	const FTransform SpawnXf = GetCharacter()->GetMesh()->GetSocketTransform(FName("RightHandSocket"));
	HeldSpell = GetWorld()->SpawnActor<ASpell>(SpellData->Spell, SpawnXf, Params);
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

AArsenalCharacter* UMagicComponent::GetCharacter() const {
	return ::Cast<AArsenalCharacter>(GetOwner());
}

USpellData* UMagicComponent::GetSpellDataForType(ESpellType SpellType) const {
	if (SpellType == ESpellType::EST_Boulder) return Boulder;
	if (SpellType == ESpellType::EST_SpikeAdder) return SpikeAdder;
	return nullptr;
}

void UMagicComponent::PlayCastMontage() {
	if (!GetCharacter() || !HeldSpell || !HeldSpell->CastMontage) return;
	UAnimInstance* AnimInstance = GetCharacter()->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UMagicComponent::OnCastNotifyBegin);

	if (AnimInstance->Montage_Play(HeldSpell->CastMontage) > 0.f) {
		FOnMontageBlendingOutStarted BlendOut;
		BlendOut.BindUObject(this, &UMagicComponent::OnCastMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOut, HeldSpell->CastMontage);
	}
}

float UMagicComponent::GetCastMontageLength() {
	if (!HeldSpell || !HeldSpell->CastMontage) return 0.01f;
	return FMath::Max(HeldSpell->CastMontage->GetPlayLength(), 0.01f);
}

void UMagicComponent::OnCastNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload) {
	if (NotifyName == FName(TEXT("Cast Spell"))) {
		RequestRelease();
	}
}

void UMagicComponent::OnCastMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) {
	// If the notify never fired (renamed/removed, or the montage was cut short), release here so
	// the held spell still launches and the casting state can't get stuck.
	if (bHasPendingCast) {
		RequestRelease();
	}
}

void UMagicComponent::RequestRelease() {
	// Only the controlling client has a valid crosshair trace; simulated proxies and the server
	// (for a remote player) just wait for that client's ServerReleaseSpell.
	if (!bHasPendingCast || !GetCharacter() || !GetCharacter()->IsLocallyControlled()) return;
	bHasPendingCast = false;
	if (!HeldSpell) return;

	// Recompute from the CURRENT hand position and aim so moving/turning during the cast is honored.
	const FVector SpawnLoc = HeldSpell->GetActorLocation();
	const FHitResult Hit = GetCharacter()->TraceUnderCrosshairs();
	const FVector Target = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
	const FVector LaunchDir = (Target - SpawnLoc).GetSafeNormal();

	ServerReleaseSpell(SpawnLoc, LaunchDir);
}

void UMagicComponent::ServerReleaseSpell_Implementation(FVector_NetQuantize LaunchLocation, FVector_NetQuantizeNormal LaunchDir) {
	// Guard against duplicate/late releases (e.g. notify + blend-out both arriving).
	if (CastState != ECastState::ECS_Casting || !HeldSpell) return;

	HeldSpell->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeldSpell->SetActorLocationAndRotation(LaunchLocation, LaunchDir.Rotation());
	HeldSpell->CollisionIgnoreOwner();
	HeldSpell->SetHeldMode(false);
	HeldSpell->LaunchInDirection(LaunchDir);
	HeldSpell = nullptr;
	CastState = ECastState::ECS_Idle;

	// Start the cooldown now that the spell has actually fired; the timer respawns the next held
	// spell once it elapses.
	const ESpellType CastSpellType = EquippedSpellType;
	FTimerHandle& Handle = CooldownTimers.FindOrAdd(CastSpellType);
	GetWorld()->GetTimerManager().SetTimer(Handle, [this, CastSpellType]() {
		if (EquippedSpellType == CastSpellType && !HeldSpell) {
			SpawnHeldSpell();
			if (GetCharacter()) GetCharacter()->AttackType = EAttackType::EAT_Magic;
		}
	}, SpellCooldownDurations.FindRef(CastSpellType), false);
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
