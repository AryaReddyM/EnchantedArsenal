#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Magic/Modifier/ModifierSpell.h"
#include "Spiker.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API ASpiker : public AModifierSpell {
	GENERATED_BODY()

public:
	ASpiker();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};