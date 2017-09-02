#include "Trials.h"
#include "TrialsGameState.h"
#include "TrialsPlayerState.h"

#include "UTHUDWidget_Objective.h"
#include "TrialsPlayerController.h"

UUTHUDWidget_Objective::UUTHUDWidget_Objective(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ScreenPosition = FVector2D(0.5f, 0.0f);
    Size = FVector2D(0.f, 0.f);
    Origin = FVector2D(0.5f, 0.5f);

    // CTF FlagStatus props
    InWorldAlpha = 0.8f;
    MaxIconScale = 0.75f;
    TopEdgePadding = 0.1f;
    BottomEdgePadding = 0.18f;
    LeftEdgePadding = 0.025f;
    RightEdgePadding = 0.025f;

    static ConstructorHelpers::FObjectFinder<USoundBase> TickSoundFinder(TEXT("SoundWav'/Trials/CharFade.CharFade'"));
    TickSound = TickSoundFinder.Object;

    static ConstructorHelpers::FObjectFinder<USoundBase> EndTickSoundFinder(TEXT("SoundWav'/Trials/CharFade.CharFade'"));
    EndTickSound = EndTickSoundFinder.Object;
}

void UUTHUDWidget_Objective::InitializeWidget(AUTHUD* Hud)
{
    Super::InitializeWidget(Hud);
}

void UUTHUDWidget_Objective::PreDraw(float DeltaTime, AUTHUD* InUTHUDOwner, UCanvas* InCanvas, FVector2D InCanvasCenter)
{
    Super::PreDraw(DeltaTime, InUTHUDOwner, InCanvas, InCanvasCenter);
    if (UTPlayerOwner != nullptr)
    {
        ViewingCharacter = Cast<AUTCharacter>(UTPlayerOwner->GetViewTarget());
        ViewingPlayerState = Cast<ATrialsPlayerState>(ViewingCharacter != nullptr ? ViewingCharacter->PlayerState != nullptr ? ViewingCharacter->PlayerState : UTPlayerOwner->PlayerState : UTPlayerOwner->PlayerState);
    }
}

void UUTHUDWidget_Objective::Draw_Implementation(float DeltaTime)
{
    auto* GameState = Cast<ATrialsGameState>(UTGameState);
    if (GameState == nullptr) return;

    if (Cast<ATrialsPlayerController>(UTPlayerOwner) != nullptr)
    {
        if (UTCharacterOwner == nullptr && static_cast<ATrialsPlayerController*>(UTPlayerOwner)->bHasScoredReplayData)
        {
            FText RequestRestartLabel = UTHUDOwner->FindKeyMappingTo("RequestRestart");
            FText& RallyLabel = UTHUDOwner->RallyLabel;

            FFormatNamedArguments Args;
            Args.Add(TEXT("keybind"), RequestRestartLabel.ToString() == TEXT("<none>") ? RallyLabel : RequestRestartLabel);

            FHUDRenderObject_Text ReplayText;
            ReplayText.bHidden = false; // where the fuck is this set to TRUE?
            ReplayText.Font = TimerText.Font;

            ReplayText.Text = FText::Format(FText::FromString(TEXT("(Press [{keybind}] to view Replay)")), Args);
            ReplayText.Position.X = 0.5;
            ReplayText.Position.Y = 0.2;
            ReplayText.HorzPosition = ETextHorzPos::Center;
            RenderObj_Text(ReplayText);
            return;
        }
        
        if (UTCharacterOwner)
        {
            FHUDRenderObject_Text ResetText;
            ResetText.bHidden = false;
            ResetText.Font = TimerText.Font;

            float ResetTime = UTCharacterOwner->UTCharacterMovement->DodgeResetTime - UTCharacterOwner->UTCharacterMovement->GetCurrentMovementTime();
            if (ResetTime > -1.0)
            {
                if (UTCharacterOwner->UTCharacterMovement->bIsDodging)
                {
                    ResetText.RenderColor = FLinearColor::Gray;
                }
                else if (ResetTime <= 0.0)
                {
                    ResetText.RenderColor = FLinearColor::Green;
                }
                ResetText.Text = ATrialsTimerState::FormatTime(ResetTime);
                ResetText.Position.X = 0.5;
                ResetText.Position.Y = 1080;
                ResetText.HorzPosition = ETextHorzPos::Center;
                RenderObj_Text(ResetText);
            }
        }
    }

    FVector ViewPoint;
    FRotator ViewRotation;

    UTPlayerOwner->GetPlayerViewPoint(ViewPoint, ViewRotation);
    DrawIndicators(GameState, ViewPoint, ViewRotation, DeltaTime);
    DrawStatus(DeltaTime);
}

void UUTHUDWidget_Objective::PostDraw(float RenderedTime)
{
    Super::PostDraw(RenderedTime);
    ViewingCharacter = nullptr;
    LastRenderedTarget = nullptr;
    ViewingPlayerState = nullptr;
}

void UUTHUDWidget_Objective::DrawIndicators(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, float DeltaTime)
{
    if (ViewingPlayerState == nullptr) return;
    for (auto& Target : GameState->Targets)
    {
        if (ViewingPlayerState->ActiveObjective == nullptr || Target->Objective == ViewingPlayerState->ActiveObjective)
        {
            DrawObjWorld(GameState, PlayerViewPoint, PlayerViewRotation, Target);
            LastRenderedTarget = Target;
        }
    }
}

void UUTHUDWidget_Objective::DrawStatus(float DeltaTime)
{
    if (ViewingPlayerState == nullptr)
    {
        return;
    }
    auto* Obj = ViewingPlayerState->ActiveObjective;
    if (Obj != nullptr)
    {
        RenderObj_Texture(StatusBackground);
        if (LastRenderedTarget != nullptr)
        {
            auto* IconMat = LastRenderedTarget->GetScreenIcon();
            if (IconMat)
            {
                DrawMaterial(IconMat, 
                    StatusBackground.Position.X, StatusBackground.Position.Y, 
                    StatusBackground.GetHeight(), StatusBackground.GetHeight(),
                    0.0, 0.0, 1.0, 1.0,
                    StatusText.RenderOpacity, StatusText.RenderColor
                );
            }
        }

        FText TitleText = Obj->Title;
        StatusText.Text = TitleText;
        RenderObj_Text(StatusText);

        if (Obj->RecordTime != LastRecordTime)
        {
            LastRecordChangeTime = GetWorld()->TimeSeconds;
        }
        LastRecordTime = Obj->RecordTime;

        FText TimeText = Obj->RecordTime != 0.00 
            ? ATrialsTimerState::FormatTime(Obj->RecordTime) 
            : FText::FromString(TEXT("--"));
        RecordText.Text = TimeText;

        float Scaler = Loopom(GetWorld()->TimeSeconds, LastRecordChangeTime, 0.18);
        RecordText.RenderColor = ATrialsTimerState::LeadColor*(1.0 - Scaler) + FLinearColor::White*Scaler;
        RecordText.TextScale = 1.0 + 0.15*Scaler;

        float XL, YL;
        Canvas->TextSize(RecordText.Font, RecordText.Text.ToString(), XL, YL, 1.0, 1.0);
        RenderObj_Text(RecordText, FVector2D((XL*RecordText.TextScale - XL)*0.5, (YL - YL*RecordText.TextScale)*0.5));
        RenderObj_Texture(RecordIcon);
    }

    auto* TimerState = ViewingPlayerState->TimerState;
    if (Obj != nullptr && TimerState != nullptr)
    {
        float RecordTime = TimerState->GetRecordTime();
        float Timer = RecordTime == 0.00 ? TimerState->GetTimer() : TimerState->GetRemainingTime();

        bool IsActive = TimerState->State == TS_Active;
        FLinearColor TimerColor = !IsActive
            ? ATrialsTimerState::IdleColor
            : ATrialsTimerState::GetTimerColor(Timer);

        FText TimeText = ATrialsTimerState::FormatTime(IsActive ? Timer : RecordTime);

        AnimationAlpha += (DeltaTime * 3);
        float Alpha = FMath::Abs<float>(FMath::Sin(AnimationAlpha));

        float WorldTime = GetWorld()->TimeSeconds;
        float Scaler = Loopom(WorldTime, TimerState->StateChangeTime, 0.1)
            + Loopom(WorldTime, EndTickTime, 0.05)*2.0
            + Loopom(WorldTime, TickTime, 0.05)*1.5 
            + Loopom(WorldTime, LastTickTime, 0.05)*1.2;

        if (IsActive && RecordTime != 0.00)
        {
            // HACK: + 1 to distinguish from -0.99 +0.99, both are rounded to 0.
            bool bTick = Timer < 10.0f && static_cast<int32>(LastDrawnTimer + 1) != static_cast<int32>(Timer + 1);
            if (bTick)
            {
                if (Timer < 0.0f && static_cast<int32>(Timer) == 0)
                {
                    EndTickTime = WorldTime;
                    UGameplayStatics::PlaySound2D(GetWorld(), EndTickSound, 3.5f, 1.0f, 0.0f);
                }
                else if (Timer > 0.0f)
                {
                    TickTime = WorldTime;
                    UGameplayStatics::PlaySound2D(GetWorld(), TickSound, 1.5f, 20.0f / Timer, 0.0f);
                }
            }
            LastTickTime = WorldTime;
            LastDrawnTimer = Timer;
        }

        TimerText.RenderColor = TimerColor;
        TimerText.RenderOpacity = Alpha;
        TimerText.TextScale = 1.0 + (0.1*Scaler);
        TimerText.Text = TimeText;

        float XL, YL;
        Canvas->TextSize(TimerText.Font, TEXT("T"), XL, YL, 1.0, 1.0);
        RenderObj_Text(TimerText, FVector2D(0.0, (YL - YL*TimerText.TextScale)*0.5));
    }
    else
    {
        AnimationAlpha = 0.0;
        LastDrawnTimer = 0;
    }
}

void UUTHUDWidget_Objective::DrawObjWorld(ATrialsGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, ATrialsObjectiveTarget* Target)
{
    bScaleByDesignedResolution = false;
    IconTemplate.RenderColor = Target->Objective && Target->Objective->IsLocked(UTPlayerOwner)
        ? ATrialsTimerState::NegativeColor // Locked color?
        : ATrialsTimerState::IdleColor; // Unlocked color?

    // Draw the flag / flag base in the world
    float WorldRenderScale = RenderScale * MaxIconScale;
    float OldAlpha = IconTemplate.RenderOpacity;

    FVector WorldPosition = Target->GetActorLocation() + FVector(0.f, 0.f, 128 * 0.75f);
    FVector ViewDir = PlayerViewRotation.Vector();
    float Dist = (Target->GetActorLocation() - PlayerViewPoint).Size();
    float Edge = CircleTemplate.GetWidth()*WorldRenderScale;
    bool bDrawEdgeArrow = false;
    FVector DrawScreenPosition = GetAdjustedScreenPosition(WorldPosition, PlayerViewPoint, ViewDir, Dist, Edge, bDrawEdgeArrow);

    float PctFromCenter = (DrawScreenPosition - FVector(0.5f*GetCanvas()->ClipX, 0.5f*GetCanvas()->ClipY, 0.f)).Size() / GetCanvas()->ClipX;
    float CurrentWorldAlpha = InWorldAlpha * FMath::Min(6.f*PctFromCenter, 1.f);

    // don't overlap player beacon
    UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
    float X, Y;
    float Scale = Canvas->ClipX / 1920.f;
    Canvas->TextSize(TinyFont, FString("+999   A999"), X, Y, Scale, Scale);

    if (!bDrawEdgeArrow)
    {
        float MinDistSq = FMath::Square(0.06f*GetCanvas()->ClipX);
        float ActualDistSq = FMath::Square(DrawScreenPosition.X - 0.5f*GetCanvas()->ClipX) + FMath::Square(DrawScreenPosition.Y - 0.5f*GetCanvas()->ClipY);
        if (ActualDistSq > MinDistSq)
        {
            DrawScreenPosition.Y -= 1.5f*Y;
        }
    }
    DrawScreenPosition.X -= RenderPosition.X;
    DrawScreenPosition.Y -= RenderPosition.Y;

    TitleItem.RenderOpacity = CurrentWorldAlpha;
    DetailsItem.RenderOpacity = CurrentWorldAlpha;
    IconTemplate.RenderOpacity = CurrentWorldAlpha;
    CircleTemplate.RenderOpacity = CurrentWorldAlpha;
    CircleBorderTemplate.RenderOpacity = CurrentWorldAlpha;

    RenderObj_TextureAt(CircleTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleTemplate.GetWidth()* WorldRenderScale, CircleTemplate.GetHeight()* WorldRenderScale);

    RenderObj_TextureAt(CircleBorderTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleBorderTemplate.GetWidth()* WorldRenderScale, CircleBorderTemplate.GetHeight()* WorldRenderScale);

    if (bDrawEdgeArrow)
    {
        DrawEdgeArrow(WorldPosition, PlayerViewPoint, PlayerViewRotation, DrawScreenPosition, CurrentWorldAlpha, WorldRenderScale);

        FFormatNamedArguments Args;
        FText NumberText = FText::AsNumber(int32(0.01f*Dist));
        Args.Add("Dist", NumberText);

        DetailsItem.RenderColor = ATrialsTimerState::IdleColor;
        DetailsItem.Text = FText::Format(NSLOCTEXT("Trials", "ObjectiveDetails", "{Dist}m"), Args);
        RenderObj_Text(DetailsItem, FVector2D(DrawScreenPosition));
    }
    else
    {
        if (ViewingPlayerState != nullptr)
        {
            if (ViewingPlayerState->ActiveObjective == nullptr)
            {
                TitleItem.Text = Target->Objective->Title;
                RenderObj_Text(TitleItem, FVector2D(DrawScreenPosition));
            }
            else if (ViewingPlayerState->ActiveObjective == Target->Objective)
            {
                auto* TimerState = ViewingPlayerState->TimerState;
                if (TimerState != nullptr)
                {
                    bool IsActive = TimerState->State == TS_Active;
                    float RecordTime = TimerState->GetRecordTime();
                    float Timer = RecordTime == 0? TimerState->GetTimer() : TimerState->GetRemainingTime();

                    FLinearColor TimerColor = IsActive
                        ? ATrialsTimerState::GetTimerColor(Timer)
                        : ATrialsTimerState::IdleColor;
                    DetailsItem.RenderColor = TimerColor;

                    FText TimeText = ViewingPlayerState->FormatTime(IsActive ? Timer : RecordTime);
                    DetailsItem.Text = TimeText;
                    RenderObj_Text(DetailsItem, FVector2D(DrawScreenPosition));
                }
            }
        }

        RenderObj_TextureAt(IconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, 1.25f*IconTemplate.GetWidth()*WorldRenderScale, 1.25f*IconTemplate.GetHeight()* WorldRenderScale);
    }

    TitleItem.RenderOpacity = OldAlpha;
    DetailsItem.RenderOpacity = OldAlpha;
    IconTemplate.RenderOpacity = OldAlpha;
    CircleTemplate.RenderOpacity = 1.f;
    CircleBorderTemplate.RenderOpacity = 1.f;

    bScaleByDesignedResolution = true;
}

FVector UUTHUDWidget_Objective::GetAdjustedScreenPosition(const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float IconSize, bool& bDrawEdgeArrow)
{
    FVector DrawScreenPosition = GetCanvas()->Project(WorldPosition);
    FVector FlagDir = (WorldPosition - ViewPoint).GetSafeNormal();
    if ((ViewDir | FlagDir) < 0.f)
    {
        bDrawEdgeArrow = true;
        FVector LeftDir = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
        bool bLeftOfScreen = (FlagDir | LeftDir) > 0.f;
        DrawScreenPosition.X = bLeftOfScreen ? -0.5f*GetCanvas()->ClipX * (ViewDir | FlagDir) : GetCanvas()->ClipX * (1.f + 0.5f*(ViewDir | FlagDir));
        DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY - IconSize;
        DrawScreenPosition.Z = 0.0f;
        DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
        return DrawScreenPosition;
    }
    if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > GetCanvas()->ClipX))
    {
        bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
        float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - GetCanvas()->ClipX;
        bDrawEdgeArrow = true;
        DrawScreenPosition.X = bLeftOfScreen ? IconSize + LeftEdgePadding*GetCanvas()->ClipX : GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX;
        //Y approaches 0.9*Canvas->ClipY as further off screen
        float MaxOffscreenDistance = GetCanvas()->ClipX;
        DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) * (DrawScreenPosition.Y - (1.f - BottomEdgePadding)*GetCanvas()->ClipY);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
    }
    else
    {
        DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
        DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
        DrawScreenPosition.Z = 0.0f;
    }
    return DrawScreenPosition;
}

void UUTHUDWidget_Objective::DrawEdgeArrow(FVector InWorldPosition, FVector PlayerViewPoint, FRotator PlayerViewRotation, FVector InDrawScreenPosition, float CurrentWorldAlpha, float WorldRenderScale)
{
    ArrowTemplate.RenderScale = 1.1f * WorldRenderScale;
    ArrowTemplate.RenderOpacity = CurrentWorldAlpha;
    ArrowTemplate.RenderColor = IconTemplate.RenderColor;
    float RotYaw = FMath::Acos(PlayerViewRotation.Vector() | (InWorldPosition - PlayerViewPoint).GetSafeNormal()) * 180.f / PI;
    if (InDrawScreenPosition.X < 0.f)
    {
        RotYaw *= -1.f;
    }
    RenderObj_TextureAtWithRotation(ArrowTemplate, FVector2D(InDrawScreenPosition.X, InDrawScreenPosition.Y), RotYaw);
}
