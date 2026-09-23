#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RCMoveData.h"
#include "RCFighterCharacter.generated.h"

class ARaviCircuitGameMode;

DECLARE_MULTICAST_DELEGATE_FourParams(FRCHitEvent, class ARCFighterCharacter*, class ARCFighterCharacter*, const FRCMoveData&, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FRCSuperEvent, class ARCFighterCharacter*, const FRCMoveData&);

UCLASS()
class RAVICIRCUIT_API ARCFighterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARCFighterCharacter();

	FRCHitEvent OnLandedHit;
	FRCSuperEvent OnStartedSuper;

	UPROPERTY(BlueprintReadOnly) FRCFighterSpec Spec;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<ARCFighterCharacter> Opponent;
	UPROPERTY(Replicated, BlueprintReadOnly) ERCFighterState FightState = ERCFighterState::Idle;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 PlayerIndex = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bCPU = false;
	UPROPERTY(Replicated, BlueprintReadOnly) float Facing = 1.f;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Health = 240;
	UPROPERTY(BlueprintReadOnly) int32 MaxHealth = 240;
	UPROPERTY(Replicated, BlueprintReadOnly) float Meter = 30.f;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bRage = false;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 ComboCount = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 ComboDamage = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) FString LastMoveName;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bLastHitWasCounter = false;
	UPROPERTY(BlueprintReadOnly) TArray<FString> InputHistory;

	void Configure(const FRCFighterSpec& InSpec, int32 InPlayerIndex, bool bInCPU, ARaviCircuitGameMode* InGame);
	void ResetForRound(const FVector& Location);
	void ReceiveAttack(ARCFighterCharacter* Attacker, const FRCMoveData& Move, bool bBlocked);
	void StartMove(FName MoveId);
	void ApplyP2KeyboardInput(APlayerController* PC, float DeltaSeconds);
	void SetForcedGuard(bool bEnabled, bool bLowGuard);

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<ARaviCircuitGameMode> Game;
	UPROPERTY() TObjectPtr<USceneComponent> BodyRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Torso;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftArm;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RightArm;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftLeg;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RightLeg;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Sash;

	FRCMoveData CurrentMove;
	FName CurrentMoveId = NAME_None;
	FName QueuedMove = NAME_None;
	FName BufferedMove = NAME_None;
	int32 MoveFrame = 0;
	int32 BufferedMoveFrames = 0;
	bool bHitConnected = false;
	bool bBlocking = false;
	bool bBlockButtonDown = false;
	bool bCrouching = false;
	bool bForcedGuard = false;
	bool bForcedLowGuard = false;
	bool bJumpQueued = false;
	float InputX = 0.f;
	float InputSide = 0.f;
	float HitScale = 1.f;
	float FrameAccumulator = 0.f;
	int32 StunFrames = 0;
	int32 BlockFrames = 0;
	int32 KnockdownFrames = 0;
	int32 InvulnerableFrames = 0;
	int32 SuperFreezeFrames = 0;
	float AiTimer = 0.f;
	FName AiPlan = NAME_None;

	void SetMoveInput(FName MoveId);
	void InputPunch();
	void InputKick();
	void InputLow();
	void InputThrow();
	void InputSpecial();
	void InputSuper();
	void SetBlockPressed();
	void SetBlockReleased();
	void AxisForward(float Value);
	void AxisRight(float Value);
	void AxisSide(float Value);
	void JumpPressed();
	void UpdateFacing();
	void UpdateStateFrames();
	void TryWakeupOptions();
	void ApplyMovement(float DeltaSeconds);
	void UpdateMove();
	void TryHit();
	bool CanBlock(const FRCMoveData& Move, float AttackerFacing) const;
	void RunCPU(float DeltaSeconds);
	void PushHistory(const FString& Text);
	void BuildVisuals();
	UStaticMeshComponent* AddPart(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Mat, FVector Loc, FVector Scale);
	UMaterialInstanceDynamic* MakeMaterial(FLinearColor Color, float Emission);
	void AnimatePose(float DeltaSeconds);
	bool TouchingWall(float AttackerFacing) const;
	float DistanceToOpponent() const;

	UFUNCTION(Server, Reliable)
	void ServerSetMoveInput(FName MoveId);

	UFUNCTION(Server, Unreliable)
	void ServerSyncInput(float InInputX, float InInputSide, bool bInBlockDown, bool bInCrouching, bool bInJumpQueued);
};
