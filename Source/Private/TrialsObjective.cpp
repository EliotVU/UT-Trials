#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsObjective.h"

#include "UTHUDWidget.h"
#include "UTATypes.h"
#include "TrialsPlayerState.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;
}

void ATrialsObjective::BeginPlay()
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        return;
    }
    auto* HUD = Cast<AUTHUD>(GetWorld()->GetFirstPlayerController()->MyHUD);
    HUD->AddPostRenderedActor(this);
}

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
    auto* ViewerPS = PC ? Cast<ATrialsPlayerState>(PC->PlayerState) : nullptr;
    auto* UTGS = GetWorld()->GetGameState<ATrialsGameState>();

    if (!ViewerPS || !UTGS || !ObjectiveInfo)
    {
        return;
    }

    // Don't render this objective if the player is going for a different objective.
    if (ViewerPS->ActiveObjectiveInfo != nullptr && ViewerPS->ActiveObjectiveInfo != ObjectiveInfo)
    {
        return;
    }

    auto* UTPC = Cast<AUTPlayerController>(PC);
    const bool bIsViewTarget = (PC->GetViewTarget() == this);
    FVector WorldPosition = GetActorLocation();
    if (UTPC != nullptr && !UTGS->IsMatchIntermission() 
        && !UTGS->HasMatchEnded() 
        && ((FVector::DotProduct(CameraDir, (WorldPosition - CameraPosition)) > 0.0f))
        && (UTPC->MyUTHUD == nullptr || !UTPC->MyUTHUD->bShowScores))
    {
        bool bDrawEdgeArrow = false; 

        FVector ViewDir = UTPC->GetControlRotation().Vector();
        float Dist = (CameraPosition - GetActorLocation()).Size();
        FVector ScreenPosition = GetAdjustedScreenPosition(Canvas, WorldPosition, CameraPosition, ViewDir, Dist, 20.f, bDrawEdgeArrow);

        float Scale = Canvas->ClipX/1920.f;
        float TextXL, YL;
        UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
        Canvas->TextSize(TinyFont, ObjectiveInfo->Title.ToString(), TextXL, YL, Scale, Scale);

        float XPos = ScreenPosition.X - TextXL*0.5f;
        float YPos = ScreenPosition.Y - YL;
        if (XPos < Canvas->ClipX || XPos + TextXL < 0.0f)
        {
            FCanvasTextItem TextItem(FVector2D(XPos, YPos), ObjectiveInfo->Title, TinyFont, FColor::Yellow);
            TextItem.EnableShadow(FColor::Black);
            Canvas->DrawItem(TextItem);

            FFormatNamedArguments Args;
            FText NumberText = FText::AsNumber(int32(0.01f*Dist));
            Args.Add("Dist", NumberText);

            float Timer = ViewerPS->GetObjectiveTimer();
            FText TimeText = Timer == -1
                ? ViewerPS->FormatTime(ObjectiveInfo->RecordTime)
                : ViewerPS->FormatTime(ViewerPS->ObjectiveRecordTime > 0 ? ViewerPS->GetObjectiveRemainingTime() : Timer);
            Args.Add("Time", TimeText);

            FText DetailsText = FText::Format(NSLOCTEXT("Trials", "ObjectiveDetails", "[{Dist} meters, {Time}]"), Args);
            Canvas->TextSize(TinyFont, DetailsText.ToString(), TextXL, YL, Scale, Scale);
            TextItem.Text = DetailsText;
            TextItem.Position.X = ScreenPosition.X - TextXL*0.5f;
            TextItem.Position.Y += YL;
            Canvas->DrawItem(TextItem);
        }
    }
}