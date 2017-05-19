#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsPlayerState.h"
#include "TrialsHUD.h"
#include "TrialsObjectiveCompleteMessage.h"

ATrialsGameMode::ATrialsGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ATrialsGameState::StaticClass();
	PlayerStateClass = ATrialsPlayerState::StaticClass();
    HUDClass = ATrialsHUD::StaticClass();
}

bool ATrialsGameMode::AllowSuicideBy(AUTPlayerController* PC)
{
    ATrialsPlayerState* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    return ScorerPS->ActiveObjectiveInfo != nullptr || Super::AllowSuicideBy(PC);
}

AActor* ATrialsGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    // TODO: If reached Hub once, give player a spawn with Tag "Hub".
    // TODO: If player has hub and chosen a objective, spawn player at the objective's set PlayerStart.
    ATrialsPlayerState* PS = Cast<ATrialsPlayerState>(Player->PlayerState);
    if (PS->ActiveObjectiveInfo != nullptr)
    {
        return PS->ActiveObjectiveInfo->GetPlayerSpawn(Player);
    }
    return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void ATrialsGameMode::ScoreTrialObjective(AUTPlayerController* PC, ATrialsObjectiveInfo* objInfo)
{
    if (PC == nullptr || objInfo == nullptr) return;

    // We don't want to complete an objective for clients whom have already completed or are doing a different objective.
    ATrialsPlayerState* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);
    if (!ScorerPS->bIsObjectiveTimerActive || ScorerPS->ActiveObjectiveInfo != objInfo) return;

    int32 msgSwitch;
    float elapsedTime = ScorerPS->EndObjectiveTimer();
    // TODO: do record time checks here
    if (elapsedTime < objInfo->RecordTime)
    {
        // New top record!
        msgSwitch = 0;
    }
    else
    {
        // tie or slower record...
        msgSwitch = 1;
    }

    // ...New time!
    BroadcastLocalized(this, UTrialsObjectiveCompleteMessage::StaticClass(), msgSwitch, ScorerPS, nullptr, objInfo);
}
