#include "Trials.h"
#include "TrialsObjective.h"
#include "TrialsObjectiveSetMessage.h"

UTrialsObjectiveSetMessage::UTrialsObjectiveSetMessage(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MessageArea = FName(TEXT("Announcements"));
    MessageSlot = FName(TEXT("GameMessages"));

    Lifetime = 1.0f;

    bIsUnique = true;
    bIsConsoleMessage = false;
    bIsStatusAnnouncement = false;

    StartedText = NSLOCTEXT("Trials", "ObjectiveStarted", "{Player1Name} started objective {Title}!");
    LeftText = NSLOCTEXT("Trials", "ObjectiveLeft", "{Player1Name} left objective {Title}!");
}

FText UTrialsObjectiveSetMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    switch (Switch)
    {
    case 0: return StartedText;
    case 1: return LeftText;
    }
    return FText::GetEmpty();
}

void UTrialsObjectiveSetMessage::GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
    auto* ObjInfo = Cast<ATrialsObjectiveInfo>(OptionalObject);
    Args.Add("Player1Name", FText::FromString(bTargetsPlayerState1 ? "You" : RelatedPlayerState_1->PlayerName));
    Args.Add("Title", ObjInfo->Title);
}
