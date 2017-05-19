#pragma once

#include "GameFramework/Info.h"
#include "TrialsObjective.h"
#include "TrialsObjectiveInfo.generated.h"

/**
 * 
 */
UCLASS()
class TRIALS_API ATrialsObjectiveInfo : public AInfo
{
	GENERATED_UCLASS_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
	ATrialsObjective* Objective;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
	AUTPlayerStart* PlayerStart;

    float RecordTime;
    float AvgRecordTime; // set at runtime

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Record)
    float DevRecordTime;

    virtual void BeginPlay() override;
    virtual AUTPlayerStart* GetPlayerSpawn(AController* Player);

    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual void ObjectiveTriggered(AUTPlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsEnabled(AUTPlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = Objective)
    virtual bool IsActive(AUTPlayerController* PC);
};

