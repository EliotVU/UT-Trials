#pragma once

#include "UTHUDWidget.h"
#include "UTHUDWidget_ObjectiveStats.generated.h"

UCLASS()
class TRIALS_API UUTHUDWidget_ObjectiveStats : public UUTHUDWidget
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text PlayerTextItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text TimeTextItem;

    void Draw_Implementation(float DeltaTime) override;
    void DrawRecordItem(FRecordInfo& RecInfo, int32 Index);
    bool ShouldDraw_Implementation(bool bShowScores) override;
};
