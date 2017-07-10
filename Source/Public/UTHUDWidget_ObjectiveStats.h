#pragma once

#include "UTHUDWidget.h"
#include "UTHUDWidget_ObjectiveStats.generated.h"

UCLASS()
class TRIALS_API UUTHUDWidget_ObjectiveStats : public UUTHUDWidget
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture ItemBackgroundTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text PlayerTextItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text TimeTextItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ItemPaddingY;

    void Draw_Implementation(float DeltaTime) override;
    void DrawRecordItem(FRecordInfo& RecInfo, float Index);
    bool ShouldDraw_Implementation(bool bShowScores) override;
};
