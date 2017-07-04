#include "Trials.h"
#include "TrialsTimerState.h"

#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "UnrealNetwork.h"

ATrialsPlayerState::ATrialsPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), ActiveObjectiveInfo(nullptr), TimerState(nullptr), ObjectiveRecordTime(0.0)
{
}

// TODO: Fetch unlocked objectives from API.
void ATrialsPlayerState::RegisterUnlockedObjective(ATrialsObjectiveInfo* Objective)
{
    check(Objective);
    if (UnlockedObjectives.Contains(Objective))
    {
        return;
    }
    UnlockedObjectives.Add(Objective);
    // Simulate for offline play.
    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        OnRep_UnlockedObjectives();
    }
}

void ATrialsPlayerState::OnRep_UnlockedObjectives()
{
    for (auto& Obj : UnlockedObjectives)
    {
        Obj->SetLocked(false);
    }
}


float ATrialsPlayerState::StartObjective() const
{
    check(TimerState);
    return TimerState->StartTimer();
}

float ATrialsPlayerState::EndObjective() const
{
    check(TimerState);
    return TimerState->EndTimer();
}

void ATrialsPlayerState::SetObjective(ATrialsObjectiveInfo* Obj)
{
    if (Obj == ActiveObjectiveInfo)
    {
        return;
    }

    ObjectiveRecordTime = 0.0;
    ActiveObjectiveInfo = Obj;
    ForceNetUpdate();

    if (ActiveObjectiveInfo != nullptr)
    {
        if (TimerState == nullptr)
        {
            TimerState = static_cast<ATrialsTimerState*>(GetWorld()->SpawnActor(ATrialsTimerState::StaticClass()));
        }

        check(TimerState);
        TimerState->Objective = Obj;
        TimerState->OwnerRecordTime = 0.0;
        TimerState->ForceNetUpdate();

        // Note: assumes that the activated objective has fetched its info!
        auto _ObjId = ActiveObjectiveInfo->ObjectiveNetId;

        auto* API = GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
        API->Fetch(TEXT("api/recs/") + FGenericPlatformHttp::UrlEncode(_ObjId) + TEXT("/") + FGenericPlatformHttp::UrlEncode(PlayerNetId), [this, Obj](const FAPIResult& Data)
        {
            // Player may have switched active objective during this request.
            if (ActiveObjectiveInfo != Obj)
            {
                return;
            }

            FRecordInfo RecInfo;
            ATrialsAPI::FromJSON(Data, &RecInfo);

            float RecordTime = RecInfo.Value;
            UpdateRecordTime(RecordTime);
        });
    }
}

void ATrialsPlayerState::UpdateRecordTime(float Time)
{
    ObjectiveRecordTime = Time;
    ForceNetUpdate();

    if (TimerState != nullptr)
    {
        TimerState->OwnerRecordTime = Time;
        TimerState->ForceNetUpdate();
    }
}

void ATrialsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATrialsPlayerState, ActiveObjectiveInfo);
    DOREPLIFETIME(ATrialsPlayerState, TimerState);
    DOREPLIFETIME(ATrialsPlayerState, ObjectiveRecordTime);
    DOREPLIFETIME_CONDITION(ATrialsPlayerState, UnlockedObjectives, COND_OwnerOnly);
}