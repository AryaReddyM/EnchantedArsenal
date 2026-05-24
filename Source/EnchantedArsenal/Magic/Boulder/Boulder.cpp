#include "Boulder.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "EnchantedArsenal/Magic/SpellData.h"

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

	if (IsEnemy(OtherActor)) {
		if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>()) {
			HealthComp->ApplyDamage(Data ? Data->GetDamage(SpellTags) : 0., GetOwner());
		}
		Destroy();
	}
}