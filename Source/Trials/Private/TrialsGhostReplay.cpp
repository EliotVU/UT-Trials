﻿#include "Trials.h"

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
        return;
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Owner = GetOwner();
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    Ghost = GetWorld()->SpawnActor<AUTCharacter>(GetWorld()->GetAuthGameMode<ATrialsGameMode>()->GhostClass, SpawnInfo);
    if (Ghost != nullptr)
    {
        Ghost->bOnlyRelevantToOwner = true;

        Ghost->SpawnDefaultController();
        Ghost->GhostComponent->GhostData = GhostData;
        Ghost->GhostComponent->GhostStartPlaying();
    }
}

// TODO: Destroy on fadeout!
void ATrialsGhostReplay::EndPlayback()
{
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->GhostStopPlaying();
    }
}

void ATrialsGhostReplay::Destroyed()
{
    if (Ghost != nullptr)
    {
        Ghost->GhostComponent->GhostData = nullptr;
        Ghost->GhostComponent->GhostStopPlaying();

        if (Ghost->Controller)
        {
            Ghost->Controller->PawnPendingDestroy(Ghost);
        }
        Ghost->Destroy();
    }
    Super::Destroyed();
}

