#pragma once

#include "TrialsPlayerController.h"

#include "TrialsGhostReplay.generated.h"

UCLASS()
class ATrialsGhostReplay : public AInfo
{
    GENERATED_UCLASS_BODY()

    UPROPERTY()
    AUTCharacter* Ghost;

    void StartPlayback(class UUTGhostData* GhostData);
    void EndPlayback();

    void Destroyed() override;
};
