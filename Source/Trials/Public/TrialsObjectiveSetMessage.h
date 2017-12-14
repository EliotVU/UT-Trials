#pragma once

#include "UTLocalMessage.h"

#include "TrialsObjectiveSetMessage.generated.h"

UCLASS()
class TRIALS_API UTrialsObjectiveSetMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    FText StartedText;
    FText LeftText;

    virtual void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    virtual FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
};
