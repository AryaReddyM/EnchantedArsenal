#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SpellData.generated.h"

class UStaticMesh;
class UFXSystemAsset;
class UFXSystemComponent;
class USceneComponent;
class UMaterialInterface;
class ASpell;

USTRUCT(BlueprintType)
struct FSpellFX {
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UFXSystemAsset* System = nullptr;

	bool IsSet() const { return System != nullptr; }

	UFXSystemComponent* SpawnAtLocation(const UObject* WorldContext, FVector Location, FRotator Rotation = FRotator::ZeroRotator) const;

	UFXSystemComponent* SpawnAttached(USceneComponent* Parent, FName Socket = NAME_None) const;
};

USTRUCT(BlueprintType)
struct FTagModifier {
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag Tag;

	UPROPERTY(EditDefaultsOnly)
	float BonusDamage = 0.0f;
	
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* MaterialOverride = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	float BonusExplosionRadius = 0.0f;
};

UCLASS(BlueprintType)
class ENCHANTEDARSENAL_API USpellData : public UDataAsset {
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "General")
	FName SpellName;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UMaterialInterface* Material = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	FSpellFX TrailFX;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	FSpellFX ImpactFX;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	FSpellFX HitImpactFX;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Cooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float InitialSpeed = 1500.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Speed = 3000.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float GravityScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Damage = 25.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float ExplosionRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinDamageMultiplier = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TArray<FTagModifier> TagModifiers;

	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer DefaultTags;

	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer ComboGrantTags;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ASpell> Spell;

	float GetDamage(const FGameplayTagContainer& ActiveTags) const;
	float GetExplosionDamage(const FGameplayTagContainer& ActiveTags, float DistanceFromCenter) const;
	float GetExplosionRadius(const FGameplayTagContainer& ActiveTags) const;
	UMaterialInterface* GetMaterial(const FGameplayTagContainer& ActiveTags) const;
};