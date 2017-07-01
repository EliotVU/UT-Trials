#pragma once

#include "TrialsGameMode.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsAPI.h"

#include "TrialsPlayerState.generated.h"

UCLASS()
class ATrialsPlayerState : public AUTPlayerState
{
    GENERATED_UCLASS_BODY()

    FPlayerInfo PlayerInfo;
    FRecordInfo ObjRecordInfo;

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjectiveInfo* ActiveObjectiveInfo;

    UPROPERTY(Replicated)
    float LastScoreObjectiveTimer;

    /* The player's record time for the current active objective. <= 0 == N/A */
    UPROPERTY(Replicated)
    float ObjectiveRecordTime;

    void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);

        GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("TIMER %f"), GetObjectiveTimer()));
        GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("REM %f"), GetObjectiveRemainingTime()));
        GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("Rec %f"), GetRacingRecordTime()));
    }

    UFUNCTION(BlueprintCallable, Category = Objective)
    float GetObjectiveTimer() const
    {
        if (ActiveObjectiveInfo == nullptr) return 0.0;
        return RoundTime((bIsObjectiveTimerActive ? GetWorld()->RealTimeSeconds : ObjectiveEndTime) - ObjectiveStartTime);
    }

    UFUNCTION(BlueprintCallable, Category = Objective)
    bool IsObjectiveTimerActive() const
    {
        return bIsObjectiveTimerActive;
    }

    UFUNCTION(BlueprintCallable, Category = Objective)
    float GetObjectiveRemainingTime(const bool IsActive = false) const
    {
        return IsObjectiveTimerActive() || IsActive
            ? GetRacingRecordTime() - GetObjectiveTimer()
            : GetRacingRecordTime();
    }

    // Always race our own record time when possible.
    float GetRacingRecordTime() const
    {
        return 
            ObjectiveRecordTime > 0.0 ? ObjectiveRecordTime : 
            (ActiveObjectiveInfo != nullptr ? ActiveObjectiveInfo->RecordTime : 0.0);
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
        bIsObjectiveTimerActive = false;
        ForceNetUpdate();
        return RoundTime(ObjectiveEndTime - ObjectiveStartTime);
    }

    void SetObjective(ATrialsObjectiveInfo* Obj)
    {
        if (Obj == ActiveObjectiveInfo)
        {
            return;
        }
        ObjectiveRecordTime = 0.0;
        LastScoreObjectiveTimer = 0.0;
        ActiveObjectiveInfo = Obj;

        if (ActiveObjectiveInfo != nullptr)
        {
            // Note: assumes that the activated objective has fetched its info!
            auto _ObjId = ActiveObjectiveInfo->ObjInfo._id;
            auto _PlayerId = PlayerInfo._id;

            auto* API = GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
            API->Fetch(TEXT("api/recs/") + FGenericPlatformHttp::UrlEncode(_ObjId) + TEXT("/") + FGenericPlatformHttp::UrlEncode(_PlayerId),
                [this, Obj](const FAPIResult& Data) {
                    // Player may have switched active objective during this request.
                    if (ActiveObjectiveInfo != Obj)
                    {
                        return;
                    }
                    ATrialsAPI::FromJSON(Data, &ObjRecordInfo);
                    ObjectiveRecordTime = RoundTime(ObjRecordInfo.Value);
                    ForceNetUpdate();
                }
            );
        }
    }

    static float RoundTime(const float Seconds)
    {
        return roundf(Seconds*100.0)/100.0;
    }

    UFUNCTION(BlueprintCallable, Category = HUD)
    static FText FormatTime(const float value)
    {
        float seconds = fabs(value);
        int32 minutes = static_cast<int32>(seconds)/60;
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

    UFUNCTION(BlueprintCallable, Category = HUD)
    static FLinearColor GetTimerColor(const float Timer)
    {
        float Fade = FMath::Fmod(fabs(Timer), 1.0);
        FLinearColor TimerColor = FLinearColor::LerpUsingHSV(Timer > 0.0 ? FLinearColor::Green : FLinearColor::Red, FLinearColor::White, 1.0 - Fade);
        return TimerColor;
    }

private:
    float ObjectiveStartTime;
    float ObjectiveEndTime;
    bool bIsObjectiveTimerActive;
};