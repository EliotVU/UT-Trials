#include "Trials.h"
#include "TrialsPlayerState.h"

#include "TrialsObjectiveCompleteMessage.h"

UTrialsObjectiveCompleteMessage::UTrialsObjectiveCompleteMessage(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MessageArea = FName(TEXT("Announcements"));
    MessageSlot = FName(TEXT("MajorRewardMessage"));

    Lifetime = 3.0f;
    AnnouncementDelay = 1.f;
    bIsConsoleMessage = true;

    ObjectiveCompletedText = NSLOCTEXT("Trials", "RecordSet", "{Player1Name} completed {Title} in {Time}");

    static ConstructorHelpers::FObjectFinder<USoundBase> RecordSetSoundFinder(TEXT("SoundCue'/Trials/RecordSet.RecordSet'"));
    RecordSetSound = RecordSetSoundFinder.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> RecordFailedSoundFinder(TEXT("SoundCue'/Trials/RecordFailed.RecordFailed'"));
    RecordFailedSound = RecordFailedSoundFinder.Object;
}

FText UTrialsObjectiveCompleteMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    return ObjectiveCompletedText;
}

void UTrialsObjectiveCompleteMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* TimerState = Cast<ATrialsTimerState>(OptionalObject);
    if (TimerState != nullptr)
    {
        Args.Add("Title", TimerState->Objective->Title);
        float Time = TimerState->GetTimer();
        Args.Add("Time", TimerState->FormatTime(Time));
    }
    
    auto* ScorerPS = Cast<ATrialsPlayerState>(RelatedPlayerState_1);
    if (ScorerPS != nullptr)
    {
        Args.Add("Player1Name", FText::FromString(bTargetsPlayerState1 ? "You" : ScorerPS->PlayerName));
    }
}

void UTrialsObjectiveCompleteMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
    Super::ClientReceive(ClientData);

    USoundBase* AnnouncementSound = RecordSetSound;
    switch (ClientData.MessageIndex)
    {
    case 1: case 2:
        AnnouncementSound = RecordFailedSound;
        break;
    }

    if (AnnouncementSound == nullptr)
    {
        return;
    }

    auto* PC = Cast<AUTPlayerController>(ClientData.LocalPC);
    if (PC == nullptr)
    {
        return;
    }
    PC->UTClientPlaySound(AnnouncementSound);
}

bool UTrialsObjectiveCompleteMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
    return false;
}
