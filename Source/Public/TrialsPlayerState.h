#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsTimerState.h"
#include "TrialsAPI.h"

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
    UPROPERTY(Replicated)
    float ObjectiveRecordTime;

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
};