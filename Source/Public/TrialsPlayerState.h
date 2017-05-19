#pragma once

#include "TrialsGameMode.h"
#include "TrialsObjectiveInfo.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
	GENERATED_UCLASS_BODY()

    float ObjectiveStartTime;
    float ObjectiveEndTime;
    bool bIsObjectiveTimerActive;

public:
    UPROPERTY(Replicated)
    ATrialsObjectiveInfo* ActiveObjectiveInfo;

    UPROPERTY(Replicated)
    float LastScoreObjectiveTimer;

    void StartObjectiveTimer()
    {
        bIsObjectiveTimerActive = true;
        ObjectiveEndTime = 0.00;
        ObjectiveStartTime = GetWorld()->RealTimeSeconds;
    }

    float EndObjectiveTimer()
    {
        ObjectiveEndTime = GetWorld()->RealTimeSeconds;
        LastScoreObjectiveTimer = GetObjectiveTimer();
        NetUpdateTime = GetWorld()->TimeSeconds - 1;
        bIsObjectiveTimerActive = false;
        return LastScoreObjectiveTimer;
    }

    float GetObjectiveTimer()
    {
        if (ActiveObjectiveInfo == nullptr) return -1;
        return RoundTime((bIsObjectiveTimerActive ? GetWorld()->RealTimeSeconds : ObjectiveEndTime) - ObjectiveStartTime);
    }

    void SetObjective(ATrialsObjectiveInfo* objectiveInfo)
    {
        if (objectiveInfo->Objective == nullptr)
        {
            UE_LOG(UT, Warning, TEXT("Can't activate objective, The objective info has no reference to an objective!"));
            return;
        }
        ActiveObjectiveInfo = objectiveInfo;
    }

    float RoundTime(float time)
    {
        return roundf(time * 100.0) / 100.0;
    }

    FText FormatTime(float value)
    {
        float seconds = fabs(value);
        int32 minutes = (int32)seconds / 60;
        int32 hours = minutes / 60;
        seconds = seconds - (minutes * 60);
        minutes = minutes - (hours * 60);

        FString hourString, minuteString, secondString, output;
        if (seconds < 10) secondString = TEXT("0") + FString::SanitizeFloat(seconds); else secondString = FString::SanitizeFloat(seconds);
        if (minutes < 10) minuteString = TEXT("0") + minutes; else minuteString = FString::FromInt(minutes);
        if (hours < 10) hourString = TEXT("0") + hours; else hourString = FString::FromInt(hours);

        if (hours != 0)
            output = hourString + TEXT(":");

        if (minutes != 0)
            output += minuteString + TEXT(":");

        output = value < 0 ? TEXT("-") + output + secondString : output + secondString;
        return FText::FromString(output);
    }
};