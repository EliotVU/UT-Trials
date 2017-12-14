#pragma once

#include "TrialsGameMode.h"

#include "TrialsSTRGameMode.generated.h"

UCLASS(Blueprintable)
class TRIALS_API ATrialsSTRGameMode : public ATrialsGameMode
{
    GENERATED_UCLASS_BODY()
    
    void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
    bool ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType) override;
    bool CheckRelevance_Implementation(AActor* Other) override;
};
