#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArsenalHUD.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FHUDPackage {
	GENERATED_BODY()

public:
	UTexture2D* CrosshairsCenter;

	UTexture2D* CrosshairsRight;

	UTexture2D* CrosshairsLeft;

	UTexture2D* CrosshairsUp;

	UTexture2D* CrosshairsDown;
};

UCLASS()
class ENCHANTEDARSENAL_API AArsenalHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;

private:
	FHUDPackage HUDPackage;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& InHUDPackage) { HUDPackage = InHUDPackage; }
};
