#pragma once

#include "UTGameObjective.h"
#include "TrialsObjective.generated.h"

UCLASS(Blueprintable)
class TRIALS_API ATrialsObjective : public AUTGameObjective
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
    FText Description;

    virtual void PostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir) override;
    virtual FVector GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow);

private:
    bool bBeaconWasLeft;
 };
