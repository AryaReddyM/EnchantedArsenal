#include "MagicComponent.h"

#include "Chaos/ChaosPerfTest.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "EnchantedArsenal/Magic/SpellInstance.h"
#include "EnchantedArsenal/Magic/SpellVisual.h"
#include "GameFramework/Pawn.h"

UMagicComponent::UMagicComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMagicComponent, SpawnedVisual);
	DOREPLIFETIME(UMagicComponent, SpawnedSpell);
	DOREPLIFETIME(UMagicComponent, bCasting);
	DOREPLIFETIME(UMagicComponent, CastState);
	DOREPLIFETIME(UMagicComponent, EquippedSpellType);
}

void UMagicComponent::BeginPlay() {
	Super::BeginPlay();
}

void UMagicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CastState == ECastState::Cooldown && ActiveSpell && ActiveSpell->Data) {
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastCastTime >= ActiveSpell->Data->Cooldown) {
			CastState = ECastState::Idle;
		}
	}
}

USpellData* UMagicComponent::GetSpellDataForType(ESpellType SpellType) const {
	switch (SpellType) {
	case ESpellType::EST_Boulder:
		return Boulder;
	case ESpellType::EST_SpikeAdder:
		return SpikeAdder;
	default:
		return nullptr;
	}
}

void UMagicComponent::EquipSpell(ESpellType SpellType) {
	if (!Character || !Character->HasAuthority()) return;
	
	if (EquippedSpellType == SpellType && SpawnedVisual) return;
	
	UnequipSpell();
	
	SpellData = GetSpellDataForType(SpellType);
	if (!SpellData || !SpellData->SpellVisual) return;

	EquippedSpellType = SpellType;
	
	ActiveSpell = NewObject<USpellInstance>(Character);
	ActiveSpell->Initialize(SpellData);
	
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	SpawnedVisual = GetWorld()->SpawnActor<ASpellVisual>(SpellData->SpellVisual, FTransform::Identity, Params);
	
	if (!SpawnedVisual) {
		return;
	}

	SpawnedVisual->Data = SpellData;
	SpawnedVisual->InitFromData();
	
	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) {
		SpawnedVisual->Destroy();
		SpawnedVisual = nullptr;
		return;
	}

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedVisual->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);

	CastState = ECastState::Idle;
}

void UMagicComponent::UnequipSpell() {
	if (!Character || !Character->HasAuthority()) return;

	if (SpawnedVisual) {
		SpawnedVisual->Destroy();
		SpawnedVisual = nullptr;
	}

	ActiveSpell = nullptr;
	EquippedSpellType = ESpellType::EST_None;
	CastState = ECastState::Idle;
	bCasting = false;
}

void UMagicComponent::Cast(bool bTriggered) {
	if (!SpawnedVisual) return;
	
	if (!bTriggered) {
		bCasting = false;

		return;
	}

	if (EquippedSpellType == ESpellType::EST_None) return;

	if (CastState != ECastState::Idle) return;
	
	
	SpawnLocation = SpawnedVisual ? SpawnedVisual->GetActorLocation() : Character->GetActorLocation();

	const FHitResult CrosshairHitResult = Character->TraceUnderCrosshairs();
	const FVector TargetPoint = CrosshairHitResult.bBlockingHit ? CrosshairHitResult.ImpactPoint : CrosshairHitResult.TraceEnd;
	Dir = (TargetPoint - SpawnLocation).GetSafeNormal();

	bCasting = true;
	ServerCast(true);
}

void UMagicComponent::ServerCast_Implementation(bool bTriggered) {
	if (!bTriggered) {
		bCasting = false;
		return;
	}

	if (!ActiveSpell || !ActiveSpell->Data) return;
	if (CastState != ECastState::Idle) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCastTime < ActiveSpell->Data->Cooldown) return;

	bCasting = true;
	LastCastTime = CurrentTime;
	CastState = ECastState::Casting;
	
	MultiCast(bTriggered);
}

void UMagicComponent::MultiCast_Implementation(bool bTriggered) {
	if (!bTriggered) return;
	if (!Character) return;

	if (Character->HasAuthority()) {
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Character;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedSpell = GetWorld()->SpawnActorDeferred<ASpell>(
			SpellData->Spell, 
			FTransform(Dir.Rotation(), SpawnLocation), 
			Params.Owner, 
			Params.Instigator, 
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (SpawnedSpell) {
			SpawnedSpell->CollisionIgnoreOwner();
			
			SpawnedSpell->SpellType = EquippedSpellType;
			SpawnedSpell->Data = SpellData;

			SpawnedSpell->InitFromData();
			
			SpawnedSpell->FinishSpawning(FTransform(Dir.Rotation(), SpawnLocation));

			SpawnedSpell->LaunchInDirection(Dir);
		}
	}
	
	if (SpawnedVisual) {
		SpawnedVisual->Destroy();
		SpawnedVisual = nullptr;
	}

	if (Character->HasAuthority()) {
		CastState = ECastState::Cooldown;
		bCasting = false;
	}
}

void UMagicComponent::OnRep_SpawnedSpell() {
	if (!SpawnedSpell) return;

	SpawnedSpell->InitFromData();
}

void UMagicComponent::OnRep_SpawnedVisual() {
	if (!Character || !SpawnedVisual) return;

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh) return;

	static const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedVisual->AttachToComponent(
		Mesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		HandSocket
	);

	SpawnedVisual->InitFromData();
}

void UMagicComponent::OnRep_CastState() {
}

void UMagicComponent::OnRep_EquippedSpellType() {
	if (EquippedSpellType != ESpellType::EST_None && Character) {
		if (SpellData) {
			ActiveSpell = NewObject<USpellInstance>(Character);
			ActiveSpell->Initialize(SpellData);
		}
	} 
	else {
		ActiveSpell = nullptr;
	}
}