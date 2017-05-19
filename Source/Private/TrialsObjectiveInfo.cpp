#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"

ATrialsObjectiveInfo::ATrialsObjectiveInfo(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // N/A
    DevRecordTime = -1;
    RecordTime = -1;
    AvgRecordTime = -1;
}

void ATrialsObjectiveInfo::BeginPlay()
{
    RecordTime = DevRecordTime;

    if (!Objective || GetWorld()->IsNetMode(NM_DedicatedServer)) return;
    AUTHUD* HUD = Cast<AUTHUD>(GetWorld()->GetFirstPlayerController()->MyHUD);
    HUD->AddPostRenderedActor(Objective);
}

AUTPlayerStart* ATrialsObjectiveInfo::GetPlayerSpawn(AController* Player)
{
    return this->PlayerStart;
}

void ATrialsObjectiveInfo::ObjectiveTriggered(AUTPlayerController* PC)
{
    if (Role != ROLE_Authority) return;

    ATrialsGameMode* GM = Cast<ATrialsGameMode>(GetWorld()->GetAuthGameMode());
    GM->ScoreTrialObjective(PC, this);
}

bool ATrialsObjectiveInfo::IsEnabled(AUTPlayerController* PC)
{
    if (PC == nullptr) return false;
    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS == nullptr) return false;
    return PS->ActiveObjectiveInfo == this;
}

bool ATrialsObjectiveInfo::IsActive(AUTPlayerController* PC)
{
    if (PC == nullptr) return false;
    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (PS == nullptr) return false;
    return PS->ActiveObjectiveInfo == this && PS->bIsObjectiveTimerActive;
}
