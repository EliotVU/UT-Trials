#include "Trials.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"
#include "TrialsGhostReplay.h"
#include "TrialsGameMode.h"
#include "TrialsGhostSerializer.h"
#include "UTGhostComponent.h"

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

    if (GetCharacter() == nullptr && ScoredGhostData != nullptr)
    {
        if (UTPlayerState->bIsSpectator)
        {
            EndSpectatingState();
        }

        ViewGhostPlayback(ScoredGhostData);
        ScoredGhostData = nullptr;
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

        GhostPlayback->Ghost->GhostComponent->GhostStopPlaying();
        GhostPlayback->Ghost->GhostComponent->GhostStartPlaying();
        GhostPlayback->Ghost->GhostComponent->OnGhostPlayFinished.AddDynamic(this, &ATrialsPlayerController::OnEndGhostPlayback);

        // TODO: on player respawn, do a ghost cleanup. 
    }
}

void ATrialsPlayerController::OnEndGhostPlayback()
{
    if (GhostPlayback != nullptr)
    {
        GhostPlayback->Ghost->GhostComponent->OnGhostPlayFinished.RemoveDynamic(this, &ATrialsPlayerController::OnEndGhostPlayback);
        GhostPlayback->Destroy();
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

void ATrialsPlayerController::ScoredObjective(ATrialsObjective* Objective)
{
    ScoredGhostData = RecordingGhostData;

    static FName NAME_CamMode(TEXT("FreeCam"));
    SetCameraMode(NAME_CamMode);

    auto* UTC = Cast<AUTCharacter>(GetCharacter());
    if (UTC != nullptr)
    {
        PawnPendingDestroy(UTC);
        SetViewTarget(UTC);

        UTC->GetCharacterMovement()->StopMovementImmediately();
        UTC->GetCharacterMovement()->DisableMovement();

        if (!UTC->IsDead())
        {            
            //PlayTaunt();
            //UTC->StopFiring(); // handled in unpossed via PawnPendingDestroy()
            UTC->DisallowWeaponFiring(true);

            UTC->PlayAnimMontage(UTC->CurrentTaunt);

            UTC->bTriggerRallyEffect = true;
            UTC->OnTriggerRallyEffect();

            // Ensure death.
            UTC->SetLifeSpan(2.5);
        }
    }

    FTimerDelegate TimerCallback;
    TimerCallback.BindLambda([this, Objective, UTC]() -> void
    {
        if (UTC != nullptr)
        {
            UTC->SpawnRallyDestinationEffectAt(UTC->GetActorLocation());
            UTC->Reset();
        }
        ScoredGhostData = nullptr;

        // We don't have a reference to the objective target, so show objective's spawn point instead.
        if (GetCharacter() == nullptr)
        {
            SetViewTarget(Objective->GetPlayerSpawn(this));
        }
    });

    if (GetWorldTimerManager().IsTimerActive(ViewGhostPlaybackTimerHandle))
    {
        GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, 0.0, false);
        UE_LOG(UT, Warning, TEXT("ViewGhostPlaybackTimerHandle is already running!!!"));
    }
    GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, TimerCallback, 2.0, false);
}
