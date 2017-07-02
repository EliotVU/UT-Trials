#include "Trials.h"
#include "TrialsGameState.h"

#include "TrialsObjective.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;
}

void ATrialsObjective::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
    if (GS)
    {
        GS->AddObjective(this);
    }
}