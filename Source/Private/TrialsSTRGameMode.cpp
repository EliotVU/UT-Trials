#include "Trials.h"
#include "TrialsSTRGameMode.h"

#include "TrialsPlayerState.h"

ATrialsSTRGameMode::ATrialsSTRGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DisplayName = NSLOCTEXT("TrialsGameMode", "STR", "Solo Trials");
    MapPrefix = TEXT("STR");
    bWeaponStayActive = true;
    bDelayedStart = false;
    StartDelay = 3.0;
}

void ATrialsSTRGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
    Super::FinishRestartPlayer(NewPlayer, StartRotation);
    // We end the objective on spawn over death, so that an objective can still be completed by projectiles during a player's death.
    auto* PS = Cast<ATrialsPlayerState>(NewPlayer->PlayerState);
    if (PS && PS->ActiveObjectiveInfo != nullptr)
    {
        PS->EndObjective();
    }

    // EXPLOIT: Cleanup any projectile that was fired before this player's death.
    for (TActorIterator<AUTProjectile> It(GetWorld()); It; ++It)
    {
        if (It->InstigatorController == NewPlayer)
        {
            It->Destroy();
        }
    }
}

bool ATrialsSTRGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
    if (InstigatedBy != nullptr && InstigatedBy != Injured->Controller && UTGameState->OnSameTeam(InstigatedBy, Injured))
    {
        // Remove team boosting.
        Momentum = FVector::ZeroVector;
        Damage = 0; // Although weapons shoot through team mates, radius damage could still be dealt to team mates.
    }
    Super::ModifyDamage_Implementation(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
    return true;
}

bool ATrialsSTRGameMode::CheckRelevance_Implementation(AActor* Other)
{
    // Never allow any drops in STR games.
    if (Other->IsA<AUTInventory>())
    {
        Cast<AUTInventory>(Other)->DroppedPickupClass = nullptr;
        Cast<AUTInventory>(Other)->bAlwaysDropOnDeath = false;
    }
    return Super::CheckRelevance_Implementation(Other);
}
