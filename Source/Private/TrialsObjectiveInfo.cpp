#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsObjectiveSetMessage.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "TrialsAPI.h"
#include "UnrealNetwork.h"

ATrialsObjectiveInfo::ATrialsObjectiveInfo(const class FObjectInitializer& ObjectInitializer)
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

void ATrialsObjectiveInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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

void ATrialsObjectiveInfo::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();
    if (GetWorld()->WorldType == EWorldType::Editor)
    {
        RecordId = GetActorLabel();
        Modify();
    }
}

FName RecordIdIsEmpty(TEXT("RecordIdIsEmpty"));

void ATrialsObjectiveInfo::CheckForErrors()
{
    if (RecordId.IsEmpty())
    {
        FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(NSLOCTEXT("Trials", "TrialsObjectiveInfo", "RecordId is empty, please change the actor label to an appropiate objecive name!")))
            ->AddToken(FMapErrorToken::Create(RecordIdIsEmpty));
    }
}

#endif

void ATrialsObjectiveInfo::BeginPlay()
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
            RequisiteObjective->OnObjectiveComplete.AddDynamic(this, &ATrialsObjectiveInfo::OnRequisiteCompleted);
            RequisiteObjective->LockedObjectives.Add(this);
        }
        SetLocked(true);
    }
}

ATrialsAPI* ATrialsObjectiveInfo::GetAPI() const
{
    return GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
}

void ATrialsObjectiveInfo::UpdateRecordState(FString& MapName)
{
    // FIXME: Only available in development builds!
    auto ObjName = RecordId;
    auto ObjTitle = Title;
    auto ObjDescription = Description;

    if (ObjName.IsEmpty())
    {
        UE_LOG(UT, Error, TEXT("RecordId must be set to support records!"));
        return;
    }

    auto* API = GetAPI();
    API->GetObj(MapName, ObjName, [this](const FObjInfo& ObjInfo)
    {
        ObjectiveNetId = ObjInfo._id;
        TopRecords = ObjInfo.Records;

        float Time = ObjInfo.RecordTime > 0.f ? ObjInfo.RecordTime : DevRecordTime;
        RecordTime = ATrialsTimerState::RoundTime(Time);

        // TODO: Implement
        AvgRecordTime = ATrialsTimerState::RoundTime(Time);

        bCanSubmitRecords = true;
    });
}

void ATrialsObjectiveInfo::ScoreRecord(float Record, AUTPlayerController* PC)
{
    if (Record < RecordTime)
    {
        RecordTime = Record;
    }

    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    ScorerPS->UpdateRecordTime(Record);

    if (bCanSubmitRecords)
    {
        checkSlow(!ObjectiveNetId.IsEmpty())
        checkSlow(!ScorerPS->PlayerNetId.IsEmpty())

        auto* API = GetAPI();
        API->SubmitRecord(Record, ObjectiveNetId, ScorerPS->PlayerNetId, [this](const FRecordInfo& RecInfo)
        {
            FString MapName = GetWorld()->GetMapName();
            UpdateRecordState(MapName);
        });
    }
}

AUTPlayerStart* ATrialsObjectiveInfo::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjectiveInfo::ActivateObjective(APlayerController* PC)
{
    if (PC == nullptr) return;

    auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
    if (Char != nullptr)
    {
        for (auto i = 0; i < PlayerInventory.Num(); ++i)
        {
            auto* Inv = Char->CreateInventory(PlayerInventory[i]);
            if (Inv != nullptr)
            {
                Char->AddInventory(Inv, false);
            }
        }
    }

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS->ActiveObjectiveInfo != this)
    {
        PS->SetObjective(this);
        PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 0, PS, nullptr, this);
    }
    PS->StartObjective();
}

void ATrialsObjectiveInfo::CompleteObjective(AUTPlayerController* PC)
{
    if (PC == nullptr) return;

    // We don't want to complete an objective for clients whom have already completed or are doing a different objective.
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS->ActiveObjectiveInfo != this) return;

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

void ATrialsObjectiveInfo::DisableObjective(APlayerController* PC, bool bDeActivate /*= false*/)
{
    if (PC == nullptr) return;

    // Happens if an objective disables for a player with no set objective!
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS == nullptr || PS->ActiveObjectiveInfo == nullptr) return;

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

    if (bDeActivate && PS->ActiveObjectiveInfo == this)
    {
        PS->SetObjective(nullptr);
        PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 1, PS, nullptr, this);
    }
    PS->EndObjective();
}

bool ATrialsObjectiveInfo::IsEnabled(APlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS && PS->ActiveObjectiveInfo == this;
}

bool ATrialsObjectiveInfo::IsActive(APlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS && PS->ActiveObjectiveInfo == this 
        && PS->TimerState && PS->TimerState->State == TS_Active;
}

void ATrialsObjectiveInfo::OnRequisiteCompleted(AUTPlayerController* PC)
{
    if (PC == nullptr)
        return;

    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    ScorerPS->RegisterUnlockedObjective(this);
}

bool ATrialsObjectiveInfo::IsLocked(APlayerController* PC)
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

void ATrialsObjectiveInfo::SetLocked(bool bIsLocked)
{
    bLockedLocale = bIsLocked;
    OnLockedChange.Broadcast(bLockedLocale);
}

void ATrialsObjectiveInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsObjectiveInfo, TopRecords);
    DOREPLIFETIME(ATrialsObjectiveInfo, RecordTime);
    DOREPLIFETIME(ATrialsObjectiveInfo, AvgRecordTime);
    DOREPLIFETIME(ATrialsObjectiveInfo, bCanSubmitRecords);
}