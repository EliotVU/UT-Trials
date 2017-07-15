#pragma once

#include "TrialsObjectiveInfo.h"

#include "TrialsObjective.generated.h"

UCLASS(Abstract, Blueprintable, DisplayName = "Objective Target")
class TRIALS_API ATrialsObjective : public AActor
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = Objective, DisplayName = Objective)
    ATrialsObjectiveInfo* ObjectiveInfo;

    void PostInitializeComponents() override;
    void Destroyed() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = HUD)
    UMaterialInterface* GetScreenIcon();
 };
