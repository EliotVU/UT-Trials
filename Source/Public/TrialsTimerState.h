#pragma once

#include "TrialsObjectiveInfo.h"

#include "TrialsTimerState.generated.h"

UENUM()
enum ETimerState
{
    TS_Idle,
    TS_Active,
    TS_Complete
};

UCLASS()
class ATrialsTimerState : public AInfo
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(Replicated, BlueprintReadOnly)
    ATrialsObjective* Objective;

    UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly)
    TEnumAsByte<enum ETimerState> State;

    // The record time when this timer was started
    float EndRecordTime; 

    UPROPERTY(Replicated)
    float OwnerRecordTime;

    UFUNCTION()
    void OnRep_State();

    UFUNCTION(BlueprintCallable, Category = Timer)
    float GetTimer() const;

    UFUNCTION(BlueprintCallable, Category = Timer)
    float GetRemainingTime() const;

    // Always race our own record time when possible.
    UFUNCTION(BlueprintCallable, Category = Timer)
    float GetRecordTime() const;

    float StartTimer();
    float StopTimer();
    float EndTimer();

    UFUNCTION(BlueprintCallable, Category = Timer)
    static float RoundTime(const float Seconds);

    UFUNCTION(BlueprintCallable, Category = HUD)
    static FText FormatTime(const float Value);

    UFUNCTION(BlueprintCallable, Category = HUD)
    static FLinearColor GetTimerColor(const float Timer);

    static const FLinearColor LeadColor;
    static const FLinearColor ActiveColor;
    static const FLinearColor IdleColor;
    static const FLinearColor PositiveColor;
    static const FLinearColor NegativeColor;

    void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);

        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("Net %s"), *UEnum::GetValueAsString(TEXT("Engine.ENetMode"), GetWorld()->GetNetMode())));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("State %s"), *UEnum::GetValueAsString(TEXT("Trials.ETimerState"), State)));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("Rec %f"), GetRecordTime()));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("End Rec %f"), EndRecordTime));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("TIMER %f"), GetTimer()));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("REM %f"), GetRemainingTime()));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("Start %f"), StartTime));
        //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::White, FString::Printf(TEXT("End %f"), EndTime));
    }

    void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

private:
    float StartTime;
    float EndTime;
};