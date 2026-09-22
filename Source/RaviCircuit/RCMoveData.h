#pragma once

#include "CoreMinimal.h"
#include <initializer_list>
#include "RCMoveData.generated.h"

UENUM(BlueprintType)
enum class ERCMoveLevel : uint8
{
	High,
	Mid,
	Low,
	Throw
};

UENUM(BlueprintType)
enum class ERCFighterState : uint8
{
	Idle,
	Attack,
	HitStun,
	BlockStun,
	Knockdown,
	KO
};

USTRUCT(BlueprintType)
struct FRCMoveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Startup = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Active = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Recovery = 18;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Damage = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Chip = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HitAdvantage = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BlockAdvantage = -4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LaunchVelocity = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Range = 210.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Pushback = 115.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Tracking = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Shake = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MeterGain = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CounterDamageMultiplier = 1.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ERCMoveLevel Level = ERCMoveLevel::Mid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bLauncher = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bKnockdown = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSuper = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> CancelInto;

	int32 TotalFrames() const { return Startup + Active + Recovery; }
};

USTRUCT(BlueprintType)
struct FRCFighterSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Style;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Primary = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Accent = FLinearColor::Yellow;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Dark = FLinearColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WalkSpeed = 560.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SideSpeed = 430.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float JumpSpeed = 640.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BodyScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, FRCMoveData> Moves;
};

struct FRCMoveLibrary
{
	static TMap<FName, FRCFighterSpec> BuildFighters()
	{
		TMap<FName, FRCFighterSpec> Out;
		Out.Add("Zara", BuildZara());
		Out.Add("Hamza", BuildHamza());
		return Out;
	}

	static FRCMoveData Move(FName Id, const TCHAR* Name, int32 Startup, int32 Active, int32 Recovery, ERCMoveLevel Level, int32 Damage, int32 BlockAdv, int32 HitAdv, float Launch, float Range, float Push, float Shake, std::initializer_list<FName> Cancels = {}, bool bLauncher = false, bool bKnockdown = false)
	{
		FRCMoveData M;
		M.Id = Id;
		M.DisplayName = FText::FromString(Name);
		M.Startup = Startup;
		M.Active = Active;
		M.Recovery = Recovery;
		M.Level = Level;
		M.Damage = Damage;
		M.Chip = FMath::Max(1, FMath::RoundToInt(Damage * 0.12f));
		M.BlockAdvantage = BlockAdv;
		M.HitAdvantage = HitAdv;
		M.LaunchVelocity = Launch;
		M.Range = Range;
		M.Pushback = Push;
		M.Shake = Shake;
		for (const FName& Cancel : Cancels)
		{
			M.CancelInto.Add(Cancel);
		}
		M.bLauncher = bLauncher;
		M.bKnockdown = bKnockdown;
		return M;
	}

	static FRCMoveData Super(FName Id, const TCHAR* Name, int32 Startup, int32 Active, int32 Recovery, int32 Damage, float Range)
	{
		FRCMoveData M = Move(Id, Name, Startup, Active, Recovery, ERCMoveLevel::Mid, Damage, -18, 32, 850.f, Range, 235.f, 0.9f, {}, false, true);
		M.bSuper = true;
		M.MeterGain = 0.f;
		return M;
	}

	static FRCFighterSpec BuildZara()
	{
		FRCFighterSpec S;
		S.Id = "Zara";
		S.DisplayName = FText::FromString("Zara Vey");
		S.Style = FText::FromString("Magnetic kickboxing");
		S.Primary = FLinearColor(0.05f, 0.78f, 0.96f);
		S.Accent = FLinearColor(1.0f, 0.77f, 0.12f);
		S.Dark = FLinearColor(0.025f, 0.025f, 0.05f);
		S.WalkSpeed = 585.f;
		S.SideSpeed = 470.f;
		S.JumpSpeed = 660.f;
		S.BodyScale = 0.94f;
		S.Moves.Add("Punch", Move("Punch", TEXT("Volt Jab"), 5, 3, 13, ERCMoveLevel::High, 8, 2, 7, 0.f, 218.f, 85.f, 0.08f, {FName("Punch2"), FName("Special")}));
		S.Moves.Add("Punch2", Move("Punch2", TEXT("Relay Cross"), 7, 3, 16, ERCMoveLevel::High, 10, 1, 8, 0.f, 224.f, 95.f, 0.12f, {FName("Launcher")}));
		S.Moves.Add("Kick", Move("Kick", TEXT("Neon Roundhouse"), 11, 4, 22, ERCMoveLevel::Mid, 15, -5, 12, 0.f, 265.f, 125.f, 0.18f, {FName("Special")}));
		S.Moves.Add("Low", Move("Low", TEXT("Circuit Sweep"), 15, 4, 25, ERCMoveLevel::Low, 13, -9, 6, 0.f, 228.f, 100.f, 0.2f, {}, false, true));
		S.Moves.Add("Launcher", Move("Launcher", TEXT("Skyline Upper"), 16, 3, 28, ERCMoveLevel::Mid, 12, -13, 18, 760.f, 205.f, 122.f, 0.22f, {FName("Kick")}, true));
		S.Moves.Add("Special", Move("Special", TEXT("Polarity Kick"), 13, 6, 30, ERCMoveLevel::Mid, 20, -8, 15, 220.f, 305.f, 155.f, 0.35f));
		S.Moves.Add("Throw", Move("Throw", TEXT("Current Toss"), 8, 2, 36, ERCMoveLevel::Throw, 22, 0, 26, 520.f, 150.f, 130.f, 0.45f));
		S.Moves.Add("Super", Super("Super", TEXT("Monsoon Breaker"), 12, 9, 66, 38, 320.f));
		return S;
	}

	static FRCFighterSpec BuildHamza()
	{
		FRCFighterSpec S;
		S.Id = "Hamza";
		S.DisplayName = FText::FromString("Hamza Kade");
		S.Style = FText::FromString("Heavy pressure grappling");
		S.Primary = FLinearColor(0.95f, 0.22f, 0.14f);
		S.Accent = FLinearColor(0.28f, 0.95f, 0.48f);
		S.Dark = FLinearColor(0.055f, 0.03f, 0.02f);
		S.WalkSpeed = 505.f;
		S.SideSpeed = 390.f;
		S.JumpSpeed = 595.f;
		S.BodyScale = 1.13f;
		S.Moves.Add("Punch", Move("Punch", TEXT("Stone Jab"), 6, 3, 14, ERCMoveLevel::High, 9, 1, 8, 0.f, 210.f, 95.f, 0.12f, {FName("Punch2")}));
		S.Moves.Add("Punch2", Move("Punch2", TEXT("Bazaar Hook"), 10, 4, 19, ERCMoveLevel::Mid, 13, -3, 12, 0.f, 230.f, 118.f, 0.18f, {FName("Launcher")}));
		S.Moves.Add("Kick", Move("Kick", TEXT("Iron Shin"), 14, 5, 25, ERCMoveLevel::Mid, 17, -6, 13, 150.f, 255.f, 135.f, 0.22f, {FName("Special")}));
		S.Moves.Add("Low", Move("Low", TEXT("Foundation Low"), 17, 4, 27, ERCMoveLevel::Low, 15, -11, 8, 0.f, 220.f, 115.f, 0.24f, {}, false, true));
		S.Moves.Add("Launcher", Move("Launcher", TEXT("Forge Lift"), 18, 4, 31, ERCMoveLevel::Mid, 14, -14, 17, 830.f, 208.f, 120.f, 0.28f, {FName("Kick")}, true));
		S.Moves.Add("Special", Move("Special", TEXT("Truck Art Ram"), 19, 6, 34, ERCMoveLevel::Mid, 24, -10, 18, 250.f, 288.f, 180.f, 0.42f));
		S.Moves.Add("Throw", Move("Throw", TEXT("Gatekeeper Slam"), 9, 2, 40, ERCMoveLevel::Throw, 27, 0, 32, 620.f, 155.f, 145.f, 0.55f));
		S.Moves.Add("Super", Super("Super", TEXT("Lahore Lockdown"), 16, 9, 72, 45, 280.f));
		return S;
	}
};
