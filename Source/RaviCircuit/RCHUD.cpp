#include "RCHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RaviCircuitGameMode.h"
#include "RCFighterCharacter.h"

void ARCHUD::BeginPlay()
{
	Super::BeginPlay();
	StageConcept = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/Concepts/noorabad_rooftop_key_art.noorabad_rooftop_key_art"), TEXT("Concepts/noorabad_rooftop_key_art.png"));
	ZaraConcept = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/Concepts/zara_vey_concept.zara_vey_concept"), TEXT("Concepts/zara_vey_concept.png"));
	HamzaConcept = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/Concepts/hamza_kade_concept.hamza_kade_concept"), TEXT("Concepts/hamza_kade_concept.png"));
	VersusSplash = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/Concepts/versus_splash.versus_splash"), TEXT("Concepts/versus_splash.png"));
	LogoPlate = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/UI/Plates/ravi_circuit_logo.ravi_circuit_logo"), TEXT("UI/Plates/ravi_circuit_logo.png"));
	ZaraPortrait = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/UI/Portraits/zara_vey_portrait.zara_vey_portrait"), TEXT("UI/Portraits/zara_vey_portrait.png"));
	HamzaPortrait = LoadImportedOrSourcePng(TEXT("/Game/RaviCircuit/UI/Portraits/hamza_kade_portrait.hamza_kade_portrait"), TEXT("UI/Portraits/hamza_kade_portrait.png"));
}

void ARCHUD::DrawHUD()
{
	Super::DrawHUD();
	ARaviCircuitGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARaviCircuitGameMode>() : nullptr;
	if (!GM || !Canvas)
	{
		return;
	}
	const float W = Canvas->SizeX;
	if (GM->bInFrontEnd)
	{
		DrawTexturePlate(StageConcept, 0.f, 0.f, W, Canvas->SizeY, FLinearColor(1.f, 1.f, 1.f, 0.82f));
		DrawTexturePlate(VersusSplash, W * 0.55f, Canvas->SizeY * 0.08f, W * 0.40f, Canvas->SizeY * 0.72f, FLinearColor(1.f, 1.f, 1.f, 0.72f));
		DrawTexturePlate(ZaraConcept, W * 0.63f, Canvas->SizeY * 0.14f, W * 0.15f, Canvas->SizeY * 0.58f, FLinearColor(1.f, 1.f, 1.f, 0.84f));
		DrawTexturePlate(HamzaConcept, W * 0.80f, Canvas->SizeY * 0.14f, W * 0.15f, Canvas->SizeY * 0.58f, FLinearColor(1.f, 1.f, 1.f, 0.84f));
		DrawTexturePlate(LogoPlate, W * 0.065f, Canvas->SizeY * 0.08f, W * 0.31f, Canvas->SizeY * 0.17f, FLinearColor(1.f, 1.f, 1.f, 0.95f));
		DrawShadowText(TEXT("RAVI RIFT"), W * 0.08f, Canvas->SizeY * 0.15f, 2.25f, FLinearColor(1.f, 0.78f, 0.12f));
		DrawShadowText(TEXT("Original 3D fighting vertical slice"), W * 0.085f, Canvas->SizeY * 0.27f, 0.9f, FLinearColor(0.7f, 0.88f, 0.98f));
		DrawShadowText(GM->FrontEndLine(), W * 0.085f, Canvas->SizeY * 0.39f, 0.95f, FLinearColor::White);
		DrawShadowText(TEXT("Keyboard and gamepad combat are live once a mode starts."), W * 0.085f, Canvas->SizeY * 0.72f, 0.72f, FLinearColor(0.72f, 0.86f, 0.95f));
		return;
	}
	if (GM->bAwaitingRematch)
	{
		DrawTexturePlate(StageConcept, 0.f, 0.f, W, Canvas->SizeY, FLinearColor(1.f, 1.f, 1.f, 0.70f));
		DrawShadowText(TEXT("MATCH COMPLETE"), W * 0.32f, Canvas->SizeY * 0.29f, 1.85f, FLinearColor(1.f, 0.78f, 0.12f));
		DrawShadowText(GM->MatchResultLine(), W * 0.32f, Canvas->SizeY * 0.43f, 1.05f, FLinearColor::White);
		return;
	}
	if (!GM->P1 || !GM->P2)
	{
		return;
	}
	DrawTexturePlate(ZaraPortrait, 40.f, 88.f, 118.f, 118.f, FLinearColor(1.f, 1.f, 1.f, 0.82f));
	DrawTexturePlate(HamzaPortrait, W - 158.f, 88.f, 118.f, 118.f, FLinearColor(1.f, 1.f, 1.f, 0.82f));
	DrawBar(40.f, 28.f, W * 0.38f, 32.f, static_cast<float>(GM->P1->Health) / GM->P1->MaxHealth, FLinearColor(0.08f, 0.9f, 0.78f), false);
	DrawBar(W - 40.f - W * 0.38f, 28.f, W * 0.38f, 32.f, static_cast<float>(GM->P2->Health) / GM->P2->MaxHealth, FLinearColor(0.95f, 0.2f, 0.12f), true);
	DrawBar(40.f, 70.f, W * 0.24f, 13.f, GM->P1->Meter / 100.f, FLinearColor(0.1f, 0.62f, 1.f), false);
	DrawBar(W - 40.f - W * 0.24f, 70.f, W * 0.24f, 13.f, GM->P2->Meter / 100.f, FLinearColor(1.f, 0.78f, 0.1f), true);
	DrawShadowText(GM->P1->Spec.DisplayName.ToString(), 170.f, 98.f, 1.05f, FLinearColor::White);
	DrawShadowText(GM->P2->Spec.DisplayName.ToString(), W - 360.f, 98.f, 1.05f, FLinearColor::White);
	DrawShadowText(GM->MatchMode == ERCMatchMode::Training ? TEXT("∞") : FString::FromInt(FMath::Max(0, FMath::CeilToInt(GM->RoundTime))), W * 0.49f, 22.f, 1.8f, FLinearColor::White);
	DrawShadowText(FString::Printf(TEXT("ROUND %d    %d-%d"), GM->RoundNumber, GM->P1Rounds, GM->P2Rounds), W * 0.43f, 78.f, 0.9f, FLinearColor(1.f, 0.78f, 0.2f));
	const FString Line = GM->HudLine();
	if (!Line.IsEmpty())
	{
		DrawShadowText(Line, W * 0.27f, 132.f, 1.05f, FLinearColor(0.2f, 0.9f, 1.f));
	}
	DrawShadowText(TEXT("P1 WASD/QE J K L U I O | P2 arrows , . numpad 1-6 | V VFX | C shake | Esc pause"), 38.f, Canvas->SizeY - 48.f, 0.72f, FLinearColor(0.72f, 0.86f, 0.95f));
	const FString Training = GM->TrainingFrameLine();
	if (!Training.IsEmpty())
	{
		DrawShadowText(Training, 38.f, 170.f, 0.52f, FLinearColor(0.84f, 0.9f, 0.98f));
	}
	if (GM->bPausedMatch)
	{
		DrawShadowText(TEXT("PAUSED"), W * 0.44f, Canvas->SizeY * 0.42f, 2.0f, FLinearColor(1.f, 0.78f, 0.15f));
	}
}

UTexture2D* ARCHUD::LoadImportedOrSourcePng(const TCHAR* ImportedPath, const TCHAR* SourceArtRelativePath)
{
	if (UTexture2D* Imported = LoadObject<UTexture2D>(nullptr, ImportedPath))
	{
		return Imported;
	}
	const FString SourcePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt"), SourceArtRelativePath);
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *SourcePath))
	{
		return nullptr;
	}
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
	{
		return nullptr;
	}
	TArray<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
	{
		return nullptr;
	}
	UTexture2D* Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	Texture->AddToRoot();
	return Texture;
}

void ARCHUD::DrawBar(float X, float Y, float W, float H, float Fraction, FLinearColor Fill, bool bReverse)
{
	Fraction = FMath::Clamp(Fraction, 0.f, 1.f);
	FCanvasTileItem Back(FVector2D(X, Y), FVector2D(W, H), FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
	Back.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Back);
	const float FW = W * Fraction;
	const float FX = bReverse ? X + W - FW : X;
	FCanvasTileItem Front(FVector2D(FX, Y), FVector2D(FW, H), Fill);
	Front.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Front);
}

void ARCHUD::DrawShadowText(const FString& Text, float X, float Y, float Scale, FLinearColor Color)
{
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	if (Lines.Num() > 1)
	{
		const float LineHeight = 34.f * Scale;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			DrawShadowText(Lines[Index], X, Y + LineHeight * Index, Scale, Color);
		}
		return;
	}
	FCanvasTextItem Shadow(FVector2D(X + 2.f, Y + 2.f), FText::FromString(Text), GEngine->GetLargeFont(), FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Shadow.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Shadow);
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), GEngine->GetLargeFont(), Color);
	Item.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Item);
}

void ARCHUD::DrawTexturePlate(UTexture2D* Texture, float X, float Y, float W, float H, FLinearColor Tint)
{
	if (!Texture)
	{
		return;
	}
	FCanvasTileItem Tile(FVector2D(X, Y), Texture->GetResource(), FVector2D(W, H), Tint);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}
