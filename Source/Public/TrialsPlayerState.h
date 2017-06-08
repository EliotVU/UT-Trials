#pragma once

#include "TrialsGameMode.h"
#include "TrialsObjectiveInfo.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
	GENERATED_UCLASS_BODY()

public:
    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjectiveInfo* ActiveObjectiveInfo;

    UPROPERTY(Replicated)
    float LastScoreObjectiveTimer;

    /* The player's record time for the current active objective. <= 0 == N/A */
    UPROPERTY(Replicated)
    float ObjectiveRecordTime;

    UFUNCTION(BlueprintCallable, Category = Objective)
    float GetObjectiveTimer() const
    {
        if (ActiveObjectiveInfo == nullptr) return -1;
        return RoundTime((bIsObjectiveTimerActive ? GetWorld()->RealTimeSeconds : ObjectiveEndTime) - ObjectiveStartTime);
    }

    UFUNCTION(BlueprintCallable, Category = Objective)
    bool IsObjectiveTimerActive() const
    {
        return bIsObjectiveTimerActive;
    }

    UFUNCTION(BlueprintCallable, Category = Objective)
    float GetObjectiveRemainingTime() const
    {
        return bIsObjectiveTimerActive 
            ? (ActiveObjectiveInfo ? ActiveObjectiveInfo->RecordTime : 0.00) - GetObjectiveTimer()
            : ObjectiveRecordTime;
    }

    void StartObjectiveTimer()
    {
        bIsObjectiveTimerActive = true;
        ObjectiveEndTime = 0.00;
        ObjectiveStartTime = GetWorld()->RealTimeSeconds;
        ForceNetUpdate();
    }

    float EndObjectiveTimer()
    {
        if (!bIsObjectiveTimerActive) 
            return LastScoreObjectiveTimer;

        ObjectiveEndTime = GetWorld()->RealTimeSeconds;
        LastScoreObjectiveTimer = GetObjectiveTimer();
        bIsObjectiveTimerActive = false;
        ForceNetUpdate();
        return LastScoreObjectiveTimer;
    }

    void SetObjective(ATrialsObjectiveInfo* objectiveInfo)
    {
        if (objectiveInfo == ActiveObjectiveInfo)
        {
            return;
        }
        ActiveObjectiveInfo = objectiveInfo;
        ForceNetUpdate();
    }

    float RoundTime(float time) const
    {
        return roundf(time*100.0)/100.0;
    }

    UFUNCTION(BlueprintCallable, Category = HUD)
    FText FormatTime(float value) const
    {
        float seconds = fabs(value);
        int32 minutes = (int32)seconds/60;
        int32 hours = minutes/60;
        seconds = seconds - (minutes*60.f);
        minutes = minutes - (hours*60);

        FString secondsString = FString::Printf(TEXT("%f"), seconds);
        secondsString = secondsString.Left(secondsString.Find(".") + 3).Append("s");
        if (minutes != 0 && seconds < 10.f) 
            secondsString = TEXT("0") + secondsString;

        FString minutesString = (hours != 0 && minutes < 10) 
            ? TEXT("0") + minutes 
            : FString::FromInt(minutes);

        FString hoursString = FString::FromInt(hours);

        FString output;
        if (hours != 0)
            output = hoursString + TEXT("h ");

        if (minutes != 0)
            output += minutesString + TEXT("m ");

        output = value < 0 
            ? TEXT("-") + output + secondsString 
            : output + secondsString;
        return FText::FromString(output);
    }

private:
    float ObjectiveStartTime;
    float ObjectiveEndTime;
    bool bIsObjectiveTimerActive;
};