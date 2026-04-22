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

void ASpiker::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (!OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;

	ASpell* OtherSpell = Cast<ASpell>(OtherActor);
	if (!OtherSpell) return;

	if (HasAuthority() && Data) {
		OtherSpell->SpellTags.AppendTags(Data->ComboGrantTags);
		Destroy();
	}
}