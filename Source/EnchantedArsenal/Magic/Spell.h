#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spell.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
enum class ESpellType : uint8;
class USpellData;

UCLASS()
class ENCHANTEDARSENAL_API ASpell : public AActor {
	GENERATED_BODY()
	
public:
	ASpell();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	void InitFromData();
	void LaunchInDirection(const FVector& Dir);
	
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	void CollisionIgnoreOwner();

	UPROPERTY(VisibleAnywhere) USphereComponent* Collision = nullptr;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* ProjComp = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell Properties", ReplicatedUsing = OnRep_SpellType)
	ESpellType SpellType;
	UFUNCTION()
	void OnRep_SpellType();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Data)
	USpellData* Data = nullptr;
	UFUNCTION()
	void OnRep_Data();
};