#pragma once

#include "UTAnnouncer.h"
#include "UTLocalMessage.h"
#include "TrialsObjectiveCompleteMessage.generated.h"

UCLASS()
class TRIALS_API UTrialsObjectiveCompleteMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    FText ObjectiveCompletedText;
    FText ObjectiveRecordTiedText;
    FText ObjectiveRecordFailText;
    FText ObjectiveRecordFirstText;
    FText ObjectiveRecordNewText;

    virtual void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    virtual FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const APlayerState* RelatedPlayerState_1, const APlayerState* RelatedPlayerState_2) const override;
    virtual void PrecacheAnnouncements_Implementation(class UUTAnnouncer* Announcer) const override;
    virtual float GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const override;
    virtual bool ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const override;
};
