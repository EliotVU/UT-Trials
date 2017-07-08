#include "Trials.h"
#include "TrialsPlayerState.h"

#include "UTHUDWidget_ObjectiveStats.h"

UUTHUDWidget_ObjectiveStats::UUTHUDWidget_ObjectiveStats(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DesignedResolution = 1080.f;
    bScaleByDesignedResolution = true;

    ScreenPosition = FVector2D(1.0f, 0.0f);
    Size = FVector2D(0.0f, 0.0f);
    Origin = FVector2D(1.0f, 0.0f);

    PlayerTextItem.Font = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->SmallFont;
    PlayerTextItem.bDrawShadow = true;
    PlayerTextItem.ShadowDirection = FVector2D(1.f, 2.f);
    PlayerTextItem.ShadowColor = FLinearColor::Black;
    PlayerTextItem.RenderColor = FLinearColor::Gray;
    PlayerTextItem.HorzPosition = ETextHorzPos::Left;

    TimeTextItem.Font = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->SmallFont;
    TimeTextItem.bDrawShadow = true;
    TimeTextItem.ShadowDirection = FVector2D(1.f, 2.f);
    TimeTextItem.ShadowColor = FLinearColor::Black;
    TimeTextItem.RenderColor = FLinearColor::Yellow;
    TimeTextItem.HorzPosition = ETextHorzPos::Right;
}

void UUTHUDWidget_ObjectiveStats::Draw_Implementation(float DeltaTime)
{
    auto* ViewedPawn = Cast<APawn>(UTPlayerOwner->GetViewTarget());
    auto* ViewPS = ViewedPawn ? Cast<ATrialsPlayerState>(ViewedPawn->PlayerState) : nullptr;
    auto* OwnerPS = ViewPS ? ViewPS : Cast<ATrialsPlayerState>(UTPlayerOwner->UTPlayerState);
    if (OwnerPS == nullptr) return;

    auto* Target = OwnerPS->ActiveObjectiveInfo;
    if (Target != nullptr)
    {
        int32 i = 0;
        for (; i < Target->TopRecords.Num() && i < 3; ++i)
        {
            DrawRecordItem(Target->TopRecords[i], i);
        }

        FRecordInfo RecInfo;
        if (i == 0)
        {
            RecInfo.Value = 0.0;
            RecInfo.Player.Name = TEXT("Developer");
            DrawRecordItem(RecInfo, Target->DevRecordTime);
        }

        RecInfo.Value = OwnerPS->ObjectiveRecordTime;
        RecInfo.Player.Name = OwnerPS->PlayerName;
        DrawRecordItem(RecInfo, i + 1);
    }
}

void UUTHUDWidget_ObjectiveStats::DrawRecordItem(FRecordInfo& RecInfo, int32 Index)
{
    float SizeY = 32 * 1.2f;

    FText PlayerText = FText::FromString(RecInfo.Player.Name);
    PlayerTextItem.Text = PlayerText;
    RenderObj_Text(PlayerTextItem, FVector2D(0, SizeY * Index));

    FText TimeText = RecInfo.Value != 0.0 ? ATrialsTimerState::FormatTime(RecInfo.Value) : FText::FromString(TEXT("N/A"));
    TimeTextItem.Text = TimeText;
    RenderObj_Text(TimeTextItem, FVector2D(0, SizeY * Index));
}

bool UUTHUDWidget_ObjectiveStats::ShouldDraw_Implementation(bool bShowScores)
{
    return !bShowScores && UTGameState && UTGameState->HasMatchStarted() && !UTGameState->HasMatchEnded() && (UTGameState->GetMatchState() != MatchState::MatchIntermission);
}
