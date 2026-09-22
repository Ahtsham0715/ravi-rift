#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RCHUD.generated.h"

class ARaviCircuitGameMode;
class UTexture2D;

UCLASS()
class RAVICIRCUIT_API ARCHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	UPROPERTY() TObjectPtr<UTexture2D> StageConcept;
	UPROPERTY() TObjectPtr<UTexture2D> ZaraConcept;
	UPROPERTY() TObjectPtr<UTexture2D> HamzaConcept;

	void DrawBar(float X, float Y, float W, float H, float Fraction, FLinearColor Fill, bool bReverse);
	void DrawShadowText(const FString& Text, float X, float Y, float Scale, FLinearColor Color);
	void DrawTexturePlate(UTexture2D* Texture, float X, float Y, float W, float H, FLinearColor Tint);
};
