#pragma once

#include "UTLocalMessage.h"

#include "TrialsObjectiveCompleteMessage.generated.h"

UCLASS()
class TRIALS_API UTrialsObjectiveCompleteMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    FText ObjectiveCompletedPrefix;
    FText ObjectiveCompletedText;
    FText ObjectiveCompletedPostfix;

    USoundBase* RecordSetSound;
    USoundBase* RecordFailedSound;

    void GetEmphasisText(FText& PrefixText, FText& EmphasisText, FText& PostfixText, FLinearColor& EmphasisColor, int32 Switch, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const override;
    FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    void ClientReceive(const FClientReceiveData& ClientData) const override;
    bool ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const override;
};
