#include "Spell.h"
#include "Net/UnrealNetwork.h"
#include "SpellData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EnchantedArsenal/Components/MagicComponent.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "EnchantedArsenal/Magic/SpellData.h"

ASpell::ASpell() {
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(100.0f);
	SetMinNetUpdateFrequency(33.0f);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);

	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentHit.AddDynamic(this, &ASpell::OnHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASpell::OnBeginOverlap);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Collision);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjComp->bRotationFollowsVelocity = true;
	ProjComp->bAutoActivate = false;
}

void ASpell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpell, SpellType);
	DOREPLIFETIME(ASpell, Data);
	DOREPLIFETIME(ASpell, SpellTags);
	DOREPLIFETIME(ASpell, bIsHeld);
}

void ASpell::InitFromData() {
	if (!Data) return;
	SpellTags.AppendTags(Data->DefaultTags);
	if (MeshComp && Data->Mesh) MeshComp->SetStaticMesh(Data->Mesh);
	if (MeshComp && Data->Material) MeshComp->SetMaterial(0, Data->GetMaterial(SpellTags));
	if (ProjComp) {
		ProjComp->InitialSpeed = Data->InitialSpeed;
		ProjComp->MaxSpeed = Data->Speed;
		ProjComp->ProjectileGravityScale = Data->GravityScale;
	}
}

void ASpell::CollisionIgnoreOwner() {
	APawn* InstPawn = GetInstigator();
	if (!InstPawn) return;

	if (Collision) {
		Collision->IgnoreActorWhenMoving(InstPawn, true);
	}

	InstPawn->MoveIgnoreActorAdd(this);

	TArray<UPrimitiveComponent*> PlayerComponents;
	InstPawn->GetComponents<UPrimitiveComponent>(PlayerComponents);
	for (UPrimitiveComponent* Comp : PlayerComponents) {
		if (Collision) {
			Collision->IgnoreComponentWhenMoving(Comp, true);
		}
	}
}

void ASpell::SetHeldMode(bool bHeld) {
	bIsHeld = bHeld;
	if (bHeld) {
		SetReplicateMovement(false);
		ProjComp->Deactivate();

		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetCollisionResponseToAllChannels(ECR_Overlap);

		CollisionIgnoreOwner();
	} 
	else {
		SetReplicateMovement(true);

		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

		CollisionIgnoreOwner();
	}
}

void ASpell::LaunchInDirection(const FVector& Dir) {
	if (ProjComp) {    
		ProjComp->Velocity = Dir.GetSafeNormal() * ProjComp->InitialSpeed;
		ProjComp->Activate(true);
	}
}

void ASpell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	HandleSpellMerge(OtherActor);
}

void ASpell::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	HandleSpellMerge(OtherActor);
}

void ASpell::HandleSpellMerge(AActor* OtherActor) {
	if (!HasAuthority() || !OtherActor) return;
	if (OtherActor == GetInstigator() || OtherActor == GetOwner()) return;

	ASpell* OtherSpell = Cast<ASpell>(OtherActor);
	if (!OtherSpell || !OtherSpell->Data) return;

	if (!bIsHeld || OtherSpell->bIsHeld) return;

	// Don't combo with hostile spells — the flying spell should pass through and damage the caster.
	if (AArsenalPlayerState::IsHostile(GetInstigator(), OtherSpell->GetInstigator())) return;

	SpellTags.AppendTags(OtherSpell->Data->ComboGrantTags);
	OnRep_SpellTags();
	OtherSpell->Destroy();
}

bool ASpell::IsEnemy(AActor* OtherActor) {
	return AArsenalPlayerState::IsHostile(GetInstigator(), OtherActor);
}

void ASpell::OnRep_Data() {
	InitFromData();
}

void ASpell::OnRep_SpellTags() {
	if (MeshComp && Data && Data->Material) {
		MeshComp->SetMaterial(0, Data->GetMaterial(SpellTags));
	}
}