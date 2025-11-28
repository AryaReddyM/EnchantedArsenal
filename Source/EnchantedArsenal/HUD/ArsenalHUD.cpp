#include "ArsenalHUD.h"

void AArsenalHUD::DrawHUD() {
	Super::DrawHUD();

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	const FVector2D ViewportCenter = { ViewportSize.X * 0.5f, ViewportSize.Y * 0.5 };

	if (CrosshairsCenter) {
		DrawCrosshair(CrosshairsCenter, ViewportCenter);
	}
	if (CrosshairsRight) {
		DrawCrosshair(CrosshairsRight, ViewportCenter);
	}
	if (CrosshairsLeft) {
		DrawCrosshair(CrosshairsLeft, ViewportCenter);
	}
	if (CrosshairsUp) {
		DrawCrosshair(CrosshairsUp, ViewportCenter);
	}
	if (CrosshairsDown) {
		DrawCrosshair(CrosshairsDown, ViewportCenter);
	}
}

void AArsenalHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter) {
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint = { ViewportCenter.X - (TextureWidth / 2.0F), ViewportCenter.Y - (TextureHeight / 2.0F) };

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0F, 0.0F, 1.0F, 1.0F, FLinearColor::White, BLEND_Translucent, 0.75F); 
}