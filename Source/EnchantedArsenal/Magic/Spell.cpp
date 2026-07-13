#include "Spell.h"
#include "Engine/Engine.h"
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

	MergeCollision = CreateDefaultSubobject<USphereComponent>(TEXT("MergeCollision"));
	MergeCollision->SetupAttachment(Collision);
	MergeCollision->InitSphereRadius(200.0f);
	MergeCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MergeCollision->SetCollisionObjectType(ECC_WorldDynamic);
	MergeCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	MergeCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	MergeCollision->SetGenerateOverlapEvents(true);
	MergeCollision->OnComponentBeginOverlap.AddDynamic(this, &ASpell::OnMergeOverlap);

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
	if (MeshComp && Data->GetMesh(SpellTags)) MeshComp->SetStaticMesh(Data->GetMesh(SpellTags));
	if (MeshComp && Data->GetMaterial(SpellTags)) MeshComp->SetMaterial(0, Data->GetMaterial(SpellTags));
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
}

void ASpell::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
}

void ASpell::OnMergeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	HandleSpellMerge(OtherActor);
}

void ASpell::HandleSpellMerge(AActor* OtherActor) {
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;

	ABaseSpell* Base = Cast<ABaseSpell>(this);
	if (!Base) return;

	AModifierSpell* Modifier = Cast<AModifierSpell>(OtherActor);
	if (!Modifier || !Modifier->Data) return;

	if (Modifier->bIsHeld) return;

	if (AArsenalPlayerState::IsHostile(GetInstigator(), Modifier->GetInstigator())) return;

	SpellTags.AppendTags(Modifier->Data->ComboGrantTags);
	OnRep_SpellTags();
	Modifier->Destroy();
}

bool ASpell::IsEnemy(AActor* OtherActor) {
	return AArsenalPlayerState::IsHostile(GetInstigator(), OtherActor);
}

void ASpell::OnRep_Data() {
	InitFromData();
}

void ASpell::OnRep_SpellTags() {
	if (MeshComp && Data && Data->GetMaterial(SpellTags)) {
		MeshComp->SetMaterial(0, Data->GetMaterial(SpellTags));
	}
	
	if (MeshComp && Data && Data->GetMesh(SpellTags)) {
		MeshComp->SetStaticMesh(Data->GetMesh(SpellTags));
	}
}