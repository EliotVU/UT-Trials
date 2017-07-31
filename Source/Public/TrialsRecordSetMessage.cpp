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
        // Top Record
    case 0:
        return ATrialsTimerState::LeadColor;

        // Personal Record
        // FIXME: case 3 may render positive even if the time is negative (first personal record)
    case 3: case 4:
        return ATrialsTimerState::PositiveColor;

        // Failed
    case 1: case 2:
        return ATrialsTimerState::NegativeColor;

    }
    return Super::GetMessageColor_Implementation(Switch);
}

void UTrialsRecordSetMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* TimerState = Cast<ATrialsTimerState>(OptionalObject);
    if (TimerState != nullptr)
    {
        float TimeDiff = TimerState->GetRemainingTime();
        FString TimeDiffStr = TimerState->FormatTime(TimeDiff).ToString();
        if (TimeDiff > 0.0)
        {
            TimeDiffStr = "+" + TimeDiffStr;
        }
        Args.Add("TimeDiff", FText::FromString(TimeDiffStr));
    }
}
