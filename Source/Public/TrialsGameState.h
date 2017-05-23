#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsGameMode.h"

#include "TrialsGameState.generated.h"

UCLASS()
class ATrialsGameState : public AUTGameState
{
	GENERATED_UCLASS_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = Trials)
    TArray<ATrialsObjectiveInfo*> Objectives;

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool AllowMinimapFor(AUTPlayerState* PS) override;
    virtual bool OnSameTeam(const AActor* Actor1, const AActor* Actor2) override;
};
