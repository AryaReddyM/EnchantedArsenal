#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsData.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API UPhysicsData : public UDataAsset {
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	FPhysicalAnimationData RagdollPhysicalAnimData;
};
