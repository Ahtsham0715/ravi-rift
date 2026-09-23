#include "RaviCircuitGameMode.h"

#include "RCFightCameraActor.h"
#include "RCFighterCharacter.h"
#include "RCHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"

ARaviCircuitGameMode::ARaviCircuitGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	HUDClass = ARCHUD::StaticClass();
}

void ARaviCircuitGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const FString RaviMode = UGameplayStatics::ParseOption(Options, TEXT("RaviMode"));
	if (RaviMode == TEXT("OnlineHost"))
	{
		StartupMode = ERCMatchMode::OnlineHost;
		bStartMatchOnBeginPlay = true;
	}
}

void ARaviCircuitGameMode::BeginPlay()
{
	Super::BeginPlay();
	FighterSpecs = FRCMoveLibrary::BuildFighters();
	RuntimeHud = Cast<ARCHUD>(UGameplayStatics::GetPlayerController(this, 0)->GetHUD());
	LoadAudioAssets();
	BuildArena();
	if (bStartMatchOnBeginPlay)
	{
		StartMatch(StartupMode);
	}
	else
	{
		ReturnToFrontEnd();
	}
}

void ARaviCircuitGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (AnnouncementTimer > 0.f)
	{
		AnnouncementTimer -= DeltaSeconds;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	HandleSettingsInput(PC);
	if (bInFrontEnd)
	{
		HandleFrontEndInput(PC);
		return;
	}
	if (bAwaitingRematch)
	{
		HandlePostMatchInput(PC);
		return;
	}
	if (PC && PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		bPausedMatch = !bPausedMatch;
	}
	if (P2 && (MatchMode == ERCMatchMode::LocalVersus || MatchMode == ERCMatchMode::Training))
	{
		P2->ApplyP2KeyboardInput(PC, DeltaSeconds);
	}
	if (MatchMode == ERCMatchMode::Training)
	{
		HandleTrainingInput(PC);
	}
	if (!bMatchActive || bPausedMatch)
	{
		return;
	}
	RoundTime -= DeltaSeconds;
	if (RoundTime <= 0.f)
	{
		EndRound((P1 && P2 && P1->Health >= P2->Health) ? P1 : P2, TEXT("TIME UP"));
	}
	if (P1 && P1->Health <= 0)
	{
		EndRound(P2, TEXT("K.O."));
	}
	else if (P2 && P2->Health <= 0)
	{
		EndRound(P1, TEXT("K.O."));
	}
}

void ARaviCircuitGameMode::StartMatch(ERCMatchMode NewMode)
{
	MatchMode = NewMode;
	bInFrontEnd = false;
	bAwaitingRematch = false;
	MatchResult.Reset();
	P1Rounds = 0;
	P2Rounds = 0;
	RoundNumber = 1;
	SpawnFighters();
	BeginRound();
}

void ARaviCircuitGameMode::HostOnlineMatch()
{
	PlayMenuConfirm();
	MatchMode = ERCMatchMode::OnlineHost;
	Announcement = TEXT("HOSTING ONLINE VERSUS");
	AnnouncementTimer = 1.5f;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ConsoleCommand(TEXT("open /Engine/Maps/Entry?RaviMode=OnlineHost?listen"));
	}
}

void ARaviCircuitGameMode::JoinOnlineMatch(const FString& Address)
{
	PlayMenuConfirm();
	MatchMode = ERCMatchMode::OnlineClient;
	Announcement = TEXT("JOINING ONLINE VERSUS");
	AnnouncementTimer = 1.5f;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ConsoleCommand(FString::Printf(TEXT("open %s"), *Address));
	}
}

void ARaviCircuitGameMode::SpawnFighters()
{
	if (P1) { P1->Destroy(); }
	if (P2) { P2->Destroy(); }
	P1 = GetWorld()->SpawnActor<ARCFighterCharacter>(ARCFighterCharacter::StaticClass(), FVector(-235.f, 0.f, 112.f), FRotator::ZeroRotator);
	P2 = GetWorld()->SpawnActor<ARCFighterCharacter>(ARCFighterCharacter::StaticClass(), FVector(235.f, 0.f, 112.f), FRotator::ZeroRotator);
	P1->Configure(FighterSpecs.FindChecked(P1Id), 0, false, this);
	P2->Configure(FighterSpecs.FindChecked(P2Id), 1, MatchMode == ERCMatchMode::ArcadeCPU, this);
	P1->Opponent = P2;
	P2->Opponent = P1;
	P1->OnLandedHit.AddUObject(this, &ARaviCircuitGameMode::HandleHit);
	P2->OnLandedHit.AddUObject(this, &ARaviCircuitGameMode::HandleHit);
	P1->OnStartedSuper.AddUObject(this, &ARaviCircuitGameMode::HandleSuper);
	P2->OnStartedSuper.AddUObject(this, &ARaviCircuitGameMode::HandleSuper);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->Possess(P1);
	}
	if (!FightCamera)
	{
		FightCamera = GetWorld()->SpawnActor<ARCFightCameraActor>(ARCFightCameraActor::StaticClass(), FVector(0.f, 720.f, 320.f), FRotator(-12.f, 180.f, 0.f));
	}
	FightCamera->Configure(P1, P2);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTarget(FightCamera);
	}
}

void ARaviCircuitGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer || MatchMode != ERCMatchMode::OnlineHost || !P1 || !P2)
	{
		return;
	}
	if (!P1->GetController())
	{
		NewPlayer->Possess(P1);
	}
	else if (P1->GetController() != NewPlayer && !P2->GetController())
	{
		NewPlayer->Possess(P2);
		Announcement = TEXT("ONLINE CHALLENGER CONNECTED");
		AnnouncementTimer = 1.5f;
	}
	if (FightCamera)
	{
		NewPlayer->SetViewTarget(FightCamera);
	}
}

void ARaviCircuitGameMode::BeginRound()
{
	RoundTime = MatchMode == ERCMatchMode::Training ? 999.f : 60.f;
	bMatchActive = true;
	bPausedMatch = false;
	P1->ResetForRound(FVector(-235.f, 0.f, 112.f));
	P2->ResetForRound(FVector(235.f, 0.f, 112.f));
	P2->SetForcedGuard(bTrainingDummyGuard, bTrainingDummyLowGuard);
	Announcement = MatchMode == ERCMatchMode::Training ? TEXT("TRAINING") : FString::Printf(TEXT("ROUND %d"), RoundNumber);
	AnnouncementTimer = 1.2f;
	if (RoundStartSound)
	{
		UGameplayStatics::PlaySound2D(this, RoundStartSound, 0.55f);
	}
	FTimerHandle FightCueTimer;
	GetWorldTimerManager().SetTimer(FightCueTimer, [this]()
	{
		if (AnnouncerFightSound)
		{
			UGameplayStatics::PlaySound2D(this, AnnouncerFightSound, 0.62f);
		}
	}, 0.62f, false);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayHaptics(PC, 0.08f, 0.18f, 0.18f);
	}
}

void ARaviCircuitGameMode::EndRound(ARCFighterCharacter* Winner, const FString& Reason)
{
	if (!bMatchActive || !Winner)
	{
		return;
	}
	bMatchActive = false;
	if (Winner == P1) { ++P1Rounds; } else { ++P2Rounds; }
	Announcement = FString::Printf(TEXT("%s - %s WINS"), *Reason, *Winner->Spec.DisplayName.ToString());
	AnnouncementTimer = 2.4f;
	if (KoHitSound && Reason.Contains(TEXT("K.O.")))
	{
		UGameplayStatics::PlaySound2D(this, KoHitSound, 0.78f);
	}
	if (Reason.Contains(TEXT("K.O.")) && AnnouncerKoSound)
	{
		UGameplayStatics::PlaySound2D(this, AnnouncerKoSound, 0.72f);
	}
	if (Winner && Winner->Health == Winner->MaxHealth && AnnouncerPerfectSound)
	{
		UGameplayStatics::PlaySound2D(this, AnnouncerPerfectSound, 0.72f);
	}
	FTimerHandle Timer;
	if (MatchMode == ERCMatchMode::Training || (P1Rounds < 2 && P2Rounds < 2))
	{
		if (MatchMode != ERCMatchMode::Training)
		{
			++RoundNumber;
		}
		GetWorldTimerManager().SetTimer(Timer, this, &ARaviCircuitGameMode::BeginRound, 2.2f, false);
	}
	else
	{
		MatchResult = FString::Printf(TEXT("%s wins the set %d-%d"), *Winner->Spec.DisplayName.ToString(), P1Rounds, P2Rounds);
		bAwaitingRematch = true;
	}
}

void ARaviCircuitGameMode::ReturnToFrontEnd()
{
	bInFrontEnd = true;
	bAwaitingRematch = false;
	bMatchActive = false;
	bPausedMatch = false;
	MatchResult.Reset();
	Announcement = TEXT("RAVI RIFT");
	AnnouncementTimer = 999999.f;
	if (!FightCamera)
	{
		FightCamera = GetWorld()->SpawnActor<ARCFightCameraActor>(ARCFightCameraActor::StaticClass(), FVector(-260.f, 980.f, 360.f), FRotator(-14.f, 166.f, 0.f));
	}
	FightCamera->Configure(nullptr, nullptr);
	FightCamera->SetActorLocation(FVector(-260.f, 980.f, 360.f));
	FightCamera->SetActorRotation(FRotator(-14.f, 166.f, 0.f));
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTarget(FightCamera);
	}
	if (P1)
	{
		P1->Destroy();
		P1 = nullptr;
	}
	if (P2)
	{
		P2->Destroy();
		P2 = nullptr;
	}
}

void ARaviCircuitGameMode::CycleFighter(bool bFirstPlayer)
{
	FName& Id = bFirstPlayer ? P1Id : P2Id;
	Id = Id == "Zara" ? FName("Hamza") : FName("Zara");
}

void ARaviCircuitGameMode::HandleFrontEndInput(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::One))
	{
		PlayMenuConfirm();
		StartMatch(ERCMatchMode::ArcadeCPU);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Two))
	{
		PlayMenuConfirm();
		StartMatch(ERCMatchMode::LocalVersus);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Three))
	{
		PlayMenuConfirm();
		StartMatch(ERCMatchMode::Training);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Four))
	{
		HostOnlineMatch();
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Five))
	{
		JoinOnlineMatch();
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Q))
	{
		PlayMenuTick();
		CycleFighter(true);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		PlayMenuTick();
		CycleFighter(false);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
}

void ARaviCircuitGameMode::HandlePostMatchInput(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::R))
	{
		PlayMenuConfirm();
		StartMatch(MatchMode);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::M) || PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		PlayMenuBack();
		ReturnToFrontEnd();
	}
}

void ARaviCircuitGameMode::HandleTrainingInput(APlayerController* PC)
{
	if (!PC || !P1 || !P2)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::F))
	{
		P1->ResetForRound(FVector(-235.f, 0.f, 112.f));
		P2->ResetForRound(FVector(235.f, 0.f, 112.f));
		P2->SetForcedGuard(bTrainingDummyGuard, bTrainingDummyLowGuard);
		Announcement = TEXT("RESET");
		AnnouncementTimer = 0.55f;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::G))
	{
		bTrainingDummyGuard = !bTrainingDummyGuard;
		P2->SetForcedGuard(bTrainingDummyGuard, bTrainingDummyLowGuard);
		Announcement = bTrainingDummyGuard ? TEXT("DUMMY GUARD ON") : TEXT("DUMMY GUARD OFF");
		AnnouncementTimer = 0.75f;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::H))
	{
		bTrainingDummyLowGuard = !bTrainingDummyLowGuard;
		P2->SetForcedGuard(bTrainingDummyGuard, bTrainingDummyLowGuard);
		Announcement = bTrainingDummyLowGuard ? TEXT("DUMMY LOW GUARD") : TEXT("DUMMY STAND GUARD");
		AnnouncementTimer = 0.75f;
	}
}

void ARaviCircuitGameMode::HandleSettingsInput(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::V))
	{
		bHitVfxEnabled = !bHitVfxEnabled;
		PlayMenuTick();
		Announcement = bHitVfxEnabled ? TEXT("HIT VFX ON") : TEXT("HIT VFX OFF");
		AnnouncementTimer = 0.75f;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::C))
	{
		bCameraShakeEnabled = !bCameraShakeEnabled;
		PlayMenuTick();
		Announcement = bCameraShakeEnabled ? TEXT("CAMERA SHAKE ON") : TEXT("CAMERA SHAKE OFF");
		AnnouncementTimer = 0.75f;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::B))
	{
		bHapticsEnabled = !bHapticsEnabled;
		PlayMenuTick();
		Announcement = bHapticsEnabled ? TEXT("HAPTICS ON") : TEXT("HAPTICS OFF");
		AnnouncementTimer = 0.75f;
	}
}

void ARaviCircuitGameMode::HandleHit(ARCFighterCharacter* Attacker, ARCFighterCharacter* Defender, const FRCMoveData& Move, bool bBlocked)
{
	SetCameraShake(Move.Shake);
	if (bHitVfxEnabled)
	{
		SpawnHitFX(Defender->GetActorLocation() + FVector(0.f, 0.f, 118.f), bBlocked, Move.bSuper);
	}
	if (!bBlocked && (Move.bSuper || Move.Damage >= 20))
	{
		TriggerReactiveProps(Defender->GetActorLocation(), 260.f);
	}
	PlayCombatSound(Move, bBlocked, Defender->GetActorLocation());
	Announcement = bBlocked ? TEXT("BLOCKED") : Move.DisplayName.ToString();
	AnnouncementTimer = 0.7f;
	if (Defender->ComboCount > 1)
	{
		Announcement = FString::Printf(TEXT("%d HIT  %d DAMAGE"), Defender->ComboCount, Defender->ComboDamage);
	}
	else if (Defender->bLastHitWasCounter)
	{
		Announcement = FString(TEXT("COUNTER HIT - ")) + Move.DisplayName.ToString();
		if (CounterHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CounterHitSound, Defender->GetActorLocation(), 0.78f);
		}
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		const float Low = bBlocked ? 0.12f : (Move.bSuper ? 0.9f : Move.bKnockdown ? 0.62f : 0.38f);
		const float High = bBlocked ? 0.22f : (Defender->bLastHitWasCounter ? 0.9f : Move.bSuper ? 1.0f : 0.58f);
		const float Duration = bBlocked ? 0.06f : (Move.bSuper ? 0.24f : Move.bKnockdown ? 0.18f : 0.11f);
		PlayHaptics(PC, Low, High, Duration);
	}
}

void ARaviCircuitGameMode::HandleSuper(ARCFighterCharacter* Fighter, const FRCMoveData& Move)
{
	if (FightCamera)
	{
		FightCamera->StartCinematic(1.15f);
		SetCameraShake(0.85f);
	}
	Announcement = Move.DisplayName.ToString().ToUpper();
	AnnouncementTimer = 1.2f;
	if (SuperRiserSound && Fighter)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SuperRiserSound, Fighter->GetActorLocation(), 0.85f);
	}
}

void ARaviCircuitGameMode::SetCameraShake(float Amount)
{
	if (FightCamera)
	{
		if (bCameraShakeEnabled)
		{
			FightCamera->AddShake(Amount);
		}
	}
}

void ARaviCircuitGameMode::SpawnWallSplat(const FVector& Location)
{
	if (bHitVfxEnabled)
	{
		SpawnHitFX(Location + FVector(0.f, 0.f, 110.f), false, true);
	}
	TriggerReactiveProps(Location, 320.f);
	if (WallSplatSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WallSplatSound, Location, 0.9f);
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayHaptics(PC, 0.78f, 0.95f, 0.22f);
	}
	Announcement = TEXT("WALL SPLAT");
	AnnouncementTimer = 0.8f;
}

FString ARaviCircuitGameMode::HudLine() const
{
	if (AnnouncementTimer > 0.f)
	{
		return Announcement;
	}
	if (MatchMode == ERCMatchMode::Training && P1 && P2)
	{
		return FString::Printf(TEXT("TRAINING | P1 %s | State %d | Meter %.0f | Dummy guard %s/%s | Inputs: %s | Dummy combo %d dmg %d"),
			*P1->LastMoveName,
			static_cast<int32>(P1->FightState),
			P1->Meter,
			bTrainingDummyGuard ? TEXT("on") : TEXT("off"),
			bTrainingDummyLowGuard ? TEXT("low") : TEXT("stand"),
			*FString::Join(P1->InputHistory, TEXT(", ")),
			P2->ComboCount,
			P2->ComboDamage);
	}
	return FString();
}

FString ARaviCircuitGameMode::FrontEndLine() const
{
	const FRCFighterSpec* P1Spec = FighterSpecs.Find(P1Id);
	const FRCFighterSpec* P2Spec = FighterSpecs.Find(P2Id);
	const FString P1Name = P1Spec ? P1Spec->DisplayName.ToString() : P1Id.ToString();
	const FString P2Name = P2Spec ? P2Spec->DisplayName.ToString() : P2Id.ToString();
	return FString::Printf(TEXT("1 Arcade / VS CPU    2 Local Versus    3 Training\n4 Host Online    5 Join 127.0.0.1\nQ swap P1: %s    E swap P2: %s\nV hit VFX: %s    C camera shake: %s\nEsc quit"),
		*P1Name,
		*P2Name,
		bHitVfxEnabled ? TEXT("on") : TEXT("off"),
		bCameraShakeEnabled ? TEXT("on") : TEXT("off")) + FString::Printf(TEXT("    B haptics: %s"), bHapticsEnabled ? TEXT("on") : TEXT("off"));
}

FString ARaviCircuitGameMode::MatchResultLine() const
{
	if (!bAwaitingRematch)
	{
		return FString();
	}
	return MatchResult + TEXT("\nR rematch    M/Esc main menu");
}

FString ARaviCircuitGameMode::TrainingFrameLine() const
{
	if (MatchMode != ERCMatchMode::Training || !P1)
	{
		return FString();
	}
	TArray<FString> Rows;
	for (const TPair<FName, FRCMoveData>& Pair : P1->Spec.Moves)
	{
		const FRCMoveData& M = Pair.Value;
		Rows.Add(FString::Printf(TEXT("%s: s%d a%d r%d dmg%d block%+d hit%+d"),
			*M.DisplayName.ToString(), M.Startup, M.Active, M.Recovery, M.Damage, M.BlockAdvantage, M.HitAdvantage));
	}
	Rows.Sort();
	return FString(TEXT("F reset    G guard    H stand/low guard\n")) + FString::Join(Rows, TEXT("\n"));
}

void ARaviCircuitGameMode::SpawnHitFX(const FVector& Location, bool bBlocked, bool bBig)
{
	for (int32 i = 0; i < (bBig ? 22 : 10); ++i)
	{
		UStaticMeshComponent* Spark = AddBlock(TEXT("HitSpark"), Location + FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-10.f, 24.f)), FVector(0.035f, 0.035f, bBig ? 0.22f : 0.14f), bBlocked ? FLinearColor(0.2f, 0.75f, 1.f) : FLinearColor(1.f, 0.55f, 0.08f), 3.f);
		Spark->SetWorldRotation(FRotator(FMath::FRandRange(0.f, 180.f), FMath::FRandRange(0.f, 180.f), FMath::FRandRange(0.f, 180.f)));
		if (AActor* SparkActor = Spark->GetOwner())
		{
			SparkActor->SetLifeSpan(0.35f);
		}
	}
}

void ARaviCircuitGameMode::BuildArena()
{
	GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-52.f, -35.f, 0.f));
	AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	Fog->GetComponent()->SetFogDensity(0.010f);
	Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.045f, 0.075f, 0.10f));
	AddBlock(TEXT("Floor"), FVector(0.f, 0.f, -8.f), FVector(16.f, 8.f, 0.18f), FLinearColor(0.035f, 0.04f, 0.045f));
	AddBlock(TEXT("MatCenter"), FVector(0.f, 0.f, -2.f), FVector(9.8f, 4.6f, 0.035f), FLinearColor(0.075f, 0.078f, 0.082f), 0.02f);
	AddBlock(TEXT("LeftWall"), FVector(-795.f, 0.f, 130.f), FVector(0.16f, 8.f, 2.2f), FLinearColor(0.85f, 0.22f, 0.07f), 0.18f);
	AddBlock(TEXT("RightWall"), FVector(795.f, 0.f, 130.f), FVector(0.16f, 8.f, 2.2f), FLinearColor(0.85f, 0.22f, 0.07f), 0.18f);
	AddBlock(TEXT("BackRailLow"), FVector(0.f, -430.f, 34.f), FVector(16.f, 0.055f, 0.30f), FLinearColor(0.035f, 0.04f, 0.045f));
	AddBlock(TEXT("BackNeonTrim"), FVector(0.f, -448.f, 76.f), FVector(15.6f, 0.028f, 0.055f), FLinearColor(0.05f, 0.90f, 1.f), 2.0f);
	AddBlock(TEXT("LeftNeonTrim"), FVector(-780.f, 0.f, 252.f), FVector(0.035f, 7.5f, 0.07f), FLinearColor(1.f, 0.68f, 0.12f), 1.5f);
	AddBlock(TEXT("RightNeonTrim"), FVector(780.f, 0.f, 252.f), FVector(0.035f, 7.5f, 0.07f), FLinearColor(1.f, 0.68f, 0.12f), 1.5f);
	FRandomStream StageRandom(7426);
	for (int32 i = 0; i < 34; ++i)
	{
		float X = FMath::Lerp(-1850.f, 1850.f, static_cast<float>(i) / 33.f) + StageRandom.FRandRange(-35.f, 35.f);
		if (FMath::Abs(X) < 520.f)
		{
			X += X < 0.f ? -520.f : 520.f;
		}
		const float H = StageRandom.FRandRange(160.f, 520.f);
		const float Y = -1700.f - StageRandom.FRandRange(0.f, 520.f);
		const FVector Loc(X, Y, H * 0.5f - 40.f);
		AddBlock(TEXT("CityBlock"), Loc, FVector(StageRandom.FRandRange(0.45f, 1.10f), StageRandom.FRandRange(0.22f, 0.62f), H / 135.f), FLinearColor(StageRandom.FRandRange(0.018f, 0.055f), StageRandom.FRandRange(0.025f, 0.065f), StageRandom.FRandRange(0.045f, 0.11f)));
		if (i % 4 == 0)
		{
			AddBlock(TEXT("NeonSign"), Loc + FVector(0.f, 36.f, StageRandom.FRandRange(-H * 0.14f, H * 0.16f)), FVector(0.42f, 0.025f, 0.09f), i % 2 ? FLinearColor(0.05f, 0.9f, 1.f) : FLinearColor(1.f, 0.68f, 0.08f), 1.8f);
		}
	}
	for (int32 i = 0; i < 8; ++i)
	{
		APointLight* Light = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), FVector(StageRandom.FRandRange(-620.f, 620.f), StageRandom.FRandRange(-330.f, 260.f), StageRandom.FRandRange(360.f, 620.f)), FRotator::ZeroRotator);
		Light->GetLightComponent()->SetLightColor(i % 3 == 0 ? FLinearColor(0.2f, 0.85f, 1.f) : i % 3 == 1 ? FLinearColor(1.f, 0.68f, 0.22f) : FLinearColor(0.9f, 0.12f, 0.28f));
		Light->GetLightComponent()->SetIntensity(1100.f);
		Light->PointLightComponent->SetAttenuationRadius(560.f);
	}
	for (int32 i = 0; i < 8; ++i)
	{
		const float X = i % 2 == 0 ? StageRandom.FRandRange(-720.f, -560.f) : StageRandom.FRandRange(560.f, 720.f);
		const float Y = i < 4 ? StageRandom.FRandRange(-360.f, -220.f) : StageRandom.FRandRange(220.f, 360.f);
		UStaticMeshComponent* Prop = AddBlock(TEXT("ReactiveLantern"), FVector(X, Y, StageRandom.FRandRange(88.f, 150.f)), FVector(0.18f, 0.06f, 0.18f), i % 2 == 0 ? FLinearColor(1.f, 0.62f, 0.08f) : FLinearColor(0.05f, 0.8f, 1.f), 1.35f);
		Prop->SetMobility(EComponentMobility::Movable);
		ReactiveProps.Add(Prop);
	}
}

void ARaviCircuitGameMode::LoadAudioAssets()
{
	ImpactLightSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/impact_light.impact_light"));
	ImpactHeavySound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/impact_heavy.impact_heavy"));
	BlockSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/block_guard.block_guard"));
	SuperRiserSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/super_riser.super_riser"));
	MenuTickSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/menu_tick.menu_tick"));
	MenuConfirmSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/menu_confirm.menu_confirm"));
	MenuBackSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/menu_back.menu_back"));
	RoundStartSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/round_start.round_start"));
	AnnouncerFightSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/announcer_fight.announcer_fight"));
	AnnouncerKoSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/announcer_ko.announcer_ko"));
	AnnouncerPerfectSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/announcer_perfect.announcer_perfect"));
	KoHitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/ko_hit.ko_hit"));
	WallSplatSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/wall_splat.wall_splat"));
	CounterHitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/counter_hit.counter_hit"));
	ThrowHitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/throw_hit.throw_hit"));
	RageReadySound = LoadObject<USoundBase>(nullptr, TEXT("/Game/RaviCircuit/Audio/rage_ready.rage_ready"));
}

void ARaviCircuitGameMode::PlayMenuTick()
{
	if (MenuTickSound)
	{
		UGameplayStatics::PlaySound2D(this, MenuTickSound, 0.55f);
	}
}

void ARaviCircuitGameMode::PlayMenuConfirm()
{
	if (MenuConfirmSound)
	{
		UGameplayStatics::PlaySound2D(this, MenuConfirmSound, 0.6f);
	}
	else
	{
		PlayMenuTick();
	}
}

void ARaviCircuitGameMode::PlayMenuBack()
{
	if (MenuBackSound)
	{
		UGameplayStatics::PlaySound2D(this, MenuBackSound, 0.5f);
	}
	else
	{
		PlayMenuTick();
	}
}

void ARaviCircuitGameMode::PlayCombatSound(const FRCMoveData& Move, bool bBlocked, const FVector& Location)
{
	USoundBase* Sound = bBlocked ? BlockSound.Get() : Move.Level == ERCMoveLevel::Throw ? ThrowHitSound.Get() : (Move.Damage >= 20 || Move.bSuper ? ImpactHeavySound.Get() : ImpactLightSound.Get());
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Move.bSuper ? 0.9f : 0.72f);
	}
}

void ARaviCircuitGameMode::PlayHaptics(APlayerController* PC, float Low, float High, float Duration)
{
	if (!bHapticsEnabled || !PC)
	{
		return;
	}
	PC->PlayDynamicForceFeedback(FMath::Clamp(FMath::Max(Low, High), 0.f, 1.f), Duration, true, true, true, true);
}

void ARaviCircuitGameMode::TriggerReactiveProps(const FVector& Location, float Radius)
{
	for (UStaticMeshComponent* Prop : ReactiveProps)
	{
		if (!Prop || !Prop->IsVisible())
		{
			continue;
		}
		if (FVector::Dist(Prop->GetComponentLocation(), Location) > Radius)
		{
			continue;
		}
		const FVector Direction = (Prop->GetComponentLocation() - Location).GetSafeNormal();
		Prop->AddWorldOffset(Direction * FMath::FRandRange(45.f, 95.f) + FVector(0.f, 0.f, FMath::FRandRange(20.f, 70.f)));
		Prop->AddWorldRotation(FRotator(FMath::FRandRange(-35.f, 35.f), FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-35.f, 35.f)));
		Prop->SetWorldScale3D(Prop->GetComponentScale() * 0.72f);
		if (FMath::FRand() < 0.28f)
		{
			Prop->SetVisibility(false, true);
		}
	}
}

UStaticMeshComponent* ARaviCircuitGameMode::AddBlock(const TCHAR* Name, FVector Location, FVector Scale, FLinearColor Color, float Emission)
{
	AActor* Actor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, Name);
	Mesh->RegisterComponent();
	Actor->SetRootComponent(Mesh);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Mesh->SetWorldScale3D(Scale);
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMaterial)
	{
		BaseMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(BaseMaterial, Actor);
	Mat->SetVectorParameterValue("Color", Color);
	Mat->SetVectorParameterValue("BaseColor", Color);
	Mat->SetVectorParameterValue("EmissiveColor", Color * Emission);
	Mat->SetScalarParameterValue("Roughness", 0.42f);
	Mesh->SetMaterial(0, Mat);
	return Mesh;
}
