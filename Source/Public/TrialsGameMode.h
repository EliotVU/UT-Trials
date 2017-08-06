#pragma once

#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsAPI.h"

#include "TrialsGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectiveCompleted, ATrialsObjective*, Objective, AUTPlayerController*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FObjectiveRecordSet, ATrialsObjective*, Objective, AUTPlayerController*, Player, float, Time, float, TimeDifference, bool, IsTopRecord);

UCLASS(Meta = (ChildCanTick), Config = Trials, Abstract)
class ATrialsGameMode : public AUTGameMode
{
    GENERATED_UCLASS_BODY()

    TAssetSubclassOf<AUTCharacter> GhostObject;

    UPROPERTY()
    TSubclassOf<AUTCharacter> GhostClass;

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

    virtual void ScoreTrialObjective(ATrialsObjective* Obj, float Timer, AUTPlayerController* Instigator);

    UPROPERTY(BlueprintAssignable, Category = Objective, BlueprintAuthorityOnly)
    FObjectiveCompleted OnObjectiveCompleted;

    UPROPERTY(BlueprintAssignable, Category = Objective, BlueprintAuthorityOnly)
    FObjectiveRecordSet OnObjectiveRecordSet;

private:
    FMapInfo CurrentMapInfo;
    bool bAPIAuthenticated;
};