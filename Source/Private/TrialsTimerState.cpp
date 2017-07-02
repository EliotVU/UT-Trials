#include "Trials.h"

#include "TrialsTimerState.h"
#include "TrialsPlayerState.h"
#include "UnrealNetwork.h"

ATrialsTimerState::ATrialsTimerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer
        .DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
    SetActorTickEnabled(true);
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.SetTickFunctionEnable(true);

    SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
    bReplicates = true;
    bAlwaysRelevant = true;
    bReplicateMovement = false;

    NetUpdateFrequency = 1;
    NetPriority = 1.0;

    State = TS_Idle;
}

void ATrialsTimerState::OnRep_State()
{
    switch (State)
    {
    case TS_Active:
        StartTimer();
        break;

    case TS_Complete:
        EndTimer();
        break;

    case TS_Idle:
        StopTimer();
        break;
    }
}

float ATrialsTimerState::GetTimer() const
{
    float CurrentTime = State == TS_Active ? GetWorld()->RealTimeSeconds : EndTime;
    return RoundTime(CurrentTime - StartTime);
}

float ATrialsTimerState::GetRemainingTime() const
{
    return GetRecordTime() - GetTimer();
}

float ATrialsTimerState::GetRecordTime() const
{
    if (State == TS_Complete)
    {
        return EndRecordTime;
    }
    return OwnerRecordTime != 0.0 ? OwnerRecordTime : Objective != nullptr ? Objective->RecordTime : 0.0;
}

float ATrialsTimerState::StartTimer()
{
    // Resume?
    if (State == TS_Idle && EndTime != 0.0)
    {
        StartTime = EndTime;
    }
    else
    {
        StartTime = GetWorld()->RealTimeSeconds;
    }
    EndTime = 0.00;
    State = TS_Active;
    ForceNetUpdate();
    return RoundTime(StartTime);
}

float ATrialsTimerState::StopTimer()
{
    State = TS_Idle;
    EndTime = GetWorld()->RealTimeSeconds;
    ForceNetUpdate();
    return RoundTime(EndTime);
}

float ATrialsTimerState::EndTimer()
{
    EndRecordTime = GetRecordTime();

    State = TS_Complete;
    EndTime = GetWorld()->RealTimeSeconds;
    ForceNetUpdate();
    return RoundTime(EndTime - StartTime);
}

float ATrialsTimerState::RoundTime(const float Seconds)
{
    return roundf(Seconds * 1000.0) / 1000.0;
}

FText ATrialsTimerState::FormatTime(const float Value)
{
    float seconds = fabs(Value);
    int32 minutes = static_cast<int32>(seconds) / 60;
    int32 hours = minutes / 60;
    seconds = seconds - (minutes * 60.f);
    minutes = minutes - (hours * 60);

    FString secondsString = FString::Printf(TEXT("%f"), seconds);
    secondsString = secondsString.Left(secondsString.Find(".") + 4).Append("s");
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

    output = Value < 0
                 ? TEXT("-") + output + secondsString
                 : output + secondsString;
    return FText::FromString(output);
}

FLinearColor ATrialsTimerState::GetTimerColor(const float Timer)
{
    float Fade = FMath::Fmod(fabs(Timer), 1.0);
    FLinearColor TimerColor = FLinearColor::LerpUsingHSV(Timer > 0.0 ? FLinearColor::Green : FLinearColor::Red, FLinearColor::White, 1.0 - Fade);
    return TimerColor;
}

void ATrialsTimerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsTimerState, Objective);
    DOREPLIFETIME(ATrialsTimerState, State);
    DOREPLIFETIME(ATrialsTimerState, OwnerRecordTime);
}