#include "Boulder.h"

#include "EnchantedArsenal/Components/HealthComponent.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"

ABoulder::ABoulder() {
	PrimaryActorTick.bCanEverTick = true;
}

void ABoulder::BeginPlay() {
	Super::BeginPlay();
}

void ABoulder::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ABoulder::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

	if (!HasAuthority() || !OtherActor) return;
	if (OtherActor == GetInstigator() || OtherActor == GetOwner()) return;
	if (Cast<ASpell>(OtherActor)) return;

	const FVector Detonation = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : GetActorLocation();
	Explode(OtherActor, Detonation);
}

void ABoulder::Explode(AActor* DirectHitActor, const FVector& Origin) {
	ShowBlast();

	if (Data && Data->GetExplosionRadius(SpellTags) > 0.0f) {
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(GetInstigator());

		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjParams, FCollisionShape::MakeSphere(Data->GetExplosionRadius(SpellTags)), QueryParams);

		TSet<AActor*> AlreadyDamaged;
		for (const FOverlapResult& Result : Overlaps) {
			AActor* OverlappedActor = Result.GetActor();
			if (!OverlappedActor || AlreadyDamaged.Contains(OverlappedActor)) continue;
			if (!IsEnemy(OverlappedActor)) continue;

			if (UHealthComponent* HealthComp = OverlappedActor->FindComponentByClass<UHealthComponent>()) {
				AlreadyDamaged.Add(OverlappedActor);

				float Distance = 0.0f;
				FVector ImpactPoint = Origin;
				if (OverlappedActor != DirectHitActor) {
					ImpactPoint = OverlappedActor->GetActorLocation();
					if (UPrimitiveComponent* Prim = Result.GetComponent()) {
						Prim->GetClosestPointOnCollision(Origin, ImpactPoint);
					}
					Distance = FVector::Dist(Origin, ImpactPoint);
				}
				const float ExplosionDamage = Data->GetExplosionDamage(SpellTags, Distance);
				HealthComp->ApplyDamage(ExplosionDamage, GetInstigator());
				
				FVector ImpactNormal = (ImpactPoint - Origin).GetSafeNormal();
				if (ImpactNormal.IsNearlyZero()) {
					ImpactNormal = (Origin - GetActorLocation()).GetSafeNormal();
				}
				ShowImpactOnPawn(ImpactPoint, ImpactNormal);
			}
		}
	}

	Destroy();
}

void ABoulder::ShowBlast_Implementation() {
	if (!Data) return;
	
	float TotalRadius = Data->GetExplosionRadius(SpellTags);
	float BaseRadius = Data->ExplosionRadius;
	float ScaleMultiplier = (BaseRadius > 0.0f) ? (TotalRadius / BaseRadius) : 1.0f;

	DrawDebugSphere(GetWorld(), GetActorLocation(), TotalRadius, 16, FColor::Red, false, 2.0f, 0, 2.0f);

	UFXSystemComponent* FXComp = Data->ImpactFX.SpawnAtLocation(this, GetActorLocation(), FRotator::ZeroRotator, FVector(ScaleMultiplier));
	if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(FXComp)) {
		NiagaraComp->SetVariableFloat(FName("Scale_All"), ScaleMultiplier);
	}
}

void ABoulder::ShowImpactOnPawn_Implementation(FVector ImpactPoint, FVector ImpactNormal) {
	if (!Data) return;

	const FRotator Rot = ImpactNormal.Rotation();
	Data->HitImpactFX.SpawnAtLocation(this, ImpactPoint, Rot);
}