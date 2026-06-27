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
	
	PlayEquipMontage();
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
	if (!HeldSpell || CastState != ECastState::ECS_Idle || bHasPendingCast) return;
	if (IsSpellOnCooldown(EquippedSpellType)) return;

	bHasPendingCast = true; 

	ServerCast();
}

void UMagicComponent::ServerCast_Implementation() {
	if (!HeldSpell || IsSpellOnCooldown(EquippedSpellType) || CastState != ECastState::ECS_Idle) {
		ClientResetCastState();
		return;
	}

	CastState = ECastState::ECS_Casting;
    
	if (GetCharacter()) {
		GetCharacter()->AttackType = EAttackType::EAT_Unarmed;
	}
    
	MultiCast();
}

void UMagicComponent::MultiCast_Implementation() {
	if (!GetCharacter()) return;

	ASpell* LocalHeldSpell = HeldSpell;
	if (!LocalHeldSpell) return;

	bHasPendingCast = true;
	PlayCastMontage();

	if (!LocalHeldSpell->Data || !LocalHeldSpell->Data->CastMontage) {
		RequestRelease();
	}
}

void UMagicComponent::ClientResetCastState_Implementation() {
	bHasPendingCast = false;
}

void UMagicComponent::ServerReleaseSpell_Implementation(FVector_NetQuantize LaunchLocation, FVector_NetQuantizeNormal LaunchDir) {
	if (CastState != ECastState::ECS_Casting || !HeldSpell) return;

	HeldSpell->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeldSpell->SetActorLocationAndRotation(LaunchLocation, LaunchDir.Rotation());
	HeldSpell->CollisionIgnoreOwner();
	HeldSpell->SetHeldMode(false);
	HeldSpell->LaunchInDirection(LaunchDir);
	HeldSpell = nullptr;
	CastState = ECastState::ECS_Idle;

	const ESpellType CastSpellType = EquippedSpellType;
	FTimerHandle& Handle = CooldownTimers.FindOrAdd(CastSpellType);
	GetWorld()->GetTimerManager().SetTimer(Handle, [this, CastSpellType]() {
		if (EquippedSpellType == CastSpellType && !HeldSpell) {
			SpawnHeldSpell();
			if (GetCharacter()) GetCharacter()->AttackType = EAttackType::EAT_Magic;
		}
	}, SpellCooldownDurations.FindRef(CastSpellType), false);
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
	if (!GetCharacter()) return;

	ASpell* LocalHeldSpell = HeldSpell;
	if (!LocalHeldSpell || !LocalHeldSpell->Data || !LocalHeldSpell->Data->CastMontage) return;

	UAnimMontage* MontageToPlay = LocalHeldSpell->Data->CastMontage;
	if (!MontageToPlay) return;

	USkeletalMeshComponent* Mesh = GetCharacter()->GetMesh();
	if (!Mesh) return;

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance || !::IsValid(AnimInstance)) return;

	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UMagicComponent::OnCastNotifyBegin);

	const float SectionLength = AnimInstance->Montage_Play(MontageToPlay);

	if (SectionLength > 0.f && ::IsValid(AnimInstance)) {
		FOnMontageBlendingOutStarted BlendOut;
		BlendOut.BindUObject(this, &UMagicComponent::OnCastMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOut, MontageToPlay);
	}
}

void UMagicComponent::PlayEquipMontage() {
	if (!GetCharacter() || !HeldSpell || !HeldSpell->Data || !HeldSpell->Data->EquipMontage) return;
	UAnimInstance* AnimInstance = GetCharacter()->GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(HeldSpell->Data->EquipMontage)) {
		AnimInstance->Montage_Play(HeldSpell->Data->EquipMontage);
	}
}

float UMagicComponent::GetCastMontageLength() {
	if (!HeldSpell || !HeldSpell->Data || !HeldSpell->Data->CastMontage) return 0.01f;
	
	return FMath::Max(HeldSpell->Data->CastMontage->GetPlayLength(), 0.01f);
}

float UMagicComponent::GetEquipMontageLength() {
	if (!HeldSpell || !HeldSpell->Data || !HeldSpell->Data->CastMontage) return 0.01f;
	
	return FMath::Max(HeldSpell->Data->CastMontage->GetPlayLength(), 0.01f);
}

UTexture2D* UMagicComponent::GetSpellIconForType(ESpellType Type) {
	return GetSpellDataForType(Type)->Icon;
}

void UMagicComponent::OnCastNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload) {
	if (NotifyName == FName(TEXT("Cast Spell"))) {
		RequestRelease();
	}
}

void UMagicComponent::OnCastMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted) {
	if (bHasPendingCast) {
		RequestRelease();
	}
}

void UMagicComponent::RequestRelease() {
	if (!bHasPendingCast || !GetCharacter() || !GetCharacter()->IsLocallyControlled()) return;
	bHasPendingCast = false;
	if (!HeldSpell) return;

	const FVector SpawnLoc = HeldSpell->GetActorLocation();
	const FHitResult Hit = GetCharacter()->TraceUnderCrosshairs();
	const FVector Target = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
	const FVector LaunchDir = (Target - SpawnLoc).GetSafeNormal();

	ServerReleaseSpell(SpawnLoc, LaunchDir);
}

void UMagicComponent::OnRep_EquippedSpellType() {
	SpellData = GetSpellDataForType(EquippedSpellType);
}

void UMagicComponent::OnRep_HeldSpell() {
	if (HeldSpell) {
		HeldSpell->SetHeldMode(true);
		HeldSpell->CollisionIgnoreOwner();
		AttachHeldSpell();
		
		PlayEquipMontage();
	}
}
