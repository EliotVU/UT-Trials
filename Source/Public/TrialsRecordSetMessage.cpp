#include "Trials.h"
#include "TrialsPlayerState.h"

#include "TrialsRecordSetMessage.h"

UTrialsRecordSetMessage::UTrialsRecordSetMessage(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MessageArea = FName(TEXT("Announcements"));
    MessageSlot = FName(TEXT("DeathMessage"));

    Lifetime = 3.0f;
    bIsConsoleMessage = true;

    FontSizeIndex = 2;
}

FText UTrialsRecordSetMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    return FText::FromString(TEXT("{TimeDiff}"));
}

FLinearColor UTrialsRecordSetMessage::GetMessageColor_Implementation(int32 Switch) const
{
    switch (Switch)
    {
        // Top record
    case 0:
        return ATrialsTimerState::LeadColor;

        // top or personal record
    case 3: 
    case 4:
        return ATrialsTimerState::PositiveColor;

        // tie, top or personal
    case 1: 
        return ATrialsTimerState::TieColor;
        
        // Negative, top and personal.
    case 2:
        return ATrialsTimerState::NegativeColor;

    }
    return Super::GetMessageColor_Implementation(Switch);
}

void UTrialsRecordSetMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* TimerState = Cast<ATrialsTimerState>(OptionalObject);
    if (TimerState != nullptr)
    {
        bool bShouldCountUp = (Switch == 3 || TimerState->GetRecordTime() == 0.00);
        float TimeDiff = bShouldCountUp
            ? TimerState->GetTimer() 
            : TimerState->GetRemainingTime();
        FString TimeDiffStr = TimerState->FormatTime(TimeDiff).ToString();
        if (TimeDiff > 0.0 && !bShouldCountUp)
        {
            TimeDiffStr = "+" + TimeDiffStr;
        }
        Args.Add("TimeDiff", FText::FromString(TimeDiffStr));
    }
}
