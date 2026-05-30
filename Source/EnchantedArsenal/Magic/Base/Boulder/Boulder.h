#pragma once

#include "CoreMinimal.h"
#include "EnchantedArsenal/Magic/Base/BaseSpell.h"
#include "Boulder.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API ABoulder : public ABaseSpell {
	GENERATED_BODY()

public:
	ABoulder();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	
	void Explode(AActor* DirectHitActor, const FVector& Origin);
	UFUNCTION(NetMulticast, Reliable)
	void ShowBlast();
	UFUNCTION(NetMulticast, Unreliable)
	void ShowImpactOnPawn(FVector ImpactPoint, FVector ImpactNormal);
};