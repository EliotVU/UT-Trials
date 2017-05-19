#include "Trials.h"
#include "TrialsPlayerState.h"

ATrialsPlayerState::ATrialsPlayerState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void ATrialsPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}