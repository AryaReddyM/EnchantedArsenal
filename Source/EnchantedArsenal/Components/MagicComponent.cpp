#include "MagicComponent.h"

#include "Chaos/ChaosPerfTest.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "EnchantedArsenal/Magic/SpellInstance.h"
#include "EnchantedArsenal/Magic/SpellVisual.h"

UMagicComponent::UMagicComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	if (SpawnedVisual) {
		DOREPLIFETIME(UMagicComponent, SpawnedVisual);
	}
	
	if (SpawnedSpell) {
		DOREPLIFETIME(UMagicComponent, SpawnedSpell);
	}
	
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
	
	USpellData* CurrentSpellData = GetSpellDataForType(SpellType);
	if (!CurrentSpellData) {
		return;
	}

	if (!CurrentSpellData->SpellVisual) {
		return;
	}

	EquippedSpellType = SpellType;
	
	ActiveSpell = NewObject<USpellInstance>(Character);
	ActiveSpell->Initialize(CurrentSpellData);
	
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	SpawnedVisual = GetWorld()->SpawnActor<ASpellVisual>(CurrentSpellData->SpellVisual, FTransform::Identity, Params);
	
	if (!SpawnedVisual) {
		return;
	}

	SpawnedVisual->Data = CurrentSpellData;
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
	if (!bTriggered) {
		bCasting = false;
		return;
	}

	if (!SpawnedVisual || !ActiveSpell || !ActiveSpell->Data) return;
	if (CastState != ECastState::Idle) return;

	bCasting = true;
	ServerCast(bTriggered);
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

	USpellData* SpellData = nullptr;
	if (ActiveSpell && ActiveSpell->Data) {
		SpellData = ActiveSpell->Data;
	} else {
		SpellData = GetSpellDataForType(EquippedSpellType);
	}

	if (!SpellData || !SpellData->Spell) {
		return;
	}

	if (Character->HasAuthority()) {
		FTransform SpawnTransform = SpawnedVisual ? SpawnedVisual->GetActorTransform() : Character->GetActorTransform();
		
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Character;
		SpawnedSpell = GetWorld()->SpawnActor<ASpell>(SpellData->Spell, SpawnTransform, Params);
		
		if (SpawnedSpell) {
			SpawnedSpell->SpellType = EquippedSpellType;
			
			SpawnedSpell->Data = SpellData;
			SpawnedSpell->InitFromData();
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
	if (!Character) return;
	
	if (!SpawnedSpell) return;
	
	USpellData* SpellData = nullptr;
	
	if (ActiveSpell && ActiveSpell->Data) {
		SpellData = ActiveSpell->Data;
	} 
	else {
		SpellData = GetSpellDataForType(EquippedSpellType);
	}

	if (!SpellData || !SpellData->Spell) {
		return;
	}
	
	FTransform SpawnTransform = SpawnedVisual ? SpawnedVisual->GetActorTransform() : Character->GetActorTransform();
		
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Character;
	SpawnedSpell = GetWorld()->SpawnActor<ASpell>(SpellData->Spell, SpawnTransform, Params);
}

void UMagicComponent::OnRep_SpawnedVisual() {
	if (!Character) return;

	if (!SpawnedVisual) {
		return;
	}

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) return;

	USpellData* SpellData = GetSpellDataForType(EquippedSpellType);
	if (SpellData) {
		SpawnedVisual->Data = SpellData;
		SpawnedVisual->InitFromData();
	}

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedVisual->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
}

void UMagicComponent::OnRep_CastState() {
}

void UMagicComponent::OnRep_EquippedSpellType() {
	if (EquippedSpellType != ESpellType::EST_None && Character) {
		USpellData* SpellData = GetSpellDataForType(EquippedSpellType);
		if (SpellData) {
			ActiveSpell = NewObject<USpellInstance>(Character);
			ActiveSpell->Initialize(SpellData);
		}
	} else {
		ActiveSpell = nullptr;
	}
}