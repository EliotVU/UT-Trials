#include "Trials.h"
#include "TrialsGameState.h"
#include "UnrealNetwork.h"

ATrialsGameState::ATrialsGameState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool ATrialsGameState::AllowMinimapFor(AUTPlayerState* PS)
{
    return true;
}

// Copy from base, made a little change so that team 255 is not considered as a non-team number. 
// Easier than hacking in a Default team in a game type where most team features are unnecessary.
bool ATrialsGameState::OnSameTeam(const AActor* Actor1, const AActor* Actor2)
{
    const IUTTeamInterface* TeamInterface1 = Cast<IUTTeamInterface>(Actor1);
    const IUTTeamInterface* TeamInterface2 = Cast<IUTTeamInterface>(Actor2);
    if (TeamInterface1 == nullptr || TeamInterface2 == nullptr)
    {
        return false;
    }
    if (TeamInterface1->IsFriendlyToAll() || TeamInterface2->IsFriendlyToAll())
    {
        return true;
    }
    uint8 TeamNum1 = TeamInterface1->GetTeamNum();
    uint8 TeamNum2 = TeamInterface2->GetTeamNum();
    return TeamNum1 == TeamNum2;
}

void ATrialsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsGameState, Targets);
}