#include "Trials.h"

#include "UTGhostComponent.h"
#include "TrialsGhostReplay.h"
#include "TrialsGameMode.h"

ATrialsGhostReplay::ATrialsGhostReplay(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer
        .DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
}

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

    Ghost = GetWorld()->SpawnActor<AUTCharacter>(GetWorld()->GetAuthGameMode<ATrialsGameMode>()->GhostClass, SpawnInfo);
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

