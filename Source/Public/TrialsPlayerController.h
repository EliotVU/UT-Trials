#pragma once

#include "STrialsRecordsMenu.h"

#include "TrialsPlayerController.generated.h"

class STrialsRecordsMenu;

UCLASS()
class ATrialsPlayerController : public AUTPlayerController
{
    GENERATED_BODY()

    TSharedPtr<class STrialsRecordsMenu> RecordsMenu;

    void ServerSuicide_Implementation() override;

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
};