#include "Spell.h"
#include "Net/UnrealNetwork.h"
#include "SpellData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EnchantedArsenal/Components/MagicComponent.h"

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
	Collision->OnComponentHit.AddDynamic(this, &ASpell::OnHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASpell::OnOverlap);

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
	if (MeshComp && Data->Mesh) MeshComp->SetStaticMesh(Data->Mesh);
	if (MeshComp && Data->Material) MeshComp->SetMaterial(0, Data->Material);
	if (ProjComp) {
		ProjComp->InitialSpeed = Data->InitialSpeed;
		ProjComp->MaxSpeed = Data->Speed;
		ProjComp->ProjectileGravityScale = Data->GravityScale;
	}
	SpellTags.AppendTags(Data->DefaultTags);
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
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	} else {
		SetReplicateMovement(true);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
	if (bIsHeld || !OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;

	if (HasAuthority()) {
		if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>()) {
			HealthComp->ApplyDamage(Data ? Data->Damage : 10.f);
			
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Health: ") + FString::SanitizeFloat(HealthComp->CurrentHealth));
		}
		Destroy();
	}
}

void ASpell::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (!OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;
	
	ASpell* OtherSpell = Cast<ASpell>(OtherActor);
	if (!OtherSpell) {
		return;
	}
	
	if (HasAuthority()) {
		switch (OtherSpell->SpellType) {
		case ESpellType::EST_SpikeAdder:
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("Spike Adder"));
			break;
		default:
			break;
		}
	}
}

void ASpell::OnRep_Data() { InitFromData(); }
