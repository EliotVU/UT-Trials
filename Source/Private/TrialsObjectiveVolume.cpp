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

    AUTPlayerController* PC = Cast<AUTPlayerController>(p->Controller);
    if (PC != nullptr)
    {
        ObjectiveInfo->ActivateObjective(PC);
    }
}

void ATrialsObjectiveVolume::ActorLeavingVolume(AActor* Other)
{
    Super::ActorLeavingVolume(Other);
    APawn* p = Cast<APawn>(Other);
    if (p == nullptr)
    {
        return;
    }

    AUTPlayerController* PC = Cast<AUTPlayerController>(p->Controller);
    if (PC != nullptr)
    {
        ObjectiveInfo->DisableObjective(PC, true);
    }
}
