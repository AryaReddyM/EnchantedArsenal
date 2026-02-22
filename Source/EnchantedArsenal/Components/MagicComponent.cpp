#include "MagicComponent.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"

UMagicComponent::UMagicComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMagicComponent, SpawnedSpell);
	DOREPLIFETIME(UMagicComponent, bCasting);
}


void UMagicComponent::BeginPlay() {
	Super::BeginPlay();
}


void UMagicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMagicComponent::EquipSpell(ESpellType SpellType) {
	if (!Character || !Character->HasAuthority()) return;

	UnequipSpell();

	if (SpawnedSpell) {
		SpawnedSpell->Destroy();
		SpawnedSpell = nullptr;
	}

	TSubclassOf<ASpell> SpellClass = nullptr;
	switch (SpellType) {
	case ESpellType::EST_Boulder:   SpellClass = Boulder;   break;
	case ESpellType::EST_SpikeAdder: SpellClass = SpikeAdder; break;
	default: return;
	}
	if (!SpellClass) return;

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	ASpell* NewSpell = GetWorld()->SpawnActor<ASpell>(SpellClass, FTransform::Identity, Params);
	if (!NewSpell) return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) {
		NewSpell->Destroy();
		return;
	}

	const FName HandSocket(TEXT("RightHandSocket"));
	NewSpell->AttachToComponent(CharMesh, FAttachmentTransformRules::KeepRelativeTransform, HandSocket);

	SpawnedSpell = NewSpell;
}

void UMagicComponent::UnequipSpell() {
	if (!Character || !Character->HasAuthority()) return;

	if (SpawnedSpell) {
		SpawnedSpell->Destroy();
		SpawnedSpell = nullptr;
	}
}

void UMagicComponent::Cast(bool bTriggered) {
	if (!SpawnedSpell) return;

	bCasting = bTriggered;
	ServerCast(bTriggered);
}

void UMagicComponent::ServerCast_Implementation(bool bTriggered) {
	bCasting = bTriggered;
	MultiCast(bTriggered);
}

void UMagicComponent::MultiCast_Implementation(bool bTriggered) {
	if (!SpawnedSpell || !Character) return;

	if (!bTriggered) return;
}

void UMagicComponent::OnRep_SpawnedSpell() {
	if (!Character || !SpawnedSpell) return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	if (!CharMesh) return;

	const FName HandSocket(TEXT("RightHandSocket"));
	SpawnedSpell->AttachToComponent(CharMesh, FAttachmentTransformRules::KeepRelativeTransform, HandSocket);
}