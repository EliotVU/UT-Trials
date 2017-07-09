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
    bTrackKillAssists = false;

    ImpactHammerObject = FStringAssetReference(TEXT("/Trials/Weapons/BP_Trials_ImpactHammer.BP_Trials_ImpactHammer_C"));
    bClearPlayerInventory = true;
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
        It->UpdateRecordState(CurrentMapInfo.Name);
    }

}

void ATrialsGameMode::BeginPlay()
{
    Super::BeginPlay();
}

// Note: Called prior to API initialization!
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
            FPlayerInfo PlayerInfo;
            ATrialsAPI::FromJSON(Data, &PlayerInfo);

            PS->PlayerNetId = PlayerInfo._id;
            UE_LOG(UT, Log, TEXT("Logged in player %s from country %s"), *PlayerInfo.Name, *PlayerInfo.CountryCode);


            // localhost:8080/api/maps/STR-Temple/players/5944211eaeaaeccdf6f782de
            RecordsAPI->Fetch(TEXT("api/maps/") + GetWorld()->GetMapName() + TEXT("/players/") + PS->PlayerNetId, [this, PS](const FAPIResult& Data)
            {
                FPlayerObjectiveInfo UnlockedInfo;
                ATrialsAPI::FromJSON(Data, &UnlockedInfo);

                // FIXME: Loop through the objectives directly instead of targets(currently named "Objectives")
                auto& ObjTargets = Cast<ATrialsGameState>(GameState)->Objectives;
                for (const auto& Target : ObjTargets)
                {
                    if (Target == nullptr) continue;

                    bool IsCompleted = UnlockedInfo.Objs.ContainsByPredicate([Target](const FObjectiveInfo& Item)
                    {
                        return Item.Name == Target->ObjectiveInfo->RecordId;
                    });
                    if (IsCompleted)
                    {
                        PS->RegisterUnlockedObjective(Target->ObjectiveInfo);
                    }
                }
            });
        });
    }
}

void ATrialsGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
    //PlayerPawn->bCanBeDamaged = false;
    Super::SetPlayerDefaults(PlayerPawn);
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
    // Otherwise use any playerstart but those with a tag.
    return Super::FindPlayerStart_Implementation(Player, TEXT(""));
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
    BroadcastLocalized(this, UTrialsObjectiveCompleteMessage::StaticClass(), RecordSwitch, ScorerPS, nullptr, ScorerPS->TimerState);

    // TODO: Add record event here
}