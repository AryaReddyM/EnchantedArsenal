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

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnMergeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void HandleSpellMerge(AActor* OtherActor);

	UFUNCTION(BlueprintCallable, Category = "Magic")
	bool IsEnemy(AActor* OtherActor);
	
	UPROPERTY(VisibleAnywhere)
	USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere)
	USphereComponent* MergeCollision;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;
	UPROPERTY(VisibleAnywhere) 
	UProjectileMovementComponent* ProjComp;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	USpellData* Data = nullptr;
	UFUNCTION() 
	void OnRep_Data();

	UPROPERTY(Replicated) 
	ESpellType SpellType;
	
	UPROPERTY(ReplicatedUsing = OnRep_SpellTags) 
	FGameplayTagContainer SpellTags;
	UFUNCTION()
	void OnRep_SpellTags();
	
	UPROPERTY(Replicated)
	bool bIsHeld = false;
};