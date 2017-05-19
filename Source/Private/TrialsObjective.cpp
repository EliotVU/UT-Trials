#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsObjective.h"

#include "UTHUDWidget.h"
#include "UTATypes.h"
#include "TrialsPlayerState.h"

FVector ATrialsObjective::GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow)
{
    FVector Cross = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
    FVector DrawScreenPosition;
    float ExtraPadding = 0.065f * Canvas->ClipX;
    DrawScreenPosition = Canvas->Project(WorldPosition);
    FVector FlagDir = WorldPosition - ViewPoint;
    if ((ViewDir | FlagDir) < 0.f)
    {
        bool bWasLeft = bBeaconWasLeft;
        bDrawEdgeArrow = true;
        DrawScreenPosition.X = bWasLeft ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
        DrawScreenPosition.Y = 0.5f*Canvas->ClipY;
        DrawScreenPosition.Z = 0.0f;
        return DrawScreenPosition;
    }
    else if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > Canvas->ClipX))
    {
        bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
        float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - Canvas->ClipX;
        bDrawEdgeArrow = true;
        DrawScreenPosition.X = bLeftOfScreen ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
        //Y approaches 0.5*Canvas->ClipY as further off screen
        float MaxOffscreenDistance = Canvas->ClipX;
        DrawScreenPosition.Y = 0.4f*Canvas->ClipY 
            + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) 
            * (DrawScreenPosition.Y - 0.6f*Canvas->ClipY);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, 0.25f*Canvas->ClipY, 0.75f*Canvas->ClipY);
        bBeaconWasLeft = bLeftOfScreen;
    }
    else
    {
        bBeaconWasLeft = false;
        DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, Edge, Canvas->ClipX - Edge);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, Edge, Canvas->ClipY - Edge);
        DrawScreenPosition.Z = 0.0f;
    }
    return DrawScreenPosition;
}

void ATrialsObjective::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
    ATrialsPlayerState* ViewerPS = PC ? Cast<ATrialsPlayerState>(PC->PlayerState) : nullptr;
    ATrialsGameState* UTGS = GetWorld()->GetGameState<ATrialsGameState>();

    if (!ViewerPS || !UTGS)
    {
        return;
    }

    // Don't render this objective if the player is going for a different objective.
    if( ViewerPS->ActiveObjectiveInfo != nullptr && ViewerPS->ActiveObjectiveInfo->Objective != this )
    {
        return;
    }

    AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
    const bool bIsViewTarget = (PC->GetViewTarget() == this);
    FVector WorldPosition = GetActorLocation();
    if (UTPC != NULL && !UTGS->IsMatchIntermission() 
        && !UTGS->HasMatchEnded() 
        && ((FVector::DotProduct(CameraDir, (WorldPosition - CameraPosition)) > 0.0f))
        && (UTPC->MyUTHUD == nullptr || !UTPC->MyUTHUD->bShowScores))
    {
        float TextXL, YL;
        float Scale = Canvas->ClipX / 1920.f;
        UFont* SmallFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->SmallFont;

        FText ObjectiveTitle = Title;
        Canvas->TextSize(SmallFont, ObjectiveTitle.ToString(), TextXL, YL, Scale, Scale);
        FVector ViewDir = UTPC->GetControlRotation().Vector();
        float Dist = (CameraPosition - GetActorLocation()).Size();
        bool bDrawEdgeArrow = false; 
        FVector ScreenPosition = GetAdjustedScreenPosition(Canvas, WorldPosition, CameraPosition, ViewDir, Dist, 20.f, bDrawEdgeArrow);
        float XPos = ScreenPosition.X - 0.5f*TextXL;
        float YPos = ScreenPosition.Y - YL;
        if (XPos < Canvas->ClipX || XPos + TextXL < 0.0f)
        {
            FFormatNamedArguments Args;
            Args.Add("Title", Title);

            FText NumberText = FText::AsNumber(int32(0.01f*Dist));
            Args.Add("Dist", NumberText);

            float timer = ViewerPS->GetObjectiveTimer();
            FText TimeText = timer == -1
                ? FText::FromString(TEXT("InActive"))
                : ViewerPS->FormatTime(timer);
            Args.Add("Time", TimeText);

            FCanvasTextItem TextItem(FVector2D(FMath::TruncToFloat(Canvas->OrgX + XPos), 
                FMath::TruncToFloat(Canvas->OrgY + YPos - 1.2f*YL)), 
                ObjectiveTitle, SmallFont, FColor::Green);

            UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
            Canvas->TextSize(TinyFont, TEXT("WWWWWWWW: XX meters, 0:00:00.000 time"), TextXL, YL, Scale, Scale);
            TextItem.Font = TinyFont;

            FText DistText = FText::Format(NSLOCTEXT("UTRallyPoint", "DistanceText", "{Title}: {Dist} meters, {Time} time"), Args);
            TextItem.Text = DistText;
            TextItem.Position.X = ScreenPosition.X - 0.5f*TextXL;
            TextItem.Position.Y += 0.9f*YL;

            Canvas->DrawItem(TextItem);
        }
    }
}