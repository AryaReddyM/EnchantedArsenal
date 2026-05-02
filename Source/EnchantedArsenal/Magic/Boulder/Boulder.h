#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "Boulder.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API ABoulder : public ASpell {
	GENERATED_BODY()

public:
	ABoulder();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;
	
	// UPROPERTY(VisibleAnywhere) USphereComponent* Collision;
};