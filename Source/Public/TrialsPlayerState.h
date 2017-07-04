#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsTimerState.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
    GENERATED_UCLASS_BODY()

    FString PlayerNetId;

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjectiveInfo* ActiveObjectiveInfo;

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsTimerState* TimerState;

    /* The player's record time for the current active objective. <= 0 == N/A */
    UPROPERTY(Replicated, BlueprintReadOnly)
    float ObjectiveRecordTime;

    /**
     * List of objective instances that have been unlocked in the current map. Objectives that were never locked are not included!
     * TODO: Fetch unlocked objectives from API.
     */
    UPROPERTY(ReplicatedUsing = OnRep_UnlockedObjectives, BlueprintReadOnly)
    TArray<ATrialsObjectiveInfo*> UnlockedObjectives;

    void RegisterUnlockedObjective(ATrialsObjectiveInfo* Objective);

    float StartObjective() const;
    float EndObjective() const;
    void SetObjective(ATrialsObjectiveInfo* Obj);

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