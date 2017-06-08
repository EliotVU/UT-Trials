#pragma once

#include "Trials.h"
#include "TrialsObjectiveInfo.h"
#include "TrialsAPI.h"

#include "TrialsGameMode.generated.h"

UCLASS(Blueprintable, Meta = (ChildCanTick), Config = Trials)
class ATrialsGameMode : public AUTGameMode
{
    GENERATED_UCLASS_BODY()

    FMapInfo* CurrentMapInfo;

public:
    ATrialsAPI* RecordsAPI;

    UPROPERTY(Config)
    FString RecordsBaseURL;

    UPROPERTY(Config)
    FString RecordsAPIToken;

    virtual void BeginPlay() override;
    virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
    virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
    virtual bool ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType) override;
    virtual bool AllowSuicideBy(AUTPlayerController* PC) override;
    virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;
    virtual void DiscardInventory(APawn* Other, AController* Killer) override;

    virtual void ScoreTrialObjective(ATrialsObjectiveInfo* objInfo, float Timer, AUTPlayerController* PC);
};