#include "Trials.h"
#include "TrialsGameState.h"

#include "TrialsObjective.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bAlwaysRelevant = true;
}

void ATrialsObjective::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
    if (GS != nullptr)
    {
        GS->AddObjective(this);
    }
}