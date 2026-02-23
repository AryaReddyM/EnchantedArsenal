#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpellData.generated.h"

class UStaticMesh;
class UNiagaraSystem;
class UMaterialInterface;
class ASpellVisual;
class ASpell;

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
	UNiagaraSystem* TrailFX = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals") 
	UNiagaraSystem* ImpactFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Stats") 
	float Cooldown = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats") 
	float Speed = 3000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats") 
	float Damage = 25.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ASpell> Spell;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ASpellVisual> SpellVisual;
};