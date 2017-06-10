#include "Trials.h"
#include "TrialsPlayerState.h"
#include "TrialsObjective.h"
#include "TrialsObjectiveCompleteMessage.h"

UTrialsObjectiveCompleteMessage::UTrialsObjectiveCompleteMessage(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MessageArea = FName(TEXT("Announcements"));
    MessageSlot = FName(TEXT("MajorRewardMessage"));

    Lifetime = 3.0f;
    AnnouncementDelay = 1.f;

    bIsPartiallyUnique = true;
    bIsConsoleMessage = false;
    bIsStatusAnnouncement = false;

    // TODO: Split messages by PlayerCompletedText and ObjectiveCompletedText where objective messages are for all time records and player for personal records!
    ObjectiveCompletedText = NSLOCTEXT("Trials", "RecordSet", "{Player1Name} completed {Title} in {Time}!");
    ObjectiveRecordTiedText = NSLOCTEXT("Trials", "RecordTied", "{Player1Name} completed {Title} with a tie to {Time}!");
    ObjectiveRecordFailText = NSLOCTEXT("Trials", "RecordFail", "{Player1Name} failed {Title} by {Time}!");
    ObjectiveRecordFirstText = NSLOCTEXT("Trials", "RecordFirst", "{Player1Name} completed {Title} in a record of {Time}!");
    ObjectiveRecordNewText = NSLOCTEXT("Trials", "RecordNew", "{Player1Name} completed {Title} with a new time of {Time}!");

    static ConstructorHelpers::FObjectFinder<USoundBase> RecordSetSoundFinder(TEXT("SoundCue'/Trials/RecordSet.RecordSet'"));
    RecordSetSound = RecordSetSoundFinder.Object;

    static ConstructorHelpers::FObjectFinder<USoundBase> RecordFailedSoundFinder(TEXT("SoundCue'/Trials/RecordFailed.RecordFailed'"));
    RecordFailedSound = RecordFailedSoundFinder.Object;
}

FText UTrialsObjectiveCompleteMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    switch (Switch)
    {
    case 0: return ObjectiveCompletedText;
    case 1: return ObjectiveRecordTiedText;
    case 2: return ObjectiveRecordFailText;
    case 3: return ObjectiveRecordFirstText;
    case 4: return ObjectiveRecordNewText;
    }
    return FText::GetEmpty();
}

void UTrialsObjectiveCompleteMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* ScorerPS = Cast<ATrialsPlayerState>(RelatedPlayerState_1);
    auto* ScoredObjInfo = Cast<ATrialsObjectiveInfo>(OptionalObject);

    float Time = ScorerPS->LastScoreObjectiveTimer;
    switch (Switch)
    {
        // Relative to obj's best time.
    case 2:
        Time = ScoredObjInfo->RecordTime - Time;
        break;
    }
    
    Args.Add("Player1Name", FText::FromString(bTargetsPlayerState1 ? "You" : ScorerPS->PlayerName));
    Args.Add("Title", ScoredObjInfo->Title);
    Args.Add("Time", ScorerPS->FormatTime(Time));
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

FName UTrialsObjectiveCompleteMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
    switch (Switch)
    {
    case 0: return TEXT("HatTrick"); break;
    }
    return NAME_None;
}

void UTrialsObjectiveCompleteMessage::PrecacheAnnouncements_Implementation(class UUTAnnouncer* Announcer) const
{
    for (int32 i = 0; i < 4; i++)
    {
        Announcer->PrecacheAnnouncement(GetAnnouncementName(i, nullptr, nullptr, nullptr));
    }
}

float UTrialsObjectiveCompleteMessage::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
    return 1.f;
}

bool UTrialsObjectiveCompleteMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
    // TODO: Add anouncement after record sound cues.
    return false;
}
