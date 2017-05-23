#pragma once

#include "GameFramework/PhysicsVolume.h"
#include "TrialsObjectiveInfo.h"

#include "TrialsObjectiveVolume.generated.h"

UCLASS(BlueprintType)
class TRIALS_API ATrialsObjectiveVolume : public APhysicsVolume
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Objective)
    ATrialsObjectiveInfo* ObjectiveInfo;

    virtual void BeginPlay() override;
    virtual void ActorEnteredVolume(class AActor* Other) override;
    virtual void ActorLeavingVolume(class AActor* Other) override;
};
