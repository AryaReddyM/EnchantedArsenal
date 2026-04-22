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
	
	if (bIsHeld || !OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;
	
	if (HasAuthority()) {
		if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>()) {
			HealthComp->ApplyDamage(Data ? Data->GetDamage(SpellTags) : 0.0f);

			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Health: ") + FString::SanitizeFloat(HealthComp->CurrentHealth));
		}
		Destroy();
	}
}

void ABoulder::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	Super::OnOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	
	if (!OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;
	
	ASpell* OtherSpell = Cast<ASpell>(OtherActor);
	if (!OtherSpell) {
		return;
	}
	
	if (HasAuthority() && OtherSpell->Data) {
		SpellTags.AppendTags(OtherSpell->Data->ComboGrantTags);
		OtherSpell->Destroy();
	}
}