#include "Spiker.h"
#include "EnchantedArsenal/Magic/SpellData.h"
#include "EnchantedArsenal/Magic/Base/BaseSpell.h"

ASpiker::ASpiker() {
	PrimaryActorTick.bCanEverTick = true;
}

void ASpiker::BeginPlay() {
	Super::BeginPlay();
}

void ASpiker::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void ASpiker::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

	if (Cast<ABaseSpell>(OtherActor)) return;

	Destroy();
}
