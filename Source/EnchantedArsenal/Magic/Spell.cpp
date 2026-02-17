#include "Spell.h"
#include "Net/UnrealNetwork.h"

ASpell::ASpell() {
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	BaseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BaseRoot"));
	SetRootComponent(BaseRoot);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

void ASpell::OnRep_SpellType() {
}
