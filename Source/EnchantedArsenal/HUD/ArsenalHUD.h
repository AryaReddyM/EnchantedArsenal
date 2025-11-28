#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArsenalHUD.generated.h"

class UTexture2D;

UCLASS()
class ENCHANTEDARSENAL_API AArsenalHUD : public AHUD {
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter);

protected:
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsUp;

	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairsDown;
};
