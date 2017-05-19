#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsGameMode.h"

#include "TrialsGameState.generated.h"

UCLASS()
class ATrialsGameState : public AUTGameState
{
	GENERATED_UCLASS_BODY()
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    UPROPERTY(BlueprintReadOnly, Category = Trials)
    TArray<ATrialsObjectiveInfo*> Objectives;
};
