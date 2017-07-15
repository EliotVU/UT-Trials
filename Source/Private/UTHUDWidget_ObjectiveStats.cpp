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

    PlayerTextItem.HorzPosition = ETextHorzPos::Left;
    TimeTextItem.HorzPosition = ETextHorzPos::Right;
    ItemPaddingY = 12.0;
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
        PlayerTextItem.RenderColor = FLinearColor::Gray;

        int32 i = 0;
        for (; i < Target->TopRecords.Num() && i < 3; ++i)
        {
            TimeTextItem.RenderColor = i == 0 ? ATrialsTimerState::LeadColor : ATrialsTimerState::NegativeColor;
            DrawRecordItem(Target->TopRecords[i], i);
        }

        FRecordInfo RecInfo;
        if (i == 0)
        {
            RecInfo.Value = Target->DevRecordTime;
            RecInfo.Player.Name = TEXT("Developer");
            TimeTextItem.RenderColor = ATrialsTimerState::LeadColor;
            DrawRecordItem(RecInfo, 0);
        }

        RecInfo.Value = OwnerPS->ObjectiveRecordTime;
        RecInfo.Player.Name = OwnerPS->PlayerName;
        PlayerTextItem.RenderColor = FLinearColor::White;
        TimeTextItem.RenderColor = ATrialsTimerState::IdleColor;
        DrawRecordItem(RecInfo, static_cast<float>(i) + 1, ItemPaddingY);
    }
}

void UUTHUDWidget_ObjectiveStats::DrawRecordItem(FRecordInfo& RecInfo, float Index, float MarginY)
{
    float SizeY = ItemBackgroundTemplate.GetHeight();
    float Y = (SizeY + ItemPaddingY) * Index + MarginY;

    RenderObj_Texture(ItemBackgroundTemplate, FVector2D(0, Y));

    FText PlayerText = FText::FromString(RecInfo.Player.Name);
    PlayerTextItem.Text = PlayerText;
    RenderObj_Text(PlayerTextItem, FVector2D(0, Y));

    FText TimeText = RecInfo.Value != 0.0 ? ATrialsTimerState::FormatTime(RecInfo.Value) : FText::FromString(TEXT("N/A"));
    TimeTextItem.Text = TimeText;
    RenderObj_Text(TimeTextItem, FVector2D(0, Y));
}

bool UUTHUDWidget_ObjectiveStats::ShouldDraw_Implementation(bool bShowScores)
{
    return !bShowScores && UTGameState && UTGameState->HasMatchStarted() && !UTGameState->HasMatchEnded() && (UTGameState->GetMatchState() != MatchState::MatchIntermission);
}
