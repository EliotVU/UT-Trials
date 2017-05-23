#include "Trials.h"
#include "TrialsGameState.h"
#include "TrialsObjectiveInfo.h"

ATrialsGameState::ATrialsGameState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ATrialsGameState::BeginPlay()
{
    for (TActorIterator<ATrialsObjectiveInfo> It(GetWorld()); It; ++It )
    {
        Objectives.Add(*It);
    }
}

void ATrialsGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool ATrialsGameState::AllowMinimapFor(AUTPlayerState* PS)
{
    return true;
}