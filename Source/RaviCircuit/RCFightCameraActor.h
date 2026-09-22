#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RCFightCameraActor.generated.h"

class UCameraComponent;
class ARCFighterCharacter;

UCLASS()
class RAVICIRCUIT_API ARCFightCameraActor : public AActor
{
	GENERATED_BODY()

public:
	ARCFightCameraActor();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<ARCFighterCharacter> P1;
	UPROPERTY() TObjectPtr<ARCFighterCharacter> P2;
	UPROPERTY(BlueprintReadOnly) float Shake = 0.f;
	UPROPERTY(BlueprintReadOnly) float CinematicTimer = 0.f;

	virtual void Tick(float DeltaSeconds) override;
	void Configure(ARCFighterCharacter* InP1, ARCFighterCharacter* InP2);
	void AddShake(float Amount);
	void StartCinematic(float Duration);
};

