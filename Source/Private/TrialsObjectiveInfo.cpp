#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsObjectiveSetMessage.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"

ATrialsObjectiveInfo::ATrialsObjectiveInfo(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;

    // N/A
    DevRecordTime = -1;
    RecordTime = -1;
    AvgRecordTime = -1;
}

void ATrialsObjectiveInfo::BeginPlay()
{
    RecordTime = DevRecordTime;
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
    auto* GM = GetWorld()->GetAuthGameMode<ATrialsGameMode>();
    if (GM != nullptr)
    {
        GM->ScoreTrialObjective(PC, this);
        OnCompleteObjective(PC);
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
