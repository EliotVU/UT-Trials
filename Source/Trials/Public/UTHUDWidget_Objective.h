#pragma once

#include "UTHUDWidget.h"
#include "UTHUDWidget_Objective.generated.h"

class ATrialsGameState;
class ATrialsObjectiveTarget;

UCLASS()
class TRIALS_API UUTHUDWidget_Objective : public UUTHUDWidget
{
    GENERATED_UCLASS_BODY()

    USoundBase* TickSound;
    USoundBase* EndTickSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text TitleItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text DetailsItem;

    // Copy from FlagStatus widget.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture CircleTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture CircleBorderTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture IconTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture ArrowTemplate;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Message")
    float InWorldAlpha;

    /** Largest scaling for in world indicators. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    float MaxIconScale;

    /** Padding for in world icons from top of screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldOverlay")
    float TopEdgePadding;

    /** Padding for in world icons from bottom of screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldOverlay")
    float BottomEdgePadding;

    /** Padding for in world icons from bottom of screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldOverlay")
    float LeftEdgePadding;

    /** Padding for in world icons from bottom of screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldOverlay")
    float RightEdgePadding;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture StatusBackground;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text StatusText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text RecordText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture RecordIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Texture TimerBackground;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
    FHUDRenderObject_Text TimerText;

    float AnimationAlpha;

    UPROPERTY(BlueprintReadOnly, Category = "Widgets Live")
    class AUTCharacter* ViewingCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Widgets Live")
    class ATrialsObjectiveTarget* LastRenderedTarget;

    UPROPERTY(BlueprintReadOnly, Category = "Widgets Live")
    class ATrialsPlayerState* ViewingPlayerState;

    void InitializeWidget(AUTHUD* Hud) override;

    void PreDraw(float DeltaTime, AUTHUD* InUTHUDOwner, UCanvas* InCanvas, FVector2D InCanvasCenter) override;
    void Draw_Implementation(float DeltaTime) override;
    void PostDraw(float RenderedTime) override;

    bool ShouldDraw_Implementation(bool bShowScores) override
    {
        return !bShowScores && UTGameState && UTGameState->HasMatchStarted() && !UTGameState->HasMatchEnded() && (UTGameState->GetMatchState() != MatchState::MatchIntermission);
    }

protected:
    virtual void DrawIndicators(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, float DeltaTime);
    virtual void DrawStatus(float DeltaTime);
    virtual void DrawObjWorld(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, ATrialsObjectiveTarget* Obj);
    FVector GetAdjustedScreenPosition(const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float IconSize, bool& bDrawEdgeArrow);
    void DrawEdgeArrow(FVector InWorldPosition, FVector PlayerViewPoint, FRotator PlayerViewRotation, FVector InDrawScreenPosition, float CurrentWorldAlpha, float WorldRenderScale);

private:
    float LastDrawnTimer;
    float EndTickTime;
    float TickTime;
    float LastTickTime;
    float LastRecordTime;
    float LastRecordChangeTime;

    static float Loopom(float timer, float time, float rate)
    {
        if (timer < time + rate*2.0)
        {
            float Delta = (timer - time) / rate;
            return FMath::Loge(2.0)*(Delta > 1.0 ? 1.0 - (Delta - 1.0) : Delta);
        }
        return 0.0;
    }
};
