#include "Trials.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"

void ATrialsPlayerController::ServerSuicide_Implementation()
{
    // Try to perform an instant re-spawn with no death events and gore.
    auto* PS = Cast<ATrialsPlayerState>(PlayerState);
    auto* Char = Cast<AUTCharacter>(GetCharacter());
    if (Char != nullptr && PS->ActiveObjectiveInfo != nullptr && !GetWorld()->GetAuthGameMode<AUTGameMode>()->AllowSuicideBy(this))
    {
        // Minor anti-spam limitation.
        if (GetWorld()->TimeSeconds - Char->CreationTime < 0.2f)
        {
            return;
        }

        Char->SpawnRallyEffectAt(Char->GetActorLocation());
        Char->Reset();
        SetPawn(nullptr);
        this->ServerRestartPlayer();

        auto* NewChar = Cast<AUTCharacter>(GetCharacter());
        if (NewChar)
        {
            Char->SpawnRallyDestinationEffectAt(Char->GetActorLocation());
        }
        return;
    }
    // Normal suicide if past quick re-spawn time.
    Super::ServerSuicide_Implementation();
}
