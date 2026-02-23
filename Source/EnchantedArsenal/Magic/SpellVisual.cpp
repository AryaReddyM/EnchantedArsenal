#include "SpellVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "SpellData.h"

ASpellVisual::ASpellVisual() {
	SetReplicates(true);
	bReplicates = true;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Root);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpellVisual::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ASpellVisual::InitFromData() {
	if (!Data) return;
	
	if (Data->Mesh) {
		MeshComp->SetStaticMesh(Data->Mesh);
	}
	
	if (Data->Material) {
		MeshComp->SetMaterial(0, Data->Material);
	}
}