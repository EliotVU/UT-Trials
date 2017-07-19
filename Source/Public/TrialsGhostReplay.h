#pragma once

#include "TrialsPlayerController.h"

#include "TrialsGhostReplay.generated.h"

UCLASS()
class ATrialsGhostReplay : public AInfo
{
    GENERATED_BODY()

public:
    UPROPERTY()
    ATrialsPlayerController* Client;

    UPROPERTY()
    AUTCharacter* Ghost;

    void StartPlayback(class UUTGhostData* GhostData);
    void EndPlayback();

    UFUNCTION()
    void OnRePlayFinished();

    void Destroyed() override;
};
