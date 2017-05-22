#pragma once

#include "TrialsObjectiveInfo.h"
#include "TrialsObjective.generated.h"

UCLASS(abstract, Blueprintable)
class TRIALS_API ATrialsObjective : public AActor
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = Objective)
    ATrialsObjectiveInfo* ObjectiveInfo;
	
    virtual void BeginPlay() override;
    virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;
    virtual FVector GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow);

private:
    bool bBeaconWasLeft;
 };
