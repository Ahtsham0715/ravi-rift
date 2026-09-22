#include "RCFightCameraActor.h"

#include "Camera/CameraComponent.h"
#include "RCFighterCharacter.h"

ARCFightCameraActor::ARCFightCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>("FightCamera");
	SetRootComponent(Camera);
	Camera->FieldOfView = 48.f;
}

void ARCFightCameraActor::Configure(ARCFighterCharacter* InP1, ARCFighterCharacter* InP2)
{
	P1 = InP1;
	P2 = InP2;
}

void ARCFightCameraActor::AddShake(float Amount)
{
	Shake = FMath::Max(Shake, Amount);
}

void ARCFightCameraActor::StartCinematic(float Duration)
{
	CinematicTimer = FMath::Max(CinematicTimer, Duration);
}

void ARCFightCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!P1 || !P2)
	{
		return;
	}
	const FVector Mid = (P1->GetActorLocation() + P2->GetActorLocation()) * 0.5f;
	const float Dist = FVector::Dist(P1->GetActorLocation(), P2->GetActorLocation());
	FVector Target = Mid + FVector(0.f, 720.f + Dist * 0.45f, 285.f + Dist * 0.1f);
	if (CinematicTimer > 0.f)
	{
		CinematicTimer -= DeltaSeconds;
		ARCFighterCharacter* Leader = P1->FightState == ERCFighterState::Attack ? P1 : P2;
		Target = Leader->GetActorLocation() + FVector(-Leader->Facing * 215.f, 270.f, 185.f);
	}
	if (Shake > 0.f)
	{
		Target += FVector(FMath::FRandRange(-Shake, Shake) * 16.f, FMath::FRandRange(-Shake, Shake) * 12.f, FMath::FRandRange(-Shake, Shake) * 10.f);
		Shake = FMath::Max(0.f, Shake - DeltaSeconds * 2.5f);
	}
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), Target, DeltaSeconds, 8.f));
	const FRotator Look = (Mid + FVector(0.f, 0.f, 120.f) - GetActorLocation()).Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Look, DeltaSeconds, 9.f));
}

