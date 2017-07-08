#include "Trials.h"
#include "TrialsGameState.h"

#include "TrialsObjective.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ATrialsObjective::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (Role == ROLE_Authority)
    {
        auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
        if (GS != nullptr)
        {
            GS->AddTarget(this);
        }
    }
}

void ATrialsObjective::Destroyed()
{
    auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
    if (GS != nullptr)
    {
        GS->RemoveTarget(this);
    }
}
