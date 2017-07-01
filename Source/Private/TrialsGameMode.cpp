#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"
#include "TrialsHUD.h"
#include "TrialsObjectiveCompleteMessage.h"
#include "TrialsAPI.h"

ATrialsGameMode::ATrialsGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PlayerControllerClass = ATrialsPlayerController::StaticClass();
    GameStateClass = ATrialsGameState::StaticClass();
    PlayerStateClass = ATrialsPlayerState::StaticClass();
    HUDClass = ATrialsHUD::StaticClass();

    TimeLimit = 0;
    RespawnWaitTime = 0;
    bAllowOvertime = false;
    MapPrefix = TEXT("STR");
    bTrackKillAssists = false;
}

void ATrialsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // Authenticate this server.
    RecordsAPI = Cast<ATrialsAPI>(GetWorld()->SpawnActor(ATrialsAPI::StaticClass()));
    RecordsAPI->Authenticate(RecordsBaseURL, RecordsAPIToken, ServerNameOverride, [this, MapName]() {
        UE_LOG(UT, Log, TEXT("Records API is Ready!"));
        
        RecordsAPI->GetMap(MapName, [this](FMapInfo& MapInfo) {
            CurrentMapInfo = MapInfo;
            APIReady();
        });
    });
}

void ATrialsGameMode::APIReady()
{
    bAPIAuthenticated = true;

    for (TActorIterator<ATrialsObjectiveInfo> It(GetWorld()); It; ++It)
    {
        It->InitData(CurrentMapInfo.Name);
    }

}

void ATrialsGameMode::BeginPlay()
{
    Super::BeginPlay();
}

void ATrialsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    auto* PS = Cast<ATrialsPlayerState>(NewPlayer->PlayerState);
    if (PS != nullptr)
    {
        FLoginInfo LoginInfo;
        LoginInfo.ProfileId = PS->UniqueId->ToString();
        LoginInfo.Name = PS->PlayerName;

        RecordsAPI->Post(TEXT("api/players/login"), ATrialsAPI::ToJSON(LoginInfo), [this, PS](const FAPIResult& Data) {
            ATrialsAPI::FromJSON(Data, &PS->PlayerInfo);
            UE_LOG(UT, Log, TEXT("Logged in player %s from country %s"), *PS->PlayerInfo.Name, *PS->PlayerInfo.CountryCode);
        });
    }
}

void ATrialsGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
    //PlayerPawn->bCanBeDamaged = false;
    Super::SetPlayerDefaults(PlayerPawn);
}

void ATrialsGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
    Super::FinishRestartPlayer(NewPlayer, StartRotation);
    // We end the objective on spawn over death, so that an objective can still be completed by projectiles during a player's death.
    auto* PS = Cast<ATrialsPlayerState>(NewPlayer->PlayerState);
    if (PS && PS->ActiveObjectiveInfo != nullptr)
    {
        // TODO: Replicate, not simulated here.
        PS->EndObjectiveTimer();
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

bool ATrialsGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
    if (InstigatedBy != nullptr && InstigatedBy != Injured->Controller && UTGameState->OnSameTeam(InstigatedBy, Injured))
    {
        // Remove team boosting.
        Momentum = FVector::ZeroVector;
        Damage = 0; // Although weapons shoot through team mates, radius damage could still be dealt to team mates.
        AUTPlayerController* InstigatorPC = Cast<AUTPlayerController>(InstigatedBy);
        if (InstigatorPC && Cast<AUTPlayerState>(Injured->PlayerState))
        {
            ((AUTPlayerState *)(Injured->PlayerState))->AnnounceSameTeam(InstigatorPC);
        }
    }
    Super::ModifyDamage_Implementation(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
    return true;
}

bool ATrialsGameMode::AllowSuicideBy(AUTPlayerController* PC)
{
    return PC->GetPawn() != nullptr && GetWorld()->TimeSeconds - PC->GetPawn()->CreationTime > 1.25f;
}

AActor* ATrialsGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    // TODO: If reached Hub once, give player a spawn with Tag "Hub".
    auto* PS = Cast<ATrialsPlayerState>(Player->PlayerState);
    if (PS->ActiveObjectiveInfo != nullptr)
    {
        return PS->ActiveObjectiveInfo->GetPlayerSpawn(Player);
    }
    return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

// EXPLOIT: Don't drop any items, only discard.
// FIXME: Players can still drop items via feign death and possibly other methods, but there is no option to disable dropping provided by Epic.
void ATrialsGameMode::DiscardInventory(APawn* Other, AController* Killer)
{
    AUTCharacter* UTC = Cast<AUTCharacter>(Other);
    if (UTC != nullptr)
    {
        UTC->DiscardAllInventory();
    }
}

void ATrialsGameMode::ScoreTrialObjective(ATrialsObjectiveInfo* Obj, float Timer, AUTPlayerController* PC)
{
    check(Obj);
    check(PC);

    auto* ScorerPS = Cast<ATrialsPlayerState>(PC->PlayerState);

    int32 RecordSwitch;
    float RecordTime = Obj->RecordTime;
    if (Timer < RecordTime || RecordTime == 0.00) // New all time
    {
        // New top record!
        RecordSwitch = 0;
        Obj->ScoreRecord(Timer, PC);
    }
    else if (Timer == RecordTime) // Tied with all time
    {
        // tie or slower record...
        RecordSwitch = 1;
    }
    else // worse, check personal time
    {
        RecordTime = ScorerPS->ObjectiveRecordTime;
        // New or first personal record
        if (RecordTime == 0.00)
        {
            RecordSwitch = 3;
            Obj->ScoreRecord(Timer, PC);
        }
        else if (Timer < RecordTime)
        {
            RecordSwitch = 4;
            Obj->ScoreRecord(Timer, PC);
        }
        else if (Timer == RecordTime)
        {
            RecordSwitch = 1;
        }
        else
        {
            RecordSwitch = 2;
        }
    }


    // ...New time?!
    BroadcastLocalized(this, UTrialsObjectiveCompleteMessage::StaticClass(), RecordSwitch, ScorerPS, nullptr, Obj);

    // TODO: Add record event here
}