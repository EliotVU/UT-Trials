#include "Trials.h"

#include "TrialsGhostReplay.h"
#include "UTGhostComponent.h"

void ATrialsGhostReplay::StartPlayback(UUTGhostData* GhostData)
{
    checkSlow(GhostData);

    // Re-usage
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->GhostData = GhostData;

        OnRePlayFinished();
        return;
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Owner = GetOwner();
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UClass* GhostCharClass = LoadClass<AUTCharacter>(GetTransientPackage(), TEXT("/Trials/Blueprints/BP_Trials_GhostCharacter.BP_Trials_GhostCharacter_C"));
    if (GhostCharClass == nullptr)
    {
        GhostCharClass = GetWorld()->GetAuthGameMode()->DefaultPawnClass.Get();
    }

    Ghost = GetWorld()->SpawnActor<AUTCharacter>(GhostCharClass, SpawnInfo);
    if (Ghost != nullptr)
    {
        Ghost->SpawnDefaultController();
        Ghost->GhostComponent->GhostData = GhostData;

        Ghost->GhostComponent->GhostMoveToStart();
        Ghost->GhostComponent->GhostStartPlaying();
        Ghost->GhostComponent->GhostStopPlaying();
        Ghost->GhostComponent->OnGhostPlayFinished.AddDynamic(this, &ATrialsGhostReplay::OnRePlayFinished);
    }
}

// TODO: Destroy on fadeout!
void ATrialsGhostReplay::EndPlayback()
{
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->OnGhostPlayFinished.RemoveDynamic(this, &ATrialsGhostReplay::OnRePlayFinished);
        Ghost->GhostComponent->GhostStopPlaying();
    }
}

void ATrialsGhostReplay::OnRePlayFinished()
{
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->OnGhostPlayFinished.RemoveDynamic(this, &ATrialsGhostReplay::OnRePlayFinished);
        Ghost->GhostComponent->GhostStopPlaying();
        Ghost->GhostComponent->OnGhostPlayFinished.AddDynamic(this, &ATrialsGhostReplay::OnRePlayFinished);
    }
}

void ATrialsGhostReplay::Destroyed()
{
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->GhostData = nullptr;
        Ghost->GhostComponent->OnGhostPlayFinished.RemoveDynamic(this, &ATrialsGhostReplay::OnRePlayFinished);
        Ghost->GhostComponent->GhostStopPlaying();

        if (Ghost->Controller)
        {
            Ghost->Controller->PawnPendingDestroy(Ghost);
        }
        Ghost->Destroy();
    }
    Super::Destroyed();
}

