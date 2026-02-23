#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spell.generated.h"

enum class ESpellType : uint8;

UCLASS()
class ENCHANTEDARSENAL_API ASpell : public AActor
{
	GENERATED_BODY()
	
public:
	ASpell();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USceneComponent* BaseRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell Properties", ReplicatedUsing = OnRep_SpellType)
	ESpellType SpellType;
	UFUNCTION()
	void OnRep_SpellType();
};