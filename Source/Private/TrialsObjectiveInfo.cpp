#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsObjectiveSetMessage.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "TrialsAPI.h"
#include "UnrealNetwork.h"
#include "TrialsPlayerController.h"
#include "TrialsGhostSerializer.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bCanSubmitRecords = false;

    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;

#if WITH_EDITORONLY_DATA
    //auto* MySprite = GetSpriteComponent();
    //if (MySprite)
    //{
    //    struct FConstructorStatics
    //    {
    //        ConstructorHelpers::FObjectFinderOptional<UTexture2D> TextureObject;
    //        FName ID;
    //        FText NAME;

    //        FConstructorStatics()
    //            : TextureObject(TEXT("/Game/RestrictedAssets/EditorAssets/Icons/generic_objective.generic_objective"))
    //            , ID(TEXT("Objectives"))
    //            , NAME(NSLOCTEXT("SpriteCategory", "Objectives", "Objectives"))
    //        {
    //        }
    //    };
    //    static FConstructorStatics ConstructorStatics;
    //    MySprite->Sprite = ConstructorStatics.TextureObject.Get();
    //    MySprite->SpriteInfo.Category = ConstructorStatics.ID;
    //    MySprite->SpriteInfo.DisplayName = ConstructorStatics.NAME;
    //}
#endif // WITH_EDITORONLY_DATA
}

#ifdef WITH_EDITOR

#include "UObjectToken.h"
#include "MapErrors.h"

void ATrialsObjective::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (GetWorld()->WorldType == EWorldType::Editor)
    {
        if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetName() == TEXT("ActorLabel"))
        {
            RecordId = GetActorLabel();
            Modify();
        }
    }
}

void ATrialsObjective::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();
    if (GetWorld()->WorldType == EWorldType::Editor)
    {
        RecordId = GetActorLabel();
        Modify();
    }
}

FName RecordIdIsEmpty(TEXT("RecordIdIsEmpty"));

void ATrialsObjective::CheckForErrors()
{
    if (RecordId.IsEmpty())
    {
        FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(NSLOCTEXT("Trials", "TrialsObjective", "RecordId is empty, please change the actor label to an appropiate objecive name!")))
            ->AddToken(FMapErrorToken::Create(RecordIdIsEmpty));
    }
}

#endif

void ATrialsObjective::BeginPlay()
{
    Super::BeginPlay();

    if (!GetWorld()->IsNetMode(NM_Client) && GetWorld()->GetAuthGameMode<ATrialsGameMode>() == nullptr)
    {
        Destroy();
        return;
    }

    RecordTime = DevRecordTime;
    if (PlayerStart != nullptr)
    {
        PlayerStart->bIgnoreInNonTeamGame = true; // Disable default spawning.
    }

    if (RequisiteObjective != nullptr)
    {
        if (Role == ROLE_Authority)
        {
            RequisiteObjective->OnObjectiveComplete.AddDynamic(this, &ATrialsObjective::OnRequisiteCompleted);
            RequisiteObjective->LockedObjectives.Add(this);
        }
        SetLocked(true);
    }
}

ATrialsAPI* ATrialsObjective::GetAPI() const
{
    return GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
}

void ATrialsObjective::UpdateRecordState(FString& MapName)
{
    auto ObjName = RecordId;
    auto ObjTitle = Title;
    auto ObjDescription = Description;

    if (ObjName.IsEmpty())
    {
        UE_LOG(UT, Error, TEXT("RecordId must be set to support records!"));
        return;
    }

    auto* API = GetAPI();
    API->GetObj(MapName, ObjName, [this, API](const FObjInfo& ObjInfo)
    {
        ObjectiveNetId = ObjInfo._id;
        TopRecords = ObjInfo.Records;

        float OldTime = RecordTime;
        float Time = ObjInfo.RecordTime > 0.f ? ObjInfo.RecordTime : DevRecordTime;
        RecordTime = ATrialsTimerState::RoundTime(Time);

        // TODO: Implement
        AvgRecordTime = ATrialsTimerState::RoundTime(Time);

        bCanSubmitRecords = true;

        // Try get ghost data
        bool bTimeChanged = Time != OldTime;
        if (TopRecords.Num() > 0 && bTimeChanged)
        {
            API->DownloadGhost(ObjectiveNetId, TopRecords[0].Player._id, [this](TArray<uint8> Data)
            {
                RecordGhostData = GhostDataSerializer::Serialize(Data);
            });
        }
    });
}

// TODO: Move to GameMode.
void ATrialsObjective::ScoreRecord(float Time, AUTPlayerController* PC)
{
    auto* TPC = Cast<ATrialsPlayerController>(PC);

    bool IsTopRecord = Time < RecordTime;
    if (IsTopRecord)
    {
        RecordGhostData = TPC->RecordingGhostData;
        RecordTime = Time;
    }

    auto* ScorerPS = Cast<ATrialsPlayerState>(TPC->PlayerState);
    ScorerPS->UpdateRecordTime(Time);
    OnRecordScored.Broadcast(TPC, Time, IsTopRecord);

    auto* DataObject = TPC->RecordingGhostData;
    TPC->RecordedGhostData = DataObject;
    TPC->RecordingGhostData = nullptr;

    if (bCanSubmitRecords)
    {
        checkSlow(!ObjectiveNetId.IsEmpty())
        checkSlow(!ScorerPS->PlayerNetId.IsEmpty())

        auto* API = GetAPI();
        API->SubmitRecord(Time, ObjectiveNetId, ScorerPS->PlayerNetId, [this, DataObject, API, Time, ScorerPS](const FRecordInfo& RecInfo)
        {
            // Just incase if the server records were not in sync with the database, so that we don't end overwriting our remotely stored ghost.
            if (DataObject != nullptr && Time <= ATrialsTimerState::RoundTime(RecInfo.Value))
            {
                TArray<uint8> ObjectBytes = GhostDataSerializer::Serialize(DataObject);
                API->SubmitGhost(ObjectBytes, ObjectiveNetId, ScorerPS->PlayerNetId);
            }

            FString MapName = GetWorld()->GetMapName();
            UpdateRecordState(MapName);
        });
    }
}

AUTPlayerStart* ATrialsObjective::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjective::ActivateObjective(APlayerController* PC)
{
    auto* TPC = Cast<ATrialsPlayerController>(PC);
    if (TPC == nullptr) return;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
    if (Char == nullptr) return;

    for (auto i = 0; i < PlayerInventory.Num(); ++i)
    {
        auto* Inv = Char->CreateInventory(PlayerInventory[i]);
        if (Inv != nullptr)
        {
            Char->AddInventory(Inv, false);
        }
    }

    if (PS->ActiveObjective != this)
    {
        TPC->StopGhostPlayback(false);

        PS->SetObjective(this);
        PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 0, PS, nullptr, this);
    }

    TPC->StartRecordingGhostData();
    TPC->FetchObjectiveGhostData(this, [this, TPC, PS](UUTGhostData* GhostData)
    {
        // Let's ensure that we don't playback a ghost if player de-activated this objective during this download.
        if (PS->ActiveObjective && PS->ActiveObjective != this)
        {
            return;
        }

        GhostData = GhostData != nullptr ? GhostData : RecordGhostData;
        if (GhostData != nullptr)
        {
            TPC->SummonGhostPlayback(GhostData);
        }
    });
    PS->StartObjective();
}

void ATrialsObjective::CompleteObjective(AUTPlayerController* PC)
{
    auto* TPC = Cast<ATrialsPlayerController>(PC);
    if (TPC == nullptr) return;

    // We don't want to complete an objective for clients whom have already completed or are doing a different objective.
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS->ActiveObjective != this) return;

    auto* TimerState = PS->TimerState;
    if (TimerState == nullptr || TimerState->State != TS_Active)
    {
        return;
    }

    auto* GM = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
    if (GM != nullptr)
    {
        TPC->StopRecordingGhostData();

        PS->RegisterUnlockedObjective(this);
        // Note: End before events
        float Timer = PS->EndObjective();

        OnObjectiveComplete.Broadcast(PC);
        GM->ScoreTrialObjective(this, Timer, PC);
    }
}

void ATrialsObjective::DisableObjective(APlayerController* PC, bool bDeActivate /*= false*/)
{
    auto* TPC = Cast<ATrialsPlayerController>(PC);
    if (TPC == nullptr) return;

    // Happens if an objective disables for a player with no set objective!
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS->ActiveObjective == this)
    {
        TPC->StopRecordingGhostData();
        TPC->RecordingGhostData = nullptr;

        TPC->StopGhostPlayback(false);
        TPC->RecordedGhostData = nullptr;

        // Do a full reset by giving an entire new Pawn, this should ensure that nothing leaves the level.
        auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
        if (Char != nullptr)
        {
            FTransform Trans = Char->GetTransform();
            auto Rot = PC->GetControlRotation();
            Trans.SetRotation(FQuat(Rot));

            auto* GameMode = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
            Char->Reset();
            PC->SetPawn(nullptr);
            GameMode->RestartPlayer(PC);
            // GameMode->RestartPlayerAtTransform(PC, Trans);
        }

        if (bDeActivate)
        {
            PS->SetObjective(nullptr);
            PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 1, PS, nullptr, this);
        }
    }
    else if (PS->ActiveObjective == nullptr)
    {
        return;
    }
    PS->EndObjective();
}

bool ATrialsObjective::IsEnabled(APlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS && PS->ActiveObjective == this;
}

bool ATrialsObjective::IsActive(APlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS && PS->ActiveObjective == this 
        && PS->TimerState && PS->TimerState->State == TS_Active;
}

void ATrialsObjective::OnRequisiteCompleted(AUTPlayerController* PC)
{
    if (PC == nullptr)
        return;

    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    ScorerPS->RegisterUnlockedObjective(this);
}

bool ATrialsObjective::IsLocked(APlayerController* PC)
{
    if (Role == ROLE_Authority)
    {
        if (PC != nullptr && bLockedLocale)
        {
            // check objectives list.
            auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
            return PS && !PS->UnlockedObjectives.Contains(this);
        }
        return false;
    }
    return bLockedLocale;
}

void ATrialsObjective::SetLocked(bool bIsLocked)
{
    bLockedLocale = bIsLocked;
    OnLockedChange.Broadcast(bLockedLocale);
}

void ATrialsObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsObjective, TopRecords);
    DOREPLIFETIME(ATrialsObjective, RecordTime);
    DOREPLIFETIME(ATrialsObjective, AvgRecordTime);
    DOREPLIFETIME(ATrialsObjective, bCanSubmitRecords);
}