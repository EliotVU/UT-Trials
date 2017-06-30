#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsObjective.h"

#include "UTHUDWidget.h"
#include "UTATypes.h"
#include "TrialsPlayerState.h"

ATrialsObjective::ATrialsObjective(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetReplicates(true);
    bAlwaysRelevant = true;
    NetPriority = 1.0;
}