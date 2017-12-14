#include "Trials.h"
#include "TrialsGameState.h"

#include "TrialsObjective.h"

ATrialsObjectiveTarget::ATrialsObjectiveTarget(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ATrialsObjectiveTarget::PostInitializeComponents()
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

void ATrialsObjectiveTarget::Destroyed()
{
    auto* GS = GetWorld()->GetGameState<ATrialsGameState>();
    if (GS != nullptr)
    {
        GS->RemoveTarget(this);
    }
}
