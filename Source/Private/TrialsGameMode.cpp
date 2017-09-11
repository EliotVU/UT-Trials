#include "Trials.h"
#include "TrialsGameMode.h"
#include "TrialsGameState.h"
#include "TrialsPlayerController.h"
#include "TrialsPlayerState.h"
#include "TrialsHUD.h"
#include "TrialsObjectiveCompleteMessage.h"
#include "TrialsAPI.h"
#include "TrialsRecordSetMessage.h"

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

    GhostObject = FStringAssetReference(TEXT("/Trials/Blueprints/BP_Trials_GhostCharacter.BP_Trials_GhostCharacter_C"));
}

void ATrialsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    if (!GhostObject.IsNull())
    {
        GhostClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), nullptr, *GhostObject.ToStringReference().ToString(), nullptr, LOAD_NoWarn));
    }
    else
    {
        GhostClass = GetWorld()->GetAuthGameMode()->DefaultPawnClass.Get();
    }

    // Authenticate this server.
    RecordsAPI = Cast<ATrialsAPI>(GetWorld()->SpawnActor(ATrialsAPI::StaticClass()));
    RecordsAPI->Authenticate(RecordsBaseURL, RecordsAPIToken, ServerNameOverride, [this, MapName]() {
        this->GetGameState<ATrialsGameState>()->bAPIAuthenticated = true;
        RecordsAPI->GetMap(MapName, [this](FMapInfo& MapInfo) {
            CurrentMapInfo = MapInfo;
            APIReady();
        });
    });
}

void ATrialsGameMode::APIReady()
{
    UE_LOG(UT, Log, TEXT("Records API is Ready!"));
    for (TActorIterator<ATrialsObjective> It(GetWorld()); It; ++It)
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
            UE_LOG(UT, Log, TEXT("Logged in player %s from country %s"), *Result.Name, *Result.CountryCode);
            PS->PlayerNetId = Result._id;

            RecordsAPI->GetPlayerObjs(GetWorld()->GetMapName(), PS->PlayerNetId, [this, PS](const FPlayerObjectiveInfo& UnlockedInfo)
            {
                // FIXME: Loop through the objectives directly instead of targets(currently named "Objectives")
                auto& ObjTargets = Cast<ATrialsGameState>(GameState)->Targets;
                for (const auto& Target : ObjTargets)
                {
                    if (Target == nullptr) continue;

                    bool IsCompleted = UnlockedInfo.Objs.ContainsByPredicate([Target](const FObjectiveInfo& Item)
                    {
                        return Item.Name == Target->Objective->RecordId;
                    });
                    if (IsCompleted)
                    {
                        PS->RegisterUnlockedObjective(Target->Objective);
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

    auto* Char = Cast<AUTCharacter>(PlayerPawn);
    if (Char != nullptr)
    {
        Char->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        auto* PC = Cast<ATrialsPlayerController>(Char->GetController());
        if (PC != nullptr && PC->CheckpointDest != nullptr)
        {
            PC->UseCheckpoint(Char);
        }

    }
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
    if (PS->ActiveObjective != nullptr)
    {
        auto* PC = Cast<ATrialsPlayerController>(Player);
        AActor* StartSpot = PC->CheckpointDest;
        if (StartSpot)
        {
            return StartSpot;
        }
        return PS->ActiveObjective->GetPlayerSpawn(Player);
    }

    // Otherwise use any playerstart but those with a tag.
    // Prefer HUB spawns(a PlayerStart with PlayerStartTag=HUB) if player has unlocked any objectives.
    const FString& NewIncomingName = IncomingName.IsEmpty() && PS->UnlockedObjectives.Num() > 0 ? TEXT("HUB") : IncomingName;
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

const uint32 RECORDFLAG_CHECKPOINT = 0x01;

void ATrialsGameMode::ScoreTrialObjective(ATrialsObjective* Obj, float Timer, AUTPlayerController* Player)
{
    auto* PlayerState = Cast<ATrialsPlayerState>(Player->PlayerState);
    PlayerState->AdjustScore(1);

    auto* PC = Cast<ATrialsPlayerController>(Player);
    bool bUsedCheckpoint = PC->CheckpointDest != nullptr;

    PC->ScoredObjective(Obj); // note: don't call after ScoreRecord!
    OnObjectiveCompleted.Broadcast(Obj, Player);

    FRecordInfo RecordInfo(Timer, PlayerState->PlayerNetId);
    if (bUsedCheckpoint)
    {
        RecordInfo.Flags |= RECORDFLAG_CHECKPOINT;
    }

    int32 RecordSwitch;
    float RecordTime = Obj->RecordTime;
    if (Timer < RecordTime || RecordTime == 0.00) // New all time
    {
        // New top record!
        RecordSwitch = 0;
        Obj->ScoreRecord(RecordInfo, Player);
    }
    else if (Timer == RecordTime) // Tied with all time
    {
        // tie or slower record...
        RecordSwitch = 1;
    }
    else // worse, check personal time
    {
        RecordTime = PlayerState->ObjectiveRecordTime;
        // New or first personal record
        if (RecordTime == 0.00)
        {
            RecordSwitch = 3;
            Obj->ScoreRecord(RecordInfo, Player);
        }
        else if (Timer < RecordTime)
        {
            RecordSwitch = 4;
            Obj->ScoreRecord(RecordInfo, Player);
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
    BroadcastLocalized(this, UTrialsObjectiveCompleteMessage::StaticClass(), RecordSwitch, PlayerState, nullptr, PlayerState->TimerState);

    bool bIsTopRecord = RecordSwitch == 0;
    OnObjectiveRecordSet.Broadcast(Obj, Player, Timer, RecordTime - Timer, bIsTopRecord);

    BroadcastLocalized(this, UTrialsRecordSetMessage::StaticClass(), RecordSwitch, PlayerState, nullptr, PlayerState->TimerState);
}