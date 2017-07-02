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
}

ATrialsAPI* ATrialsObjectiveInfo::GetAPI() const
{
    return GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
}

void ATrialsObjectiveInfo::InitData(FString MapName)
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
    API->SubmitRecord(Record, ObjectiveNetId, ScorerPS->PlayerNetId);
}

AUTPlayerStart* ATrialsObjectiveInfo::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjectiveInfo::ActivateObjective(APlayerController* PC)
{
    if (PC == nullptr) return;

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
    check(PS);

    auto* TimerState = PS->TimerState;
    if (TimerState == nullptr || TimerState->State != TS_Active || PS->ActiveObjectiveInfo != this)
    {
        return;
    }

    auto* GM = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
    if (GM != nullptr)
    {
        // Note: End before events
        float Timer = PS->EndObjective();

        OnObjectiveComplete.Broadcast(PC);
        GM->ScoreTrialObjective(this, Timer, PC);
    }
}

void ATrialsObjectiveInfo::DisableObjective(APlayerController* PC, bool bDeActivate /*= false*/)
{
    if (PC == nullptr) return;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
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

void ATrialsObjectiveInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsObjectiveInfo, TopRecords);
    DOREPLIFETIME(ATrialsObjectiveInfo, RecordTime);
    DOREPLIFETIME(ATrialsObjectiveInfo, AvgRecordTime);
}