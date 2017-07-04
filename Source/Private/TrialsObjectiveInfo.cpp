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
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;
}

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

void ATrialsObjectiveInfo::UpdateRecordState(FString MapName)
{
    // FIXME: Only available in development builds!
    auto ObjName = GetActorLabel();
    auto ObjTitle = Title;
    auto ObjDescription = Description;

    auto* API = GetAPI();
    API->GetObj(MapName, ObjName, [this](const FObjInfo& ObjInfo)
    {
        ObjectiveNetId = ObjInfo._id;
        TopRecords = ObjInfo.Records;

        float Time = ObjInfo.RecordTime > 0.f ? ObjInfo.RecordTime : DevRecordTime;
        RecordTime = ATrialsTimerState::RoundTime(Time);

        // TODO: Implement
        AvgRecordTime = ATrialsTimerState::RoundTime(Time);
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

    checkSlow(!ObjectiveNetId.IsEmpty())
    checkSlow(!ScorerPS->PlayerNetId.IsEmpty())

    auto* API = GetAPI();
    API->SubmitRecord(Record, ObjectiveNetId, ScorerPS->PlayerNetId, [this](const FRecordInfo& RecInfo)
    {
        UpdateRecordState(GetWorld()->GetMapName());
    });
}

AUTPlayerStart* ATrialsObjectiveInfo::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjectiveInfo::ActivateObjective(APlayerController* PC)
{
    if (PC == nullptr) return;

    if (IsLocked(PC))
    {
        auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
        if (Char != nullptr)
        {
            Char->PlayerSuicide();
        }
        return;
    }

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

    auto* TimerState = PS->TimerState;
    if (TimerState == nullptr || TimerState->State != TS_Active || PS->ActiveObjectiveInfo != this)
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

    // EXPLOIT: Cleanup any projectile that were fired that may potentionally be exploited to boost themselves after restarting the objective.
    // Handled in RestartPlayer
    //for (TActorIterator<AUTProjectile> It(GetWorld()); It; ++It)
    //{
    //    if (It->InstigatorController == PC)
    //    {
    //        It->Destroy();
    //    }
    //}

    // FIXME: Give player a new pawn at the same location instead?
    auto* Char = Cast<AUTCharacter>(PC->GetCharacter());
    if (Char != nullptr)
    {
        // Handled here instead of GameMode, because it shouldn't be called on initial spawns.
        auto* GameMode = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
        //Char->DiscardAllInventory();
        //GameMode->GiveDefaultInventory(Char);

        // FIXME: SetPlayerDefaults is stacking armor without this!
        //Char->SetInitialHealth();
        //Char->RemoveArmor(150);
        //GameMode->SetPlayerDefaults(Char);

        // Do a full reset by giving an entire new Pawn, this should ensure that nothing leaves the level.
        Char->Reset();
        PC->SetPawn(nullptr);
        GameMode->RestartPlayer(PC);
    }

    // Happens if an objective disables for a player with no set objective!
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS == nullptr || PS->ActiveObjectiveInfo == nullptr) return;

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
}