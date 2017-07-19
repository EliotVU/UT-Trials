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
    if (Char != nullptr && PS->ActiveObjectiveInfo != nullptr && !GetWorld()->GetAuthGameMode<AUTGameMode>()->AllowSuicideBy(this))
    {
        // Minor anti-spam limitation.
        if (GetWorld()->TimeSeconds - Char->CreationTime < 0.2f)
        {
            return;
        }

        Char->SpawnRallyEffectAt(Char->GetActorLocation());
        Char->Reset();
        SetPawn(nullptr);
        this->ServerRestartPlayer();

        auto* NewChar = Cast<AUTCharacter>(GetCharacter());
        if (NewChar)
        {
            Char->SpawnRallyDestinationEffectAt(Char->GetActorLocation());
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

void ATrialsPlayerController::FetchObjectiveGhostData(ATrialsObjectiveInfo* Objective, const TFunction<void(UUTGhostData* GhostData)> OnResult)
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
