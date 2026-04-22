#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "Spiker.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API ASpiker : public ASpell {
	GENERATED_BODY()

public:
	ASpiker();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};