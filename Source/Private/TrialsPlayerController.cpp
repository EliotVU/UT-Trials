#include "Trials.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"
#include "TrialsGhostReplay.h"
#include "TrialsGameMode.h"
#include "TrialsGhostSerializer.h"
#include "UTGhostComponent.h"
#include "UnrealNetwork.h"

void ATrialsPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("RequestRally", IE_Pressed, this, &ATrialsPlayerController::RequestRestart);
}

void ATrialsPlayerController::Destroyed()
{
    if (GhostPlayback)
    {
        GhostPlayback->Destroy();
        GhostPlayback = nullptr;
    }
    Super::Destroyed();
}

void ATrialsPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    TPlayerState = Cast<ATrialsPlayerState>(PlayerState);
}

void ATrialsPlayerController::ServerSuicide_Implementation()
{
    // Try to perform an instant re-spawn with no death events and gore.
    auto* PS = Cast<ATrialsPlayerState>(PlayerState);
    auto* Char = Cast<AUTCharacter>(GetCharacter());
    if (Char != nullptr && PS->ActiveObjective != nullptr && !GetWorld()->GetAuthGameMode<AUTGameMode>()->AllowSuicideBy(this))
    {
        // Minor anti-spam limitation.
        if (GetWorld()->TimeSeconds - Char->CreationTime < 0.2f)
        {
            return;
        }

        Char->SpawnRallyDestinationEffectAt(Char->GetActorLocation());
        Char->Reset();
        SetPawn(nullptr);
        this->ServerRestartPlayer();

        auto* NewChar = Cast<AUTCharacter>(GetCharacter());
        if (NewChar)
        {
            Char->SpawnRallyDestinationEffectAt(NewChar->GetActorLocation());
        }
        return;
    }
    // Normal suicide if past quick re-spawn time.
    Super::ServerSuicide_Implementation();
}

void ATrialsPlayerController::ServerRestartPlayer_Implementation()
{
    ScoredGhostData = nullptr;
    bHasScoredReplayData = false;
    StopGhostPlayback(false);
    Super::ServerRestartPlayer_Implementation();
}

void ATrialsPlayerController::RequestRestart()
{
    if (UTPlayerState)
    {
        ServerRequestRestart();
    }
}

bool ATrialsPlayerController::ServerRequestRestart_Validate()
{
    return true;
}

void ATrialsPlayerController::ServerRequestRestart_Implementation()
{
    if (UTPlayerState->IsOnlySpectator())
    {
        return;
    }

    // FIXME: ScoredGhostData is sometimes not available? (assumption). Sometimes replay will force a map center view.
    if (GetCharacter() == nullptr && ScoredGhostData != nullptr)
    {
        if (UTPlayerState->bIsSpectator)
        {
            EndSpectatingState();
        }

        ViewGhostPlayback(ScoredGhostData);
        ScoredGhostData = nullptr;
        bHasScoredReplayData = false;
        return;
    }
    ServerSuicide();
}

void ATrialsPlayerController::StartRecordingGhostData()
{
    RecordingGhostData = nullptr;

    auto* Char = Cast<AUTCharacter>(GetCharacter());
    if (Char == nullptr)
    {
        return;
    }

    Char->GhostComponent->GhostStartRecording();
    RecordingGhostData = Char->GhostComponent->GhostData;
}

void ATrialsPlayerController::StopRecordingGhostData()
{
    auto* Char = Cast<AUTCharacter>(GetCharacter());
    if (Char == nullptr)
    {
        return;
    }

    Char->GhostComponent->GhostStopRecording();
}

void ATrialsPlayerController::ViewGhostPlayback(UUTGhostData* GhostData)
{
    SummonGhostPlayback(GhostData);
    check(GhostPlayback);

    bPlayerIsWaiting = true;
    BeginSpectatingState();

    if (GhostPlayback->Ghost != nullptr)
    {
        SetViewTarget(GhostPlayback->Ghost);

        GhostPlayback->Ghost->GhostComponent->GhostStartPlaying();
        GhostPlayback->Ghost->GhostComponent->OnGhostPlayFinished.AddDynamic(this, &ATrialsPlayerController::OnEndGhostPlayback);

        // TODO: on player respawn, do a ghost cleanup. 
    }
}

void ATrialsPlayerController::OnEndGhostPlayback()
{
    if (GhostPlayback != nullptr)
    {
        bool bRestart = false;
        if (GetViewTarget() == GhostPlayback->Ghost)
        {
            bRestart = true;
        }

        GhostPlayback->Ghost->GhostComponent->OnGhostPlayFinished.RemoveDynamic(this, &ATrialsPlayerController::OnEndGhostPlayback);
        GhostPlayback->EndPlayback();

        if (bRestart)
        {
            ServerRestartPlayer(); // Note: will destroy GhostPlayback.
        }
    }
}

void ATrialsPlayerController::SummonGhostPlayback(UUTGhostData* GhostData)
{
    check(GhostData);

    if (GhostPlayback == nullptr)
    {
        FActorSpawnParameters SpawnInfo;
        SpawnInfo.Owner = this;
        GhostPlayback = GetWorld()->SpawnActor<ATrialsGhostReplay>(SpawnInfo);
    }

    if (GhostPlayback != nullptr)
    {
        GhostPlayback->StartPlayback(GhostData);
    }
}

void ATrialsPlayerController::StopGhostPlayback(bool bDeActivate)
{
    if (GhostPlayback != nullptr)
    {
        if (bDeActivate)
        {
            GhostPlayback->EndPlayback();
            return;
        }

        GhostPlayback->Destroy();
        GhostPlayback = nullptr;
    }
}

void ATrialsPlayerController::FetchObjectiveGhostData(ATrialsObjective* Objective, const TFunction<void(UUTGhostData* GhostData)> OnResult)
{
    check(Objective);

    if (RecordedGhostData)
    {
        OnResult(RecordedGhostData);
        return;
    }

    auto* API = GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
    check(API);
    API->DownloadGhost(Objective->ObjectiveNetId, Cast<ATrialsPlayerState>(PlayerState)->PlayerNetId, 
        [this, OnResult](TArray<uint8> Data)
        {
            UUTGhostData* GhostData = GhostDataSerializer::Serialize(Data);
            RecordedGhostData = GhostData;
            OnResult(GhostData);
        }, 
        [OnResult]()
        {
            OnResult(nullptr);
        }
    );
}

// FIXME: Camera is locked despite "Free".
void ATrialsPlayerController::ScoredObjective(ATrialsObjective* Objective)
{
    static FName NAME_CamMode(TEXT("FreeCam"));
    SetCameraMode(NAME_CamMode);

    ScoredGhostData = RecordingGhostData;
    bHasScoredReplayData = true;

    auto* UTPawn = Cast<APawn>(GetPawn());
    if (UTPawn != nullptr)
    {
        auto* UTChar = Cast<AUTCharacter>(GetCharacter());
        if (UTChar != nullptr && !UTChar->IsDead())
        {
            UTChar->DisallowWeaponFiring(true);
            UTChar->GetCharacterMovement()->StopMovementImmediately();
            UTChar->GetCharacterMovement()->DisableMovement();

            UTChar->PlayAnimMontage(UTChar->CurrentTaunt);
            UTChar->bTriggerRallyEffect = true;
            UTChar->OnTriggerRallyEffect();
        }

        // Ensure death.
        UTPawn->SetLifeSpan(2.5);
        PawnPendingDestroy(UTPawn);
        SetViewTarget(UTPawn);
    }
    else
    {
        SetViewTarget(Objective->GetPlayerSpawn(this));
    }

    FTimerDelegate TimerCallback;
    TimerCallback.BindLambda([this, UTPawn]() -> void
    {
        // Cleanup replay possibility.
        ScoredGhostData = nullptr;
        bHasScoredReplayData = false;

        if (UTPawn != nullptr)
        {
            if (Cast<AUTCharacter>(UTPawn))
            {
                static_cast<AUTCharacter*>(UTPawn)->SpawnRallyDestinationEffectAt(UTPawn->GetActorLocation());
            }
            UTPawn->SetLifeSpan(0.0);
            UTPawn->Destroy();
        }

        bool bIsSpectatingGhost = (GhostPlayback != nullptr && GetViewTarget() == GhostPlayback->Ghost);
        if ((StateName == NAME_Spectating || GetCharacter() == nullptr) && !bIsSpectatingGhost)
        {
            SetViewTarget(GetWorld()->GetAuthGameMode()->FindPlayerStart(this));
        }
    });

    if (GetWorldTimerManager().IsTimerActive(ViewGhostPlaybackTimerHandle))
    {
        GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, 0.0, false);
        UE_LOG(UT, Warning, TEXT("ViewGhostPlaybackTimerHandle is already running!!!"));
    }
    GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, TimerCallback, 2.0, false);
}

void ATrialsPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ATrialsPlayerController, bHasScoredReplayData, COND_OwnerOnly);
}