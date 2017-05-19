#include "Trials.h"
#include "TrialsObjectiveVolume.h"
#include "TrialsPlayerState.h"

void ATrialsObjectiveVolume::BeginPlay()
{
    Super::BeginPlay();
    if (ObjectiveInfo == nullptr)
    {
        UE_LOG(UT, Error, TEXT("No ObjectiveInfo set for this volume"));
        return;
    }
}

void ATrialsObjectiveVolume::ActorEnteredVolume(class AActor* Other)
{
    Super::ActorEnteredVolume(Other);

    APawn* p = Cast<APawn>(Other);
    if (p == nullptr)
    {
        return;
    }

    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(p->PlayerState);
    if (PS == nullptr) 
    {
        return;
    }

    // Start objective
    PS->SetObjective(ObjectiveInfo);
}

void ATrialsObjectiveVolume::ActorLeavingVolume(AActor* Other)
{
    Super::ActorLeavingVolume(Other);

    APawn* p = Cast<APawn>(Other);
    if (p == nullptr)
    {
        return;
    }

    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(p->PlayerState);
    if (PS == nullptr)
    {
        return;
    }

    if (PS->ActiveObjectiveInfo == nullptr) 
    {
        return; // fucker spawned inside the objective volume without touching it first?
    }

    PS->StartObjectiveTimer();
}
