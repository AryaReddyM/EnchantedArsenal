#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpellData.generated.h"

class UStaticMesh;
class UNiagaraSystem;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ESpellType : uint8
{
	EST_Boulder,
	EST_SpikeAdder,
	EST_MAX
};

UCLASS(BlueprintType)
class ENCHANTEDARSENAL_API USpellData : public UDataAsset {
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) ESpellType SpellType = ESpellType::EST_Boulder;

	UPROPERTY(EditDefaultsOnly) UStaticMesh* Mesh = nullptr;
	UPROPERTY(EditDefaultsOnly) UMaterialInterface* Material = nullptr;
	UPROPERTY(EditDefaultsOnly) UNiagaraSystem* TrailFX = nullptr;
	UPROPERTY(EditDefaultsOnly) UNiagaraSystem* ImpactFX = nullptr;

	UPROPERTY(EditDefaultsOnly) float Cooldown = 1.0f;
	UPROPERTY(EditDefaultsOnly) float Speed = 3000.0f;
	UPROPERTY(EditDefaultsOnly) float Damage = 25.0f;
};