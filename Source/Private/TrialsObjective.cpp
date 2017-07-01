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

void ATrialsObjective::BeginPlay()
{
    Super::BeginPlay();
    auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
    if (GS)
    {
        GS->AddObjective(this);
    }
}