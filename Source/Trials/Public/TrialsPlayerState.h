#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsTimerState.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
    GENERATED_UCLASS_BODY()

    FString PlayerNetId;
    FString ObjectiveRecordId; // The player's current objective's record id. Required to fetch ghost data.

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjective* ActiveObjective;

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsTimerState* TimerState;

    /* The player's record time for the current active objective. <= 0 == N/A */
    UPROPERTY(Replicated, BlueprintReadOnly)
    float ObjectiveRecordTime;

    /**
     * List of objective instances that have been unlocked in the current map. Objectives that were never locked are not included!
     */
    UPROPERTY(ReplicatedUsing = OnRep_UnlockedObjectives, BlueprintReadOnly)
    TArray<ATrialsObjective*> UnlockedObjectives;

    void RegisterUnlockedObjective(ATrialsObjective* Objective);

    float StartObjective() const;
    float EndObjective() const;
    void SetActiveObjective(ATrialsObjective* Obj);

    void UpdateRecordTime(float Time);

    UFUNCTION(BlueprintCallable, Category = HUD)
    static FText FormatTime(const float Value)
    {
        return ATrialsTimerState::FormatTime(Value);
    }

    void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

protected:
    UFUNCTION()
    void OnRep_UnlockedObjectives();
};