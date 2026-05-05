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
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
};