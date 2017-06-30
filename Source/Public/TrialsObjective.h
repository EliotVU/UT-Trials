#pragma once

#include "TrialsObjectiveInfo.h"

#include "TrialsObjective.generated.h"

UCLASS(abstract, Blueprintable)
class TRIALS_API ATrialsObjective : public AActor
{
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = Objective)
    ATrialsObjectiveInfo* ObjectiveInfo;
 };
