#include "Spell.h"
#include "Net/UnrealNetwork.h"
#include "SpellData.h"

ASpell::ASpell() {
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("BaseRoot"));
	SetRootComponent(Root);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpell, SpellType);
}

void ASpell::BeginPlay() {
	Super::BeginPlay();
}

void ASpell::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ASpell::InitFromData() {
	if (!Data) return;
	
	if (Data->Mesh) {
		MeshComp->SetStaticMesh(Data->Mesh);
	}
	
	if (Data->Material) {
		MeshComp->SetMaterial(0, Data->Material);
	}
}

void ASpell::OnRep_SpellType() {
}
