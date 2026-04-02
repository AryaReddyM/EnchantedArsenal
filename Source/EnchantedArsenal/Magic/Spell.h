#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Spell.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USpellData;
enum class ESpellType : uint8;

UCLASS()
class ENCHANTEDARSENAL_API ASpell : public AActor {
	GENERATED_BODY()

public:
	ASpell();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitFromData();
	void LaunchInDirection(const FVector& Dir);
	void SetHeldMode(bool bHeld);
	void CollisionIgnoreOwner();

protected:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
	UPROPERTY(VisibleAnywhere) USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* MeshComp;
	UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* ProjComp;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	USpellData* Data = nullptr;
	UFUNCTION() void OnRep_Data();

	UPROPERTY(Replicated) 
	ESpellType SpellType;
	UPROPERTY(Replicated) 
	FGameplayTagContainer SpellTags;
	UPROPERTY(Replicated) 
	bool bIsHeld = false;
};
