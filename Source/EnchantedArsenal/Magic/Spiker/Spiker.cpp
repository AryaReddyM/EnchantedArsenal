#include "Spiker.h"
#include "EnchantedArsenal/Magic/SpellData.h"

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
	
	Destroy();
}
