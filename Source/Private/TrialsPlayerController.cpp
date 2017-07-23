#include "Trials.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"
#include "TrialsGhostReplay.h"
#include "TrialsGameMode.h"
#include "TrialsGhostSerializer.h"
#include "UTGhostComponent.h"

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

void ATrialsPlayerController::SetScoreObjectiveState()
{
    static FName NAME_CamMode(TEXT("FreeCam"));
    SetCameraMode(NAME_CamMode);

    auto* UTC = Cast<AUTCharacter>(GetCharacter());
    if (UTC != nullptr)
    {
        PawnPendingDestroy(UTC);
        UnPossess();
        SetViewTarget(UTC);

        UTC->GetCharacterMovement()->StopMovementImmediately();
        UTC->GetCharacterMovement()->DisableMovement();

        if (!UTC->IsDead())
        {            
            //PlayTaunt();
            UTC->DisallowWeaponFiring(true);

            UTC->PlayAnimMontage(UTC->CurrentTaunt);

            UTC->bTriggerRallyEffect = true;
            UTC->OnTriggerRallyEffect();
        }
    }

    auto* GhostData = RecordingGhostData;
    FTimerDelegate TimerCallback;
    TimerCallback.BindLambda([this, GhostData, UTC]() -> void
    {
        if (UTC != nullptr)
        {
            UTC->SpawnRallyDestinationEffectAt(UTC->GetActorLocation());
            UTC->Destroy();
        }

        auto* Char = GetCharacter();
        if (Char != nullptr && UTC != Char)
        {
            return;
        }

        // Was GhostData GC'd?
        if (GhostData == nullptr)
        {
            return;
        }
        ViewGhostPlayback(GhostData);
    });

    if (GetWorldTimerManager().IsTimerActive(ViewGhostPlaybackTimerHandle))
    {
        GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, 0.0, false);
        UE_LOG(UT, Warning, TEXT("ViewGhostPlaybackTimerHandle is already running!!!"));
    }
    GetWorldTimerManager().SetTimer(ViewGhostPlaybackTimerHandle, TimerCallback, 2.0, false);
}
