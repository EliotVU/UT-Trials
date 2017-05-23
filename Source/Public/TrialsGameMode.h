#pragma once

#include "Trials.h"
#include "TrialsObjectiveInfo.h"

#include "TrialsGameMode.generated.h"

UCLASS(Blueprintable, Meta = (ChildCanTick), Config = Trials)
class ATrialsGameMode : public AUTGameMode
{
	GENERATED_UCLASS_BODY()

public:
    bool AllowSuicideBy(AUTPlayerController* PC);
    virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
    virtual void ScoreTrialObjective(AUTPlayerController* PC, ATrialsObjectiveInfo* objInfo);
};