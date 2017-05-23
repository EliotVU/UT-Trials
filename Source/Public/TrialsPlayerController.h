#pragma once

#include "TrialsPlayerState.h"

#include "TrialsPlayerController.generated.h"

UCLASS()
class ATrialsPlayerController : public AUTPlayerController
{
	GENERATED_BODY()

    virtual void ServerSuicide_Implementation() override;
};