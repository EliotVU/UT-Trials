#pragma once

#include "GameFramework/Info.h"

#include "TrialsObjectiveInfo.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveComplete, AUTPlayerController*, PC);

/**
 * Defines an objective in your Trials map. 
 * Use ActivateObjective() to activate this objective for a player, use CompleteObjective() to score, and DisableObjective() to de-activate the objective.
 * Or use the pre-built BP_TrialsProximityObjective with a link to your TrialsObjectiveInfo instance
 * - along with a BP_Trials_Objective_Shield to activate the objective.
 */
UCLASS()
class TRIALS_API ATrialsObjectiveInfo : public AInfo
{
	GENERATED_UCLASS_BODY()

public:
    /* A title to be displayed to players. 
     * Note: This actor's name will be used to reference records. 
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
    FText Title;

    /* An optional description for the objective. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
    FText Description;

    /* A PlayerStart for those who have died during this objective. */
    UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Objective)
	AUTPlayerStart* PlayerStart;

    float RecordTime;
    float AvgRecordTime; // set at runtime

    /* Your local objective time record. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Record)
    float DevRecordTime;

    virtual void BeginPlay() override;
    virtual AUTPlayerStart* GetPlayerSpawn(AController* Player);

    /* True if the objective has been activated, regardless of the timer's state. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsEnabled(AUTPlayerController* PC);

    /* True if player has activated this objective and if the Timer is running. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsActive(AUTPlayerController* PC);

    /* Activates(if disabled) and starts a timer for player. When active, GetPlayerSpawn() will be used to spawn the player. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual void ActivateObjective(AUTPlayerController* PC);

    /* Completes the objective for player. When called, OnCompleteObjective will be emitted. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual void CompleteObjective(AUTPlayerController* PC);

    /* Disables and stops the timer for player. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual void DisableObjective(AUTPlayerController* PC, bool bDeActivate = false);

    /* Fired when this objective has been completed. Fired by CompleteObjective()*/
    UPROPERTY(BlueprintAssignable, Category = Objective)
    FObjectiveComplete OnObjectiveComplete;
};

