#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "TrialsAPI.h"
#include "UnrealNetwork.h"
#include "TrialsPlayerController.h"
#include "TrialsGhostSerializer.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;

    Camera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("ObjectiveCamera"));
    if (Camera)
    {
        if (RootComponent)
        {
            Camera->SetupAttachment(RootComponent);
        }
        else
        {
            RootComponent = Camera;
        }

        FRotator DefaultCameraRotation;
        DefaultCameraRotation.Roll = 0.f;
        DefaultCameraRotation.Pitch = -20.f;
        DefaultCameraRotation.Yaw = 90.f;

        FTransform CameraTransform;
        CameraTransform.SetTranslation(FVector(0.f, 0.f, 80.f));
        CameraTransform.SetRotation(DefaultCameraRotation.Quaternion());
        Camera->SetRelativeTransform(CameraTransform);
        Camera->SetFieldOfView(100.f);
    }

#if WITH_EDITORONLY_DATA
    if (!IsRunningCommandlet())
    {
        if (GetSpriteComponent())
        {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteObject(TEXT("/Trials/HUD/SPRITE_Objective.SPRITE_Objective"));
            GetSpriteComponent()->Sprite = SpriteObject.Get();
            GetSpriteComponent()->SetupAttachment(RootComponent);
        }
    }
#endif // WITH_EDITORONLY_DATA

    bCanSubmitRecords = false;
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
    if (RecordId.IsEmpty())
    {
        UE_LOG(UT, Error, TEXT("RecordId must be set to support records!"));
        return;
    }

    auto* API = GetAPI();
    API->GetObj(MapName, RecordId, [this, API](const FObjInfo& ObjInfo)
    {
        ObjectiveNetId = ObjInfo._id;
        TopRecords = ObjInfo.Records;

        float OldTime = RecordTime;
        float Time = ObjInfo.RecordTime;
        RecordTime = ATrialsTimerState::RoundTime(Time);
        AvgRecordTime = ATrialsTimerState::RoundTime(ObjInfo.AvgRecordTime);

        bCanSubmitRecords = true;

        bool bHasMetaDataChanged = ObjInfo.Title != Title.ToString() 
            || ObjInfo.Description != Description.ToString()
            || ObjInfo.GoldMedalTime != GoldMedalTime
            || ObjInfo.SilverMedalTime != SilverMedalTime
            || ObjInfo.BronzeMedalTime != BronzeMedalTime
        ;
        if (bHasMetaDataChanged)
        {
            FObjInfo ObjData;
            ObjData._id = ObjectiveNetId;
            ObjData.Title = Title.ToString();
            ObjData.Description = Description.ToString();
            ObjData.GoldMedalTime = GoldMedalTime;
            ObjData.SilverMedalTime = SilverMedalTime;
            ObjData.BronzeMedalTime = BronzeMedalTime;
            API->UpdateObj(ObjData);
        }

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

int32 ATrialsObjective::CalcStarsCount(float Time) const
{
    if (Time <= GoldMedalTime)
    {
        return 3;
    }
    if (Time <= SilverMedalTime)
    {
        return 2;
    }
    if (Time <= BronzeMedalTime)
    {
        return 1;
    }
    return 0;
}

// TODO: Move to GameMode.
void ATrialsObjective::ScoreRecord(float Time, AUTPlayerController* PC)
{
    auto* TPC = Cast<ATrialsPlayerController>(PC);
    if (TPC == nullptr)
    {
        return;
    }

    bool IsTopRecord = Time < RecordTime || RecordTime <= 0.00;
    if (IsTopRecord)
    {
        RecordGhostData = TPC->RecordingGhostData;
        RecordTime = Time;
    }

    auto* ScorerPS = Cast<ATrialsPlayerState>(TPC->PlayerState);
    int32 NumStars = CalcStarsCount(Time);
    ScorerPS->AdjustScore(NumStars);
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
        API->SubmitRecord(ObjectiveNetId, ScorerPS->PlayerNetId, Time, [this, DataObject, API, Time, ScorerPS](const FRecordInfo& RecInfo)
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

    auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
    if (Char == nullptr)
    {
        UE_LOG(UT, Warning, TEXT("A character is required to active an objective."));
        return;
    }

    for (auto i = 0; i < PlayerInventory.Num(); ++i)
    {
        auto* Inv = Char->CreateInventory(PlayerInventory[i]);
        if (Inv != nullptr)
        {
            Char->AddInventory(Inv, true);
        }
    }

    TPC->StartObjective(this);
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

    // Note: We end the objective regardless if this was the objectives' proper exit as an anti cheat measure.

    // Must be checked before calling EndObjective, as EndObjective may change the active objective!
    //bool IsActiveObjective = Cast<ATrialsPlayerState>(PC->PlayerState)->ActiveObjective == this;
    TPC->EndObjective(this, bDeActivate);

    // Do a full reset by giving an entire new Pawn, this should ensure that nothing leaves the level.
    auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
    if (Char != nullptr)
    {
        FTransform Trans = Char->GetTransform();
        auto Rot = PC->GetControlRotation();

        auto* GameMode = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
        Char->Reset();
        PC->SetPawn(nullptr);
        GameMode->RestartPlayer(PC);
        // GameMode->RestartPlayerAtTransform(PC, Trans);

        if (PC->GetPawn())
        {
            PC->GetPawn()->SetActorTransform(Trans);
            PC->SetControlRotation(Rot);
        }
    }
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