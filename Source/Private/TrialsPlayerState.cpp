#include "Trials.h"
#include "TrialsTimerState.h"

#include "TrialsPlayerState.h"
#include "TrialsGameMode.h"
#include "UnrealNetwork.h"

ATrialsPlayerState::ATrialsPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), ActiveObjective(nullptr), TimerState(nullptr), ObjectiveRecordTime(0.0)
{
}

void ATrialsPlayerState::RegisterUnlockedObjective(ATrialsObjective* Objective)
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

void ATrialsPlayerState::SetObjective(ATrialsObjective* Obj)
{
    if (Obj == ActiveObjective)
    {
        return;
    }

    ObjectiveRecordTime = 0.0;
    ActiveObjective = Obj;
    ForceNetUpdate();

    if (ActiveObjective != nullptr)
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
        auto _ObjId = ActiveObjective->ObjectiveNetId;

        auto* API = GetWorld()->GetAuthGameMode<ATrialsGameMode>()->RecordsAPI;
        API->Fetch(TEXT("api/recs/") + FGenericPlatformHttp::UrlEncode(_ObjId) + TEXT("/") + FGenericPlatformHttp::UrlEncode(PlayerNetId), [this, Obj](const FAPIResult& Data)
        {
            // Player may have switched active objective during this request.
            if (ActiveObjective != Obj)
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

    DOREPLIFETIME(ATrialsPlayerState, ActiveObjective);
    DOREPLIFETIME(ATrialsPlayerState, TimerState);
    DOREPLIFETIME(ATrialsPlayerState, ObjectiveRecordTime);
    DOREPLIFETIME_CONDITION(ATrialsPlayerState, UnlockedObjectives, COND_OwnerOnly);
}