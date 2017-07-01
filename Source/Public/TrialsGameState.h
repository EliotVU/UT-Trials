#pragma once

#include "TrialsObjective.h"
#include "TrialsGameMode.h"

#include "TrialsGameState.generated.h"

UCLASS()
class ATrialsGameState : public AUTGameState
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(BlueprintReadOnly, Category = Trials, DisplayName = "Objective Targets")
    TArray<ATrialsObjective*> Objectives;

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool AllowMinimapFor(AUTPlayerState* PS) override;
    bool OnSameTeam(const AActor* Actor1, const AActor* Actor2) override;

    void AddObjective(ATrialsObjective* Obj)
    {
        Objectives.Add(Obj);
    }

    void RemoveObjective(ATrialsObjective* Obj)
    {
        Objectives.Remove(Obj);
    }
};
