#pragma once

#include "STrialsRecordsMenu.h"
#include "TrialsPlayerState.h"

#include "TrialsPlayerController.generated.h"

class STrialsRecordsMenu;

UCLASS()
class ATrialsPlayerController : public AUTPlayerController
{
    GENERATED_BODY()

public:
    TSharedPtr<class STrialsRecordsMenu> RecordsMenu;

    ATrialsPlayerState* TPlayerState;

    // Ghost manager that is currently active for this player.
    UPROPERTY()
    class ATrialsGhostReplay* GhostPlayback;

    // Current ghost data that was RECORDED. Available when actively playing an objective.
    UPROPERTY()
    class UUTGhostData* RecordedGhostData;

    // Current ghost data that is being RECORDED. Available when actively playing an objective.
    UPROPERTY()
    class UUTGhostData* RecordingGhostData;

    // Temporary reference to the player's ghost data of last scored scored objective run.
    UPROPERTY()
    class UUTGhostData* ScoredGhostData;

    void SetupInputComponent() override;
    void Destroyed() override;
    void OnRep_PlayerState() override;

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
            .PlayerOwner(GetUTLocalPlayer());
        GEngine->GameViewport->AddViewportWidgetContent(RecordsMenu.ToSharedRef());
    }

    UFUNCTION(Exec)
    void ShowRecords()
    {
        OpenRecordsMenu(FString(), FString());
    }

    void StartRecordingGhostData();
    void StopRecordingGhostData();

    void ViewGhostPlayback(class UUTGhostData* GhostData);
    void OnEndGhostPlayback();
    void SummonGhostPlayback(class UUTGhostData* GhostData);
    void StopGhostPlayback(bool bDeActivate);

    void FetchObjectiveGhostData(ATrialsObjective* Objective, const TFunction<void(class UUTGhostData* GhostData)> OnResult);

    void ScoredObjective(ATrialsObjective* Objective);

private:
    FTimerHandle ViewGhostPlaybackTimerHandle;

};