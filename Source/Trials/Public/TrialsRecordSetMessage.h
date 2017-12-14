#pragma once

#include "UTLocalMessage.h"

#include "TrialsRecordSetMessage.generated.h"

UCLASS()
class TRIALS_API UTrialsRecordSetMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    FLinearColor GetMessageColor_Implementation(int32 MessageIndex) const override;
};

UCLASS()
class TRIALS_API UTrialsRecordSetRemoteMessage : public UUTLocalMessage
{
    GENERATED_UCLASS_BODY()

    void GetArgs(FFormatNamedArguments& Args, int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    FText GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const override;
    FLinearColor GetMessageColor_Implementation(int32 MessageIndex) const override;
};
