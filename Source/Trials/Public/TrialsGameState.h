#pragma once

#include "TrialsObjective.h"
#include "TrialsGameMode.h"

#include "TrialsGameState.generated.h"

UCLASS()
class ATrialsGameState : public AUTGameState
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly, Category = Trials, DisplayName = "Objective Targets")
    TArray<ATrialsObjectiveTarget*> Targets;

    UPROPERTY(Replicated)
    uint32 bAPIAuthenticated : 1;

    void AddTarget(ATrialsObjectiveTarget* Obj)
    {
        Targets.AddUnique(Obj);
    }

    void RemoveTarget(ATrialsObjectiveTarget* Obj)
    {
        Targets.Remove(Obj);
    }

    bool AllowMinimapFor(AUTPlayerState* PS) override;
    bool OnSameTeam(const AActor* Actor1, const AActor* Actor2) override;
};
