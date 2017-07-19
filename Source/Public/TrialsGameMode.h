#pragma once

#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsAPI.h"

#include "TrialsGameMode.generated.h"

UCLASS(Meta = (ChildCanTick), Config = Trials)
class ATrialsGameMode : public AUTGameMode
{
    GENERATED_UCLASS_BODY()

    ATrialsAPI* RecordsAPI;

    UPROPERTY(Config)
    FString RecordsBaseURL;

    UPROPERTY(Config)
    FString RecordsAPIToken;

    void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void APIReady();

    void BeginPlay() override;
    void PostLogin(APlayerController* NewPlayer) override;
    void SetPlayerDefaults(APawn* PlayerPawn) override;
    bool AllowSuicideBy(AUTPlayerController* PC) override;

    AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

    void DiscardInventory(APawn* Other, AController* Killer) override;

    virtual void ScoreTrialObjective(ATrialsObjective* Obj, float Timer, AUTPlayerController* PC);

private:
    FMapInfo CurrentMapInfo;
    bool bAPIAuthenticated;
};