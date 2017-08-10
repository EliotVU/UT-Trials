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

    ObjectiveCompletedPrefix = NSLOCTEXT("Trials", "ObjectiveCompletedPrefix", "{Player1Name} completed ");
    ObjectiveCompletedText = NSLOCTEXT("Trials", "ObjectiveCompletedText", "{Title}");
    ObjectiveCompletedPostfix = NSLOCTEXT("Trials", "ObjectiveCompletedPostfix", " in {Time}{Stars}");

    static ConstructorHelpers::FObjectFinder<USoundBase> RecordSetSoundFinder(TEXT("SoundCue'/Trials/RecordSet.RecordSet'"));
    RecordSetSound = RecordSetSoundFinder.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> RecordFailedSoundFinder(TEXT("SoundCue'/Trials/RecordFailed.RecordFailed'"));
    RecordFailedSound = RecordFailedSoundFinder.Object;
}

void UTrialsObjectiveCompleteMessage::GetEmphasisText(FText& PrefixText, FText& EmphasisText, FText& PostfixText, FLinearColor& EmphasisColor, int32 Switch, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const
{
    bool bTargetsPlayerState1 = RelatedPlayerState_1 && RelatedPlayerState_1->GetOwner() && static_cast<APlayerController*>(RelatedPlayerState_1->GetOwner())->Player;
    FFormatNamedArguments Args;
    GetArgs(Args, Switch, bTargetsPlayerState1, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
    PrefixText = FText::Format(ObjectiveCompletedPrefix, Args);
    EmphasisText = FText::Format(ObjectiveCompletedText, Args);
    PostfixText = FText::Format(ObjectiveCompletedPostfix, Args);
    EmphasisColor = ATrialsTimerState::IdleColor;
    //Super::GetEmphasisText(PrefixText, EmphasisText, PostfixText, EmphasisColor, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

FText UTrialsObjectiveCompleteMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    return BuildEmphasisText(Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

void UTrialsObjectiveCompleteMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* TimerState = Cast<ATrialsTimerState>(OptionalObject);
    if (TimerState != nullptr)
    {
        Args.Add("Title", TimerState->Objective->Title);
        float Time = TimerState->GetTimer();
        Args.Add("Time", TimerState->FormatTime(Time));

        FText Stars = FText::FromString(FString().Append(TEXT("***"), TimerState->Objective->CalcStarsCount(Time)));
        Args.Add("Stars", Stars);
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
