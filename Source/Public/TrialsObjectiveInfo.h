#pragma once

#include "GameFramework/Info.h"
#include "TrialsAPI.h"

#include "TrialsObjectiveInfo.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRecordScored, AUTPlayerController*, Player, float, Time, bool, IsTopRecord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveComplete, AUTPlayerController*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveLockChange, bool, IsLocked);

/**
 * Defines an objective in your Trials map. 
 * Use ActivateObjective() to activate this objective for a player, use CompleteObjective() to score, and DisableObjective() to de-activate the objective.
 * Or use the pre-built BP_TrialsProximityObjective with a link to your TrialsObjectiveInfo instance
 * - along with a BP_Trials_Objective_Shield to activate the objective.
 */
UCLASS(DisplayName = "Trials Objective")
class TRIALS_API ATrialsObjective : public AInfo
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(Instanced, EditAnywhere, Category = "Camera")
    UCameraComponent* Camera;

    /**
     * A title to be displayed to players. 
     * Note: This actor's name will be used to reference records. 
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = ObjectiveSummary)
    FText Title;

    /* An optional description for the objective. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = ObjectiveSummary)
    FText Description;

    /** A shot of the objective's area. To be used within menus. */
    UPROPERTY(EditInstanceOnly, Category = ObjectiveSummary)
    UTexture2D* Screenshot;

    /**
     * If set, objective will be locked if set objective is not unlocked (on a player basis).
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Objective)
    ATrialsObjective* RequisiteObjective;

    /* Objectives that are held locked by this objective. */
    UPROPERTY()
    TArray<ATrialsObjective*> LockedObjectives;

    /**
    * Inventory to give to player when this objective activates.
    *
    * Note: When a player disables this objective he or she will be given an entire new Pawn as an anti-cheat measure.
    */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
    TArray< TSubclassOf<AUTInventory> > PlayerInventory;

    /**
    * This represents the id that will be used when storing records remotely.
    * You can modify this id by changing the actor's label.
    */
    UPROPERTY(VisibleInstanceOnly, Category = Record)
    FString RecordId;

    FString ObjectiveNetId;

    // Current top record's ghost data.
    UPROPERTY()
    class UUTGhostData* RecordGhostData;

    /* Your local objective time record. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Record)
    float DevRecordTime;

    UPROPERTY(Replicated)
    TArray<FRecordInfo> TopRecords;

    UPROPERTY(Replicated)
    float RecordTime;

    UPROPERTY(Replicated)
    float AvgRecordTime;

    UPROPERTY(Replicated)
    uint32 bCanSubmitRecords : 1;

    void BeginPlay() override;

    void UpdateRecordState(FString& MapName);
    void ScoreRecord(float Record, AUTPlayerController* PC);

    /**
     * Broadcasts when a player has set a new/improved personal or top record.
     */
    UPROPERTY(BlueprintAssignable, Category = Record, BlueprintAuthorityOnly)
    FRecordScored OnRecordScored;

    virtual AUTPlayerStart* GetPlayerSpawn(AController* Player);

    /* True if the objective has been activated, regardless of the timer's state. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsEnabled(APlayerController* PC);

    /* True if player has activated this objective and if the Timer is running. */
    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsActive(APlayerController* PC);

    /* Activates(if disabled) and starts a timer for player. When active, GetPlayerSpawn() will be used to spawn the player. */
    UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
    virtual void ActivateObjective(APlayerController* PC);

    /* Completes the objective for player. When called, OnCompleteObjective will be emitted. */
    UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
    virtual void CompleteObjective(AUTPlayerController* PC);

    /* Disables and stops the timer for player. */
    UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
    virtual void DisableObjective(APlayerController* PC, bool bDeActivate = false);

    /* Fired when this objective has been completed. Fired by CompleteObjective()*/
    UPROPERTY(BlueprintAssignable, Category = Objective, BlueprintAuthorityOnly)
    FObjectiveComplete OnObjectiveComplete;

    UFUNCTION()
    void OnRequisiteCompleted(AUTPlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsLocked(APlayerController* PC);

    virtual void SetLocked(bool bIsLocked);

    /* Fired when this objective has been completed. Fired by CompleteObjective()*/
    UPROPERTY(BlueprintAssignable, Category = Objective)
    FObjectiveLockChange OnLockedChange;

#ifdef WITH_EDITOR
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void PostRegisterAllComponents() override;
    void CheckForErrors() override;
#endif

protected:
    uint32 bLockedLocale : 1;

    ATrialsAPI* GetAPI() const;

    /* A PlayerStart for those who have died during this objective. */
    UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Objective)
    AUTPlayerStart* PlayerStart;
};

