#include "RCFighterCharacter.h"

#include "RaviCircuitGameMode.h"
#include "Camera/CameraShakeBase.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float WallX = 720.f;
	constexpr float WallZ = 315.f;
	constexpr float FrameStep = 1.f / 60.f;
}

ARCFighterCharacter::ARCFighterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	GetCapsuleComponent()->InitCapsuleSize(44.f, 96.f);
	GetCharacterMovement()->GravityScale = 2.25f;
	GetCharacterMovement()->AirControl = 0.42f;
	GetCharacterMovement()->BrakingFrictionFactor = 4.5f;
	GetCharacterMovement()->MaxWalkSpeed = 560.f;
	BodyRoot = CreateDefaultSubobject<USceneComponent>("BodyRoot");
	BodyRoot->SetupAttachment(GetRootComponent());
}

void ARCFighterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ARCFighterCharacter::Configure(const FRCFighterSpec& InSpec, int32 InPlayerIndex, bool bInCPU, ARaviCircuitGameMode* InGame)
{
	Spec = InSpec;
	PlayerIndex = InPlayerIndex;
	bCPU = bInCPU;
	Game = InGame;
	GetCharacterMovement()->MaxWalkSpeed = Spec.WalkSpeed;
	JumpMaxHoldTime = 0.08f;
	BuildVisuals();
}

void ARCFighterCharacter::ResetForRound(const FVector& Location)
{
	SetActorLocation(Location);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	Health = MaxHealth;
	Meter = 30.f;
	bRage = false;
	ComboCount = 0;
	ComboDamage = 0;
	bLastHitWasCounter = false;
	FightState = ERCFighterState::Idle;
	CurrentMoveId = NAME_None;
	QueuedMove = NAME_None;
	BufferedMove = NAME_None;
	MoveFrame = 0;
	BufferedMoveFrames = 0;
	bHitConnected = false;
	StunFrames = 0;
	BlockFrames = 0;
	KnockdownFrames = 0;
	InvulnerableFrames = 0;
	SuperFreezeFrames = 0;
	HitScale = 1.f;
	InputHistory.Reset();
}

void ARCFighterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("P1Forward", this, &ARCFighterCharacter::AxisForward);
	PlayerInputComponent->BindAxis("P1Right", this, &ARCFighterCharacter::AxisRight);
	PlayerInputComponent->BindAxis("P1Side", this, &ARCFighterCharacter::AxisSide);
	PlayerInputComponent->BindAction("P1Punch", IE_Pressed, this, &ARCFighterCharacter::InputPunch);
	PlayerInputComponent->BindAction("P1Kick", IE_Pressed, this, &ARCFighterCharacter::InputKick);
	PlayerInputComponent->BindAction("P1Low", IE_Pressed, this, &ARCFighterCharacter::InputLow);
	PlayerInputComponent->BindAction("P1Throw", IE_Pressed, this, &ARCFighterCharacter::InputThrow);
	PlayerInputComponent->BindAction("P1Special", IE_Pressed, this, &ARCFighterCharacter::InputSpecial);
	PlayerInputComponent->BindAction("P1Super", IE_Pressed, this, &ARCFighterCharacter::InputSuper);
	PlayerInputComponent->BindAction("P1Block", IE_Pressed, this, &ARCFighterCharacter::SetBlockPressed);
	PlayerInputComponent->BindAction("P1Block", IE_Released, this, &ARCFighterCharacter::SetBlockReleased);
}

void ARCFighterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocallyControlled() && !HasAuthority())
	{
		ServerSyncInput(InputX, InputSide, bBlockButtonDown, bCrouching, bJumpQueued);
	}
	if (!HasAuthority())
	{
		AnimatePose(DeltaSeconds);
		return;
	}
	if (Health <= 0)
	{
		FightState = ERCFighterState::KO;
		AnimatePose(DeltaSeconds);
		return;
	}
	if (Game && Game->bPausedMatch)
	{
		return;
	}
	if (bCPU)
	{
		RunCPU(DeltaSeconds);
	}
	else
	{
		bBlocking = bBlockButtonDown || InputX < -0.55f;
	}
	if (bForcedGuard)
	{
		bBlocking = true;
		bCrouching = bForcedLowGuard;
	}
	UpdateFacing();
	ApplyMovement(DeltaSeconds);
	FrameAccumulator += DeltaSeconds;
	while (FrameAccumulator >= FrameStep)
	{
		FrameAccumulator -= FrameStep;
	if (SuperFreezeFrames > 0)
		{
			--SuperFreezeFrames;
			continue;
		}
		UpdateStateFrames();
		TryWakeupOptions();
		if (BufferedMoveFrames > 0)
		{
			--BufferedMoveFrames;
			if (FightState == ERCFighterState::Idle && !BufferedMove.IsNone())
			{
				const FName Next = BufferedMove;
				BufferedMove = NAME_None;
				BufferedMoveFrames = 0;
				StartMove(Next);
			}
		}
		UpdateMove();
	}
	AnimatePose(DeltaSeconds);
	FVector P = GetActorLocation();
	P.X = FMath::Clamp(P.X, -WallX, WallX);
	P.Y = FMath::Clamp(P.Y, -WallZ, WallZ);
	SetActorLocation(P);
}

void ARCFighterCharacter::AxisForward(float Value)
{
	bCrouching = Value < -0.55f;
	if (Value > 0.7f)
	{
		bJumpQueued = true;
	}
}

void ARCFighterCharacter::AxisRight(float Value)
{
	InputX = FMath::Clamp(Value, -1.f, 1.f);
}

void ARCFighterCharacter::AxisSide(float Value)
{
	InputSide = FMath::Clamp(Value, -1.f, 1.f);
}

void ARCFighterCharacter::SetMoveInput(FName MoveId)
{
	if (!HasAuthority())
	{
		ServerSetMoveInput(MoveId);
		return;
	}
	if (FightState == ERCFighterState::Knockdown && KnockdownFrames < 54 && (MoveId == "Kick" || MoveId == "Low" || MoveId == "Special"))
	{
		KnockdownFrames = 0;
		InvulnerableFrames = 10;
		FightState = ERCFighterState::Idle;
		StartMove(MoveId);
		PushHistory(TEXT("Wake-up"));
		return;
	}
	if (FightState == ERCFighterState::HitStun || FightState == ERCFighterState::BlockStun || FightState == ERCFighterState::Knockdown || FightState == ERCFighterState::KO)
	{
		if (FightState != ERCFighterState::KO)
		{
			BufferedMove = MoveId;
			BufferedMoveFrames = 8;
		}
		return;
	}
	if (MoveId == "Super" && Meter < 100.f)
	{
		return;
	}
	if (FightState == ERCFighterState::Attack)
	{
		if (CurrentMove.CancelInto.Contains(MoveId) && MoveFrame > CurrentMove.Startup)
		{
			QueuedMove = MoveId;
		}
		else
		{
			BufferedMove = MoveId;
			BufferedMoveFrames = 8;
		}
		return;
	}
	StartMove(MoveId);
}

void ARCFighterCharacter::InputPunch()
{
	SetMoveInput("Punch");
}

void ARCFighterCharacter::InputKick()
{
	SetMoveInput("Kick");
}

void ARCFighterCharacter::InputLow()
{
	SetMoveInput("Low");
}

void ARCFighterCharacter::InputThrow()
{
	SetMoveInput("Throw");
}

void ARCFighterCharacter::InputSpecial()
{
	SetMoveInput("Special");
}

void ARCFighterCharacter::InputSuper()
{
	SetMoveInput("Super");
}

void ARCFighterCharacter::SetBlockPressed()
{
	bBlockButtonDown = true;
	PushHistory(TEXT("Block"));
}

void ARCFighterCharacter::SetBlockReleased()
{
	bBlockButtonDown = false;
}

void ARCFighterCharacter::JumpPressed()
{
	bJumpQueued = true;
}

void ARCFighterCharacter::StartMove(FName MoveId)
{
	const FRCMoveData* Found = Spec.Moves.Find(MoveId);
	if (!Found)
	{
		return;
	}
	CurrentMove = *Found;
	CurrentMoveId = MoveId;
	MoveFrame = 0;
	bHitConnected = false;
	FightState = ERCFighterState::Attack;
	LastMoveName = CurrentMove.DisplayName.ToString();
	PushHistory(LastMoveName);
	if (CurrentMove.bSuper)
	{
		Meter = 0.f;
		SuperFreezeFrames = 16;
		OnStartedSuper.Broadcast(this, CurrentMove);
	}
}

void ARCFighterCharacter::UpdateMove()
{
	if (FightState != ERCFighterState::Attack)
	{
		return;
	}
	++MoveFrame;
	if (MoveFrame >= CurrentMove.Startup && MoveFrame <= CurrentMove.Startup + CurrentMove.Active && !bHitConnected)
	{
		TryHit();
	}
	if (MoveFrame >= CurrentMove.TotalFrames())
	{
		if (!QueuedMove.IsNone())
		{
			FName Next = QueuedMove;
			QueuedMove = NAME_None;
			StartMove(Next);
		}
		else
		{
			FightState = ERCFighterState::Idle;
			CurrentMoveId = NAME_None;
		}
	}
}

void ARCFighterCharacter::TryHit()
{
	if (!Opponent || Opponent->Health <= 0 || Opponent->InvulnerableFrames > 0)
	{
		return;
	}
	const FVector Delta = Opponent->GetActorLocation() - GetActorLocation();
	if (FMath::Abs(Delta.Y) > 92.f + CurrentMove.Tracking || FMath::Abs(Delta.X) > CurrentMove.Range)
	{
		return;
	}
	bHitConnected = true;
	const bool bBlocked = Opponent->CanBlock(CurrentMove, Facing);
	Opponent->ReceiveAttack(this, CurrentMove, bBlocked);
	OnLandedHit.Broadcast(this, Opponent, CurrentMove, bBlocked);
}

void ARCFighterCharacter::ReceiveAttack(ARCFighterCharacter* Attacker, const FRCMoveData& Move, bool bBlocked)
{
	const float RageBonus = Attacker && Attacker->bRage ? 1.12f : 1.f;
	const bool bCounterHit = !bBlocked && FightState == ERCFighterState::Attack && MoveFrame < CurrentMove.Startup;
	bLastHitWasCounter = bCounterHit;
	const float CounterBonus = bCounterHit ? Move.CounterDamageMultiplier : 1.f;
	const int32 Damage = bBlocked ? Move.Chip : FMath::RoundToInt(Move.Damage * FMath::Max(0.32f, Attacker ? Attacker->HitScale : 1.f) * RageBonus * CounterBonus);
	Health = FMath::Max(0, Health - Damage);
	const float Dir = Attacker ? Attacker->Facing : 1.f;
	if (bBlocked)
	{
		BlockFrames = FMath::Max(6, 12 - Move.BlockAdvantage);
		FightState = ERCFighterState::BlockStun;
		LaunchCharacter(FVector(Dir * Move.Pushback, 0.f, 0.f), true, false);
	}
	else
	{
		StunFrames = FMath::Max(8, 18 + Move.HitAdvantage);
		FightState = ERCFighterState::HitStun;
		++ComboCount;
		ComboDamage += Damage;
		if (Attacker)
		{
			Attacker->HitScale *= 0.86f;
			Attacker->Meter = FMath::Clamp(Attacker->Meter + Move.MeterGain, 0.f, 100.f);
		}
		LaunchCharacter(FVector(Dir * Move.Pushback, 0.f, Move.LaunchVelocity), true, Move.LaunchVelocity > 0.f);
		if (Move.bKnockdown || Health <= 0)
		{
			KnockdownFrames = 72;
			FightState = ERCFighterState::Knockdown;
		}
		if (TouchingWall(Dir))
		{
			StunFrames += 24;
			LaunchCharacter(FVector(-Dir * 60.f, 0.f, 360.f), true, true);
			if (Game)
			{
				Game->SpawnWallSplat(GetActorLocation());
			}
		}
	}
	Meter = FMath::Clamp(Meter + 7.f, 0.f, 100.f);
	bRage = Health <= MaxHealth * 0.28f;
}

bool ARCFighterCharacter::CanBlock(const FRCMoveData& Move, float AttackerFacing) const
{
	if (Move.Level == ERCMoveLevel::Throw || !bBlocking)
	{
		return false;
	}
	if (Move.Level == ERCMoveLevel::Low && !bCrouching)
	{
		return false;
	}
	if (Move.Level == ERCMoveLevel::High && bCrouching)
	{
		return false;
	}
	return FMath::Sign(AttackerFacing) == FMath::Sign(Facing);
}

void ARCFighterCharacter::UpdateStateFrames()
{
	if (InvulnerableFrames > 0)
	{
		--InvulnerableFrames;
	}
	if (StunFrames > 0 && --StunFrames == 0 && FightState == ERCFighterState::HitStun)
	{
		FightState = ERCFighterState::Idle;
		ComboCount = 0;
		ComboDamage = 0;
	}
	if (BlockFrames > 0 && --BlockFrames == 0 && FightState == ERCFighterState::BlockStun)
	{
		FightState = ERCFighterState::Idle;
	}
	if (KnockdownFrames > 0 && --KnockdownFrames == 0)
	{
		FightState = ERCFighterState::Idle;
		InvulnerableFrames = 24;
		ComboCount = 0;
		ComboDamage = 0;
	}
	HitScale = FMath::Min(1.f, HitScale + 0.005f);
}

void ARCFighterCharacter::TryWakeupOptions()
{
	if (FightState != ERCFighterState::Knockdown || KnockdownFrames > 54 || KnockdownFrames <= 8)
	{
		return;
	}
	if (FMath::Abs(InputSide) > 0.6f)
	{
		AddActorWorldOffset(FVector(0.f, FMath::Sign(InputSide) * 118.f, 0.f), true);
		KnockdownFrames = 0;
		InvulnerableFrames = 18;
		FightState = ERCFighterState::Idle;
		PushHistory(TEXT("Wake roll"));
	}
}

void ARCFighterCharacter::ApplyMovement(float DeltaSeconds)
{
	if (FightState == ERCFighterState::HitStun || FightState == ERCFighterState::BlockStun || FightState == ERCFighterState::Knockdown || FightState == ERCFighterState::KO)
	{
		return;
	}
	float LocalX = InputX;
	float LocalY = InputSide;
	if (FightState == ERCFighterState::Attack)
	{
		LocalX *= 0.14f;
		LocalY *= 0.22f;
	}
	AddMovementInput(FVector(Facing, 0.f, 0.f), LocalX);
	AddMovementInput(FVector(0.f, 1.f, 0.f), LocalY * (Spec.SideSpeed / FMath::Max(1.f, Spec.WalkSpeed)));
	if (bJumpQueued && GetCharacterMovement()->IsMovingOnGround() && FightState == ERCFighterState::Idle)
	{
		LaunchCharacter(FVector(0.f, 0.f, Spec.JumpSpeed), false, true);
	}
	bJumpQueued = false;
}

void ARCFighterCharacter::UpdateFacing()
{
	if (!Opponent)
	{
		return;
	}
	Facing = Opponent->GetActorLocation().X < GetActorLocation().X ? -1.f : 1.f;
	const FRotator Target(0.f, Facing > 0.f ? 90.f : -90.f, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, GetWorld()->GetDeltaSeconds(), 12.f));
}

void ARCFighterCharacter::RunCPU(float DeltaSeconds)
{
	AiTimer -= DeltaSeconds;
	if (AiTimer <= 0.f)
	{
		const float D = DistanceToOpponent();
		if (D > 310.f)
		{
			AiPlan = "Approach";
		}
		else if (Opponent && Opponent->FightState == ERCFighterState::Attack && FMath::FRand() < 0.48f)
		{
			AiPlan = "Guard";
		}
		else
		{
			static const FName Choices[] = {"Punch", "Kick", "Low", "Throw", "Special", "Launcher"};
			AiPlan = Choices[FMath::RandRange(0, UE_ARRAY_COUNT(Choices) - 1)];
			if (Meter >= 100.f && Health < MaxHealth * 0.42f && FMath::FRand() < 0.35f)
			{
				AiPlan = "Super";
			}
		}
		AiTimer = FMath::FRandRange(0.18f, 0.62f);
	}
	InputX = 0.f;
	InputSide = 0.f;
	bBlockButtonDown = false;
	bBlocking = false;
	bCrouching = false;
	if (AiPlan == "Approach")
	{
		InputX = 1.f;
	}
	else if (AiPlan == "Guard")
	{
		bBlocking = true;
		bCrouching = FMath::FRand() < 0.25f;
	}
	else if (FightState == ERCFighterState::Idle)
	{
		StartMove(AiPlan);
	}
}

void ARCFighterCharacter::ApplyP2KeyboardInput(APlayerController* PC, float DeltaSeconds)
{
	if (!PC || bCPU || PlayerIndex != 1)
	{
		return;
	}
	InputX = 0.f;
	InputSide = 0.f;
	if (PC->IsInputKeyDown(EKeys::Right)) { InputX += 1.f; }
	if (PC->IsInputKeyDown(EKeys::Left)) { InputX -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::Period)) { InputSide += 1.f; }
	if (PC->IsInputKeyDown(EKeys::Comma)) { InputSide -= 1.f; }
	bCrouching = PC->IsInputKeyDown(EKeys::Down);
	bJumpQueued = PC->WasInputKeyJustPressed(EKeys::Up);
	bBlockButtonDown = PC->IsInputKeyDown(EKeys::RightControl);
	if (PC->WasInputKeyJustPressed(EKeys::NumPadOne)) { SetMoveInput("Punch"); }
	if (PC->WasInputKeyJustPressed(EKeys::NumPadTwo)) { SetMoveInput("Kick"); }
	if (PC->WasInputKeyJustPressed(EKeys::NumPadThree)) { SetMoveInput("Low"); }
	if (PC->WasInputKeyJustPressed(EKeys::NumPadFour)) { SetMoveInput("Throw"); }
	if (PC->WasInputKeyJustPressed(EKeys::NumPadFive)) { SetMoveInput("Special"); }
	if (PC->WasInputKeyJustPressed(EKeys::NumPadSix)) { SetMoveInput("Super"); }
}

void ARCFighterCharacter::SetForcedGuard(bool bEnabled, bool bLowGuard)
{
	bForcedGuard = bEnabled;
	bForcedLowGuard = bLowGuard;
	if (!bForcedGuard)
	{
		bBlockButtonDown = false;
	}
}

void ARCFighterCharacter::PushHistory(const FString& Text)
{
	InputHistory.Insert(Text, 0);
	if (InputHistory.Num() > 10)
	{
		InputHistory.SetNum(10);
	}
}

void ARCFighterCharacter::BuildVisuals()
{
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInstanceDynamic* Primary = MakeMaterial(Spec.Primary, 0.18f);
	UMaterialInstanceDynamic* Accent = MakeMaterial(Spec.Accent, 0.35f);
	UMaterialInstanceDynamic* Dark = MakeMaterial(Spec.Dark, 0.02f);
	UMaterialInstanceDynamic* Skin = MakeMaterial(PlayerIndex == 0 ? FLinearColor(0.66f, 0.42f, 0.31f) : FLinearColor(0.72f, 0.46f, 0.32f), 0.f);
	UMaterialInstanceDynamic* Trim = MakeMaterial(PlayerIndex == 0 ? FLinearColor(0.04f, 0.85f, 1.f) : FLinearColor(1.f, 0.18f, 0.10f), 0.75f);
	const float S = Spec.BodyScale;
	const bool bPowerBuild = PlayerIndex == 1;
	Torso = AddPart(TEXT("Torso"), Sphere, Primary, FVector(0, 0, 126), FVector(bPowerBuild ? 0.70f : 0.54f, bPowerBuild ? 0.44f : 0.34f, bPowerBuild ? 0.88f : 0.82f) * S);
	Head = AddPart(TEXT("Head"), Sphere, Skin, FVector(0, 0, bPowerBuild ? 215 : 208), FVector(0.31f, 0.31f, 0.35f) * S);
	AddPart(TEXT("ChestPlate"), Cube, Dark, FVector(-12, 0, 142), FVector(0.06f, bPowerBuild ? 0.55f : 0.44f, 0.34f) * S);
	AddPart(TEXT("FaceGuard"), Cube, Trim, FVector(-27, 0, bPowerBuild ? 216 : 209), FVector(0.035f, 0.20f, 0.055f) * S);
	AddPart(TEXT("LeftShoulder"), Sphere, Accent, FVector(0, -47, 158), FVector(0.24f, 0.20f, 0.20f) * S);
	AddPart(TEXT("RightShoulder"), Sphere, Accent, FVector(0, 47, 158), FVector(0.24f, 0.20f, 0.20f) * S);
	LeftArm = AddPart(TEXT("LeftArm"), Cylinder, Accent, FVector(0, -67, 130), FVector(0.13f, 0.13f, 0.48f) * S);
	RightArm = AddPart(TEXT("RightArm"), Cylinder, Accent, FVector(0, 67, 130), FVector(0.13f, 0.13f, 0.48f) * S);
	AddPart(TEXT("LeftGlove"), Sphere, Trim, FVector(20, -96, 122), FVector(0.22f, 0.20f, 0.18f) * S);
	AddPart(TEXT("RightGlove"), Sphere, Trim, FVector(20, 96, 122), FVector(0.22f, 0.20f, 0.18f) * S);
	LeftLeg = AddPart(TEXT("LeftLeg"), Cylinder, Dark, FVector(0, -25, 55), FVector(0.16f, 0.16f, 0.64f) * S);
	RightLeg = AddPart(TEXT("RightLeg"), Cylinder, Dark, FVector(0, 25, 55), FVector(0.16f, 0.16f, 0.64f) * S);
	AddPart(TEXT("LeftBoot"), Cube, Trim, FVector(22, -27, 12), FVector(0.34f, 0.18f, 0.12f) * S);
	AddPart(TEXT("RightBoot"), Cube, Trim, FVector(22, 27, 12), FVector(0.34f, 0.18f, 0.12f) * S);
	Sash = AddPart(TEXT("Sash"), Cube, Trim, FVector(-6, 0, 111), FVector(0.10f, bPowerBuild ? 0.95f : 0.78f, 0.065f) * S);
	if (UTexture2D* Concept = LoadSourcePng(PlayerIndex == 0 ? TEXT("Concepts/zara_vey_concept.png") : TEXT("Concepts/hamza_kade_concept.png")))
	{
		FighterArt = NewObject<UBillboardComponent>(this, TEXT("GeneratedFighterArt"));
		FighterArt->RegisterComponent();
		FighterArt->AttachToComponent(BodyRoot, FAttachmentTransformRules::KeepRelativeTransform);
		FighterArt->SetSprite(Concept);
		FighterArt->bIsScreenSizeScaled = false;
		FighterArt->ScreenSize = 0.0025f;
		FighterArt->OpacityMaskRefVal = 0.08f;
		FighterArt->SetRelativeLocation(FVector(-26.f, 0.f, 140.f));
		FighterArt->SetRelativeScale3D(FVector(2.25f * S, 2.25f * S, 2.25f * S));
		FighterArt->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		TArray<USceneComponent*> Children;
		BodyRoot->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			if (Child && Child != FighterArt)
			{
				Child->SetVisibility(false, true);
			}
		}
	}
}

UStaticMeshComponent* ARCFighterCharacter::AddPart(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Mat, FVector Loc, FVector Scale)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
	Part->RegisterComponent();
	Part->AttachToComponent(BodyRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Part->SetStaticMesh(Mesh);
	Part->SetMaterial(0, Mat);
	Part->SetRelativeLocation(Loc);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return Part;
}

UMaterialInstanceDynamic* ARCFighterCharacter::MakeMaterial(FLinearColor Color, float Emission)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		Base = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, this);
	Mat->SetVectorParameterValue("Color", Color);
	Mat->SetVectorParameterValue("BaseColor", Color);
	Mat->SetVectorParameterValue("EmissiveColor", Color * Emission);
	Mat->SetScalarParameterValue("Roughness", 0.48f);
	return Mat;
}

UTexture2D* ARCFighterCharacter::LoadSourcePng(const TCHAR* SourceArtRelativePath)
{
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

void ARCFighterCharacter::AnimatePose(float DeltaSeconds)
{
	const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Bob = FMath::Sin(T * 6.f + PlayerIndex) * 2.5f;
	BodyRoot->SetRelativeLocation(FVector(0, 0, Bob));
	const float AttackAlpha = FightState == ERCFighterState::Attack ? FMath::Sin(FMath::Clamp(static_cast<float>(MoveFrame) / FMath::Max(1, CurrentMove.TotalFrames()), 0.f, 1.f) * PI) : 0.f;
	const float StunAlpha = (FightState == ERCFighterState::HitStun || FightState == ERCFighterState::BlockStun) ? 1.f : 0.f;
	RightArm->SetRelativeRotation(FMath::RInterpTo(RightArm->GetRelativeRotation(), FRotator(54.f * AttackAlpha, 0.f, 82.f - 35.f * StunAlpha), DeltaSeconds, 12.f));
	LeftArm->SetRelativeRotation(FMath::RInterpTo(LeftArm->GetRelativeRotation(), FRotator(-40.f * AttackAlpha, 0.f, -82.f + 28.f * StunAlpha), DeltaSeconds, 12.f));
	const bool bKickPose = CurrentMoveId == "Kick" || CurrentMoveId == "Low" || CurrentMoveId == "Special";
	RightLeg->SetRelativeRotation(FMath::RInterpTo(RightLeg->GetRelativeRotation(), FRotator(bKickPose ? -62.f * AttackAlpha : 0.f, 0.f, 0.f), DeltaSeconds, 10.f));
	Torso->SetRelativeRotation(FMath::RInterpTo(Torso->GetRelativeRotation(), FRotator(0.f, 0.f, -14.f * Facing * AttackAlpha + 10.f * StunAlpha), DeltaSeconds, 9.f));
	if (FighterArt)
	{
		const float ArtLean = -3.f * Facing + 10.f * AttackAlpha * Facing - 8.f * StunAlpha * Facing;
		FighterArt->SetRelativeRotation(FMath::RInterpTo(FighterArt->GetRelativeRotation(), FRotator(0.f, 0.f, ArtLean), DeltaSeconds, 9.f));
		FighterArt->SetRelativeLocation(FVector(-26.f + 12.f * AttackAlpha * Facing, 0.f, 140.f + (bCrouching ? -24.f : 0.f)));
	}
	const float Knock = FightState == ERCFighterState::Knockdown || FightState == ERCFighterState::KO ? 82.f * Facing : 0.f;
	BodyRoot->SetRelativeRotation(FMath::RInterpTo(BodyRoot->GetRelativeRotation(), FRotator(0.f, 0.f, Knock), DeltaSeconds, 6.f));
	BodyRoot->SetRelativeScale3D(FVector(1.f, 1.f, bCrouching && FightState == ERCFighterState::Idle ? 0.72f : 1.f));
}

bool ARCFighterCharacter::TouchingWall(float AttackerFacing) const
{
	const float X = GetActorLocation().X;
	return (AttackerFacing > 0.f && X > WallX - 45.f) || (AttackerFacing < 0.f && X < -WallX + 45.f);
}

float ARCFighterCharacter::DistanceToOpponent() const
{
	return Opponent ? FVector::Dist(GetActorLocation(), Opponent->GetActorLocation()) : 99999.f;
}

void ARCFighterCharacter::ServerSetMoveInput_Implementation(FName MoveId)
{
	SetMoveInput(MoveId);
}

void ARCFighterCharacter::ServerSyncInput_Implementation(float InInputX, float InInputSide, bool bInBlockDown, bool bInCrouching, bool bInJumpQueued)
{
	InputX = FMath::Clamp(InInputX, -1.f, 1.f);
	InputSide = FMath::Clamp(InInputSide, -1.f, 1.f);
	bBlockButtonDown = bInBlockDown;
	bCrouching = bInCrouching;
	bJumpQueued = bJumpQueued || bInJumpQueued;
}

void ARCFighterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARCFighterCharacter, FightState);
	DOREPLIFETIME(ARCFighterCharacter, PlayerIndex);
	DOREPLIFETIME(ARCFighterCharacter, bCPU);
	DOREPLIFETIME(ARCFighterCharacter, Facing);
	DOREPLIFETIME(ARCFighterCharacter, Health);
	DOREPLIFETIME(ARCFighterCharacter, Meter);
	DOREPLIFETIME(ARCFighterCharacter, bRage);
	DOREPLIFETIME(ARCFighterCharacter, ComboCount);
	DOREPLIFETIME(ARCFighterCharacter, ComboDamage);
	DOREPLIFETIME(ARCFighterCharacter, LastMoveName);
	DOREPLIFETIME(ARCFighterCharacter, bLastHitWasCounter);
}
