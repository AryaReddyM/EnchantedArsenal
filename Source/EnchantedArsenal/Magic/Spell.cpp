#include "Spell.h"
#include "Net/UnrealNetwork.h"
#include "SpellData.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ASpell::ASpell() {
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	AActor::SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->OnComponentHit.AddDynamic(this, &ASpell::OnHit);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Collision);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjComp->UpdatedComponent = Collision;
	ProjComp->bRotationFollowsVelocity = true;
	ProjComp->bAutoActivate = false;
}

void ASpell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASpell, SpellType);
	DOREPLIFETIME(ASpell, Data);
}

void ASpell::BeginPlay() {
	Super::BeginPlay();
}

void ASpell::InitFromData() {
	if (!Data || !ProjComp || !MeshComp) return;

	if (Data->Mesh) MeshComp->SetStaticMesh(Data->Mesh);
	if (Data->Material) MeshComp->SetMaterial(0, Data->Material);

	ProjComp->InitialSpeed = Data->InitialSpeed;
	ProjComp->MaxSpeed = Data->Speed;

	ProjComp->ProjectileGravityScale = Data->GravityScale;
}

void ASpell::LaunchInDirection(const FVector& Dir) {
	if (!ProjComp) return;

	const FVector N = Dir.GetSafeNormal();
	ProjComp->Velocity = N * ProjComp->InitialSpeed;
	ProjComp->Activate(true);
}

void ASpell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	if (!OtherActor || OtherActor == GetOwner()) {
		Destroy();
		return;
	}

	if (HasAuthority()) {
		if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>()) {
				HealthComp->ApplyDamage(Data->Damage);

			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Health: ") + FString::SanitizeFloat(HealthComp->CurrentHealth));
		}
		
		Destroy();
	}
}

void ASpell::CollisionIgnoreOwner() {
	APawn* InstPawn = GetInstigator();
	if (!InstPawn) return;

	if (Collision) {
		Collision->IgnoreActorWhenMoving(InstPawn, true);
	}
	
	if (MeshComp) {
		MeshComp->IgnoreActorWhenMoving(InstPawn, true);
	}

	InstPawn->MoveIgnoreActorAdd(this);
}

void ASpell::OnRep_SpellType() {
}

void ASpell::OnRep_Data() {
	InitFromData();
}
