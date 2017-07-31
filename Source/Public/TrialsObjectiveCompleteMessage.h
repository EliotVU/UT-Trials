#pragma once

#include "UTLocalMessage.h"

#include "TrialsObjectiveCompleteMessage.generated.h"

UCLASS()
class TRIALS_API UTrialsObjectiveCompleteMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    FText ObjectiveCompletedText;

    USoundBase* RecordSetSound;
    USoundBase* RecordFailedSound;

    void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    void ClientReceive(const FClientReceiveData& ClientData) const override;
    FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    bool ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const override;
};
