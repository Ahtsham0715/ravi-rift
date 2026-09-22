#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RCMoveData.h"
#include "RaviCircuitGameMode.generated.h"

class ARCFighterCharacter;
class ARCFightCameraActor;
class ARCHUD;
class APlayerController;
class USoundBase;

UENUM(BlueprintType)
enum class ERCMatchMode : uint8
{
	ArcadeCPU,
	LocalVersus,
	Training
};

UCLASS()
class RAVICIRCUIT_API ARaviCircuitGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARaviCircuitGameMode();

	UPROPERTY(BlueprintReadOnly) TObjectPtr<ARCFighterCharacter> P1;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<ARCFighterCharacter> P2;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<ARCFightCameraActor> FightCamera;
	UPROPERTY(BlueprintReadOnly) ERCMatchMode MatchMode = ERCMatchMode::ArcadeCPU;
	UPROPERTY(BlueprintReadOnly) float RoundTime = 60.f;
	UPROPERTY(BlueprintReadOnly) int32 RoundNumber = 1;
	UPROPERTY(BlueprintReadOnly) int32 P1Rounds = 0;
	UPROPERTY(BlueprintReadOnly) int32 P2Rounds = 0;
	UPROPERTY(BlueprintReadOnly) bool bMatchActive = false;
	UPROPERTY(BlueprintReadOnly) bool bPausedMatch = false;
	UPROPERTY(BlueprintReadOnly) bool bInFrontEnd = true;
	UPROPERTY(BlueprintReadOnly) bool bAwaitingRematch = false;
	UPROPERTY(BlueprintReadOnly) bool bHitVfxEnabled = true;
	UPROPERTY(BlueprintReadOnly) bool bCameraShakeEnabled = true;
	UPROPERTY(BlueprintReadOnly) bool bHapticsEnabled = true;

	void SpawnWallSplat(const FVector& Location);
	void SetCameraShake(float Amount);
	void StartMatch(ERCMatchMode NewMode);
	FString HudLine() const;
	FString FrontEndLine() const;
	FString MatchResultLine() const;
	FString TrainingFrameLine() const;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<ARCHUD> RuntimeHud;
	TMap<FName, FRCFighterSpec> FighterSpecs;
	FName P1Id = "Zara";
	FName P2Id = "Hamza";
	float AnnouncementTimer = 0.f;
	FString Announcement;
	FString MatchResult;
	bool bTrainingDummyGuard = false;
	bool bTrainingDummyLowGuard = false;
	UPROPERTY() TObjectPtr<USoundBase> ImpactLightSound;
	UPROPERTY() TObjectPtr<USoundBase> ImpactHeavySound;
	UPROPERTY() TObjectPtr<USoundBase> BlockSound;
	UPROPERTY() TObjectPtr<USoundBase> SuperRiserSound;
	UPROPERTY() TObjectPtr<USoundBase> MenuTickSound;
	UPROPERTY() TObjectPtr<USoundBase> MenuConfirmSound;
	UPROPERTY() TObjectPtr<USoundBase> MenuBackSound;
	UPROPERTY() TObjectPtr<USoundBase> RoundStartSound;
	UPROPERTY() TObjectPtr<USoundBase> AnnouncerFightSound;
	UPROPERTY() TObjectPtr<USoundBase> AnnouncerKoSound;
	UPROPERTY() TObjectPtr<USoundBase> AnnouncerPerfectSound;
	UPROPERTY() TObjectPtr<USoundBase> KoHitSound;
	UPROPERTY() TObjectPtr<USoundBase> WallSplatSound;
	UPROPERTY() TObjectPtr<USoundBase> CounterHitSound;
	UPROPERTY() TObjectPtr<USoundBase> ThrowHitSound;
	UPROPERTY() TObjectPtr<USoundBase> RageReadySound;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ReactiveProps;

	void BuildArena();
	void LoadAudioAssets();
	void PlayMenuTick();
	void PlayMenuConfirm();
	void PlayMenuBack();
	void PlayCombatSound(const FRCMoveData& Move, bool bBlocked, const FVector& Location);
	void PlayHaptics(APlayerController* PC, float Low, float High, float Duration);
	void TriggerReactiveProps(const FVector& Location, float Radius);
	void SpawnFighters();
	void BeginRound();
	void EndRound(ARCFighterCharacter* Winner, const FString& Reason);
	void ReturnToFrontEnd();
	void CycleFighter(bool bFirstPlayer);
	void HandleFrontEndInput(APlayerController* PC);
	void HandlePostMatchInput(APlayerController* PC);
	void HandleTrainingInput(APlayerController* PC);
	void HandleSettingsInput(APlayerController* PC);
	void HandleHit(ARCFighterCharacter* Attacker, ARCFighterCharacter* Defender, const FRCMoveData& Move, bool bBlocked);
	void HandleSuper(ARCFighterCharacter* Fighter, const FRCMoveData& Move);
	void SpawnHitFX(const FVector& Location, bool bBlocked, bool bBig);
	UStaticMeshComponent* AddBlock(const TCHAR* Name, FVector Location, FVector Scale, FLinearColor Color, float Emission = 0.f);
};
