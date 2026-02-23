#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellVisual.generated.h"

class UStaticMeshComponent;
class USpellData;

UCLASS()
class ENCHANTEDARSENAL_API ASpellVisual : public AActor {
	GENERATED_BODY()
	
public:	
	ASpellVisual();

	void InitFromData();

	UPROPERTY(VisibleAnywhere) 
	USceneComponent* Root = nullptr;
	UPROPERTY(VisibleAnywhere) 
	UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(EditAnywhere)
	USpellData* Data;
};