#include "Trials.h"
#include "TrialsObjectiveCompleteMessage.h"

#include "TrialsPlayerState.h"
#include "TrialsObjective.h"

UTrialsObjectiveCompleteMessage::UTrialsObjectiveCompleteMessage(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Lifetime = 3.0f;
    MessageArea = FName(TEXT("Announcements"));
    MessageSlot = FName(TEXT("MajorRewardMessage"));

    bIsConsoleMessage = false;
    bIsStatusAnnouncement = false;

    ObjectiveCompletedText = NSLOCTEXT("Trials", "ObjectiveCompleted", "{Player1} completed {Title} in {Time} seconds!");
}

FText UTrialsObjectiveCompleteMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    ATrialsPlayerState* ScorerPS = Cast<ATrialsPlayerState>(RelatedPlayerState_1);
    ATrialsObjectiveInfo* ScoredObjInfo = Cast<ATrialsObjectiveInfo>(OptionalObject);

    FFormatNamedArguments Args;
    Args.Add("Title", ScoredObjInfo->Objective->Title);
    Args.Add("Player1", FText::FromString(ScorerPS->PlayerName));

    FText message;

    if (Switch == 0)
    {
        message = ObjectiveCompletedText;
        Args.Add("Time", ScorerPS->FormatTime(ScorerPS->LastScoreObjectiveTimer));
    }
    else if (Switch == 1)
    {
        message = NSLOCTEXT("Trials", "RecordFail", "{Player1} failed {Title} by {Time} seconds!");
        Args.Add("Time", ScorerPS->FormatTime(ScoredObjInfo->RecordTime - ScorerPS->LastScoreObjectiveTimer));
    }

    return FText::Format(message, Args);
}

FName UTrialsObjectiveCompleteMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
    switch (Switch)
    {
    case 0: return TEXT("F_Score"); break;
    case 1: return TEXT("F_Assist"); break;
    }
    return NAME_None;
}

void UTrialsObjectiveCompleteMessage::PrecacheAnnouncements_Implementation(class UUTAnnouncer* Announcer) const
{
    for (int32 i = 0; i < 2; i++)
    {
        FName SoundName = GetAnnouncementName(i, NULL, NULL, NULL);
        if (SoundName != NAME_None)
        {
            Announcer->PrecacheAnnouncement(SoundName);
        }
    }
}

float UTrialsObjectiveCompleteMessage::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
    return 1.f;
}

bool UTrialsObjectiveCompleteMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
    return true;
}
