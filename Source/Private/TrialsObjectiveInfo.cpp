#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsObjectiveSetMessage.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "TrialsAPI.h"

ATrialsObjectiveInfo::ATrialsObjectiveInfo(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;
}

void ATrialsObjectiveInfo::BeginPlay()
{
    if (GetWorld()->GetAuthGameMode<ATrialsGameMode>() == nullptr)
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
    auto ObjName = GetActorLabel();
    auto ObjTitle = Title;

    auto* API = GetAPI();
    API->Fetch(TEXT("api/maps/") + FGenericPlatformHttp::UrlEncode(MapName) + TEXT("/") + FGenericPlatformHttp::UrlEncode(ObjName) + TEXT("?create=1"), 
        [this](const FAPIResult& Data) {
            ATrialsAPI::FromJSON(Data, &ObjInfo);

            if (ObjInfo.Records.Num() == 0) {

            }

            // Acquire last cached record time.
            RecordTime = ATrialsPlayerState::RoundTime(ObjInfo.RecordTime > 0.f ? ObjInfo.RecordTime : DevRecordTime);
        });
}

void ATrialsObjectiveInfo::ScoreRecord(float Record, AUTPlayerController* PC)
{
    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (Record < RecordTime)
    {
        RecordTime = Record;
    }

    ScorerPS->ObjectiveRecordTime = Record;

    auto ObjId = ObjInfo._id;
    check(!ObjId.IsEmpty())

    FRecordInfo RecInfo;
    RecInfo.Value = Record;
    RecInfo.PlayerId = ScorerPS->PlayerInfo._id;
    check(!RecInfo.PlayerId.IsEmpty())

    auto* API = GetAPI();
    API->Post(TEXT("api/recs/") + FGenericPlatformHttp::UrlEncode(ObjId), 
        ATrialsAPI::ToJSON(RecInfo),
        [this](const FAPIResult& Data) {
            FRecordInfo RecInfo;
            ATrialsAPI::FromJSON(Data, &RecInfo);
        });
}

AUTPlayerStart* ATrialsObjectiveInfo::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjectiveInfo::ActivateObjective(AUTPlayerController* PC)
{
    if (PC == nullptr)
    {
        return;
    }
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS->ActiveObjectiveInfo != this)
    {
        PS->SetObjective(this);
        PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 0, PS, nullptr, this);
    }
    PS->StartObjectiveTimer();
}

void ATrialsObjectiveInfo::CompleteObjective(AUTPlayerController* PC)
{
    if (PC == nullptr) return;

    // We don't want to complete an objective for clients whom have already completed or are doing a different objective.
    auto* CompleterPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (CompleterPS == nullptr || !CompleterPS->IsObjectiveTimerActive() || CompleterPS->ActiveObjectiveInfo != this)
    {
        return;
    }

    float Timer = CompleterPS->EndObjectiveTimer();
    CompleterPS->LastScoreObjectiveTimer = Timer;

    auto* GM = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
    if (GM != nullptr)
    {
        OnObjectiveComplete.Broadcast(PC);
        GM->ScoreTrialObjective(this, Timer, PC);
    }
}

void ATrialsObjectiveInfo::DisableObjective(AUTPlayerController* PC, bool bDeActivate /*= false*/)
{
    if (PC == nullptr)
    {
        return;
    }
    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (bDeActivate && PS->ActiveObjectiveInfo == this)
    {
        PS->SetObjective(nullptr);
        PC->ClientReceiveLocalizedMessage(UTrialsObjectiveSetMessage::StaticClass(), 1, PS, nullptr, this);
    }
    PS->EndObjectiveTimer();
}

bool ATrialsObjectiveInfo::IsEnabled(AUTPlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS->ActiveObjectiveInfo == this;
}

bool ATrialsObjectiveInfo::IsActive(AUTPlayerController* PC)
{
    if (PC == nullptr) return false;

    auto* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return PS->ActiveObjectiveInfo == this && PS->IsObjectiveTimerActive();
}
