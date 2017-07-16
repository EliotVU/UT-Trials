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
        RecordsAPI->GetMap(MapName, [this](FMapInfo& MapInfo) {
            CurrentMapInfo = MapInfo;
            APIReady();
        });
    });
}

void ATrialsGameMode::APIReady()
{
    UE_LOG(UT, Log, TEXT("Records API is Ready!"));
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

        RecordsAPI->LoginPlayer(PS->UniqueId->ToString(), PS->PlayerName, [this, PS](const FPlayerInfo& Result)
        {
            PS->PlayerNetId = Result._id;
            UE_LOG(UT, Log, TEXT("Logged in player %s from country %s"), *Result.Name, *Result.CountryCode);

            // localhost:8080/api/maps/STR-Temple/players/5944211eaeaaeccdf6f782de
            RecordsAPI->Fetch(TEXT("api/maps/") 
                + FGenericPlatformHttp::UrlEncode(GetWorld()->GetMapName()) 
                + TEXT("/players/") 
                + FGenericPlatformHttp::UrlEncode(PS->PlayerNetId), 
                [this, PS](const FAPIResult& Data)
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
    auto* Char = Cast<ACharacter>(PlayerPawn);
    if (Char)
    {
        Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    }
    //PlayerPawn->bCanBeDamaged = false;
    Super::SetPlayerDefaults(PlayerPawn);
}

bool ATrialsGameMode::AllowSuicideBy(AUTPlayerController* PC)
{
    return PC->GetPawn() != nullptr && GetWorld()->TimeSeconds - PC->GetPawn()->CreationTime > 3.0f;
}

// Copied from Base, needed a playerstart finder that ignores geometry collision.
AActor* ATrialsGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Choose a player start
    APlayerStart* FoundPlayerStart = nullptr;
    UClass* PawnClass = GetDefaultPawnClassForController(Player);
    APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
    TArray<APlayerStart*> StartPoints;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* PlayerStart = *It;
        if (PlayerStart->IsA<APlayerStartPIE>())
        {
            // Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
            FoundPlayerStart = PlayerStart;
            break;
        }

        // Ignore our objective playerstarts, bIgnoreInNonTeamGame is always set to true on BeginPlay.
        if (PlayerStart->IsA<AUTPlayerStart>() && Cast<AUTPlayerStart>(PlayerStart)->bIgnoreInNonTeamGame)
        {
            continue;
        }

        StartPoints.Add(PlayerStart);
    }
    if (FoundPlayerStart == nullptr)
    {
        if (StartPoints.Num() > 0)
        {
            FoundPlayerStart = StartPoints[FMath::RandRange(0, StartPoints.Num() - 1)];
        }
    }
    return FoundPlayerStart;
}

AActor* ATrialsGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    const auto* PS = Cast<ATrialsPlayerState>(Player->PlayerState);

    // Prefer HUB spawns(a PlayerStart with PlayerStartTag=HUB) if player has unlocked any objectives.
    const FString& NewIncomingName = IncomingName.IsEmpty() && PS->UnlockedObjectives.Num() > 0 ? TEXT("HUB") : IncomingName;

    if (PS->ActiveObjectiveInfo != nullptr)
    {
        return PS->ActiveObjectiveInfo->GetPlayerSpawn(Player);
    }
    // Otherwise use any playerstart but those with a tag.
    return Super::FindPlayerStart_Implementation(Player, NewIncomingName);
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