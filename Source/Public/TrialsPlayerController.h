#pragma once

#include "STrialsRecordsMenu.h"
#include "TrialsPlayerState.h"

#include "TrialsPlayerController.generated.h"

class STrialsRecordsMenu;

USTRUCT()
struct FCheckpointData
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    int32 Health;

    UPROPERTY()
    int32 Armor;
};

UCLASS()
class ATrialsPlayerController : public AUTPlayerController
{
    GENERATED_BODY()

public:
    TSharedPtr<class STrialsRecordsMenu> RecordsMenu;

    // Ghost manager that is currently active for this player.
    UPROPERTY()
    class ATrialsGhostReplay* GhostPlayback;

    // Current ghost data that was RECORDED. Available when actively playing an objective.
    UPROPERTY()
    class UUTGhostData* RecordedGhostData;

    // Current ghost data that is being RECORDED. Available when actively playing an objective.
    UPROPERTY()
    class UUTGhostData* RecordingGhostData;

    // Temporary reference to the player's ghost data of the last scored objective run.
    UPROPERTY()
    class UUTGhostData* ScoredGhostData;

    UPROPERTY(Replicated)
    uint32 bHasScoredReplayData : 1;

    void SetupInputComponent() override;
    void Destroyed() override;

    void ServerSuicide_Implementation() override;
    void ServerRestartPlayer_Implementation() override;

    UFUNCTION(Exec)
    virtual void RequestRestart();

    UFUNCTION(Server, Reliable, WithValidation)
    virtual void ServerRequestRestart();

    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = UI)
    void OpenRecordsMenu(const FString& MapName, const FString& ObjName)
    {
        if (RecordsMenu.IsValid()) 
        {
            GEngine->GameViewport->RemoveViewportWidgetContent(RecordsMenu.ToSharedRef());
            RecordsMenu = nullptr;
            return;
        }
        RecordsMenu = SNew(STrialsRecordsMenu)
            .PlayerOwner(GetUTLocalPlayer())
            .MapName(MapName)
            .ObjName(ObjName);
        GEngine->GameViewport->AddViewportWidgetContent(RecordsMenu.ToSharedRef());
    }

    UFUNCTION(Exec)
    void ShowRecords()
    {
        if (!Cast<ATrialsPlayerState>(PlayerState)->ActiveObjective)
        {
            return;
        }
        OpenRecordsMenu(GetWorld()->GetMapName(), Cast<ATrialsPlayerState>(PlayerState)->ActiveObjective->RecordId);
    }

    void FetchObjectiveGhostData(ATrialsObjective* Objective, const TFunction<void(class UUTGhostData* GhostData)> OnResult);
    void StartRecordingGhostData();
    void StopRecordingGhostData();

    void ViewGhostPlayback(class UUTGhostData* GhostData);

    UFUNCTION()
    void OnEndGhostPlayback();

    void SummonGhostPlayback(class UUTGhostData* GhostData);
    void StopGhostPlayback(bool bDeActivate);

    /**
     * Handles logic for starting an objective, this should never be called directly! See ATrialsObjectiveInfo::ActivateObjective!
     */
    void StartObjective(ATrialsObjective* Objective);

    /**
    * Handles logic for ending an objective, this should never be called directly! See ATrialsObjectiveInfo::DisableObjective!
    */
    void EndObjective(ATrialsObjective* Objective, bool bDeActivate);

    void ScoredObjective(ATrialsObjective* Objective);

    UPROPERTY(BlueprintReadOnly, Category = Checkpoint)
    AActor* CheckpointDest;

    UPROPERTY()
    FCheckpointData CheckpointData;

    UFUNCTION(BlueprintCallable, Category = Checkpoint)
    void SetCheckpoint(AActor* Dest);

    void UseCheckpoint(AUTCharacter* NewChar);

private:
    FTimerHandle ViewGhostPlaybackTimerHandle;

};