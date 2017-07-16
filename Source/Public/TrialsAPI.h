#pragma once

#include "GameFramework/Actor.h"
#include "Runtime/Online/HTTP/Public/Http.h"

#include "TrialsAPI.generated.h"

USTRUCT()
struct FClientInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString Name;

    FClientInfo() {}
};

USTRUCT()
struct FLoginInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString ProfileId;

    UPROPERTY()
    FString Name;
};

USTRUCT()
struct FPlayerInfo : public FLoginInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString CountryCode;
};

USTRUCT()
struct FObjectiveInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Title;

    FObjectiveInfo() {}
};

USTRUCT()
struct FPlayerObjectiveInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    TArray<FObjectiveInfo> Objs; // Only includes: id, name, and recordTime(of player).

};

USTRUCT()
struct FMapInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    TArray<FObjectiveInfo> Objs; // id and name only

    UPROPERTY()
    bool IsRanked;

    FMapInfo() : IsRanked(false) {}
};

USTRUCT()
struct FRecordInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    float Value;

    UPROPERTY()
    uint32 Flags;

    // id and name only
    UPROPERTY()
    FPlayerInfo Player;

    // id and name only
    UPROPERTY()
    FClientInfo Client;

    FRecordInfo() {}
    FRecordInfo(float Value, FString PlayerId): Flags(0)
    {
        this->Value = Value;
        this->Player._id = PlayerId;
    }
};

USTRUCT()
struct FObjInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Title;

    UPROPERTY()
    float RecordTime;

    UPROPERTY()
    TArray<FRecordInfo> Records;

    FObjInfo() : RecordTime(0.f) {}
};

typedef TSharedPtr<FJsonObject>& FAPIResult;
typedef FString FAPIError;

typedef TFunction<void(const FAPIResult Result)> FAPIOnResult;
typedef TFunction<void(const FAPIError Error)> FAPIOnError;

UCLASS()
class TRIALS_API ATrialsAPI : public AActor
{
    GENERATED_BODY()
    
public:	
    FString BaseURL;
    FString AuthToken;

    ATrialsAPI();

    void BeginPlay() override;
    void Tick(float DeltaTime) override;

    typedef TFunction<void()> FAuthenticate;
    void Authenticate(const FString& APIBaseURL, const FString& APIKey, const FString& ClientName, const FAuthenticate& OnSuccess);

    typedef TFunction<void(FPlayerInfo& PlayerInfo)> FLoginPlayer;
    void LoginPlayer(const FString& UniqueId, const FString& Name, const FLoginPlayer& OnSuccess = nullptr);

    typedef TFunction<void(FMapInfo& MapInfo)> FGetMap;
    void GetMap(const FString MapName, const FGetMap& OnSuccess);

    /*
    * Fetches data of an objective, including record time and a brief list of top records.
    * If the objective isn't registered yet, it shall be created.
    */
    void GetObj(const FString& MapName, const FString& ObjName, const TFunction<void(const FObjInfo& ObjInfo)> OnSuccess = nullptr)
    {
        checkSlow(!MapName.IsEmpty());
        checkSlow(!ObjName.IsEmpty());

        Fetch(TEXT("api/maps/") + FGenericPlatformHttp::UrlEncode(MapName) + TEXT("/") + FGenericPlatformHttp::UrlEncode(ObjName) + TEXT("?create=1&limit=3"),
            [OnSuccess](const FAPIResult Result) {
            FObjInfo ObjInfo;
            FromJSON(Result, &ObjInfo);

            if (OnSuccess)
                OnSuccess(ObjInfo);
        });
    }

    void SubmitRecord(const float Value, const FString& ObjId, const FString& PlayerId, const TFunction<void(const FRecordInfo& RecInfo)> OnSuccess = nullptr)
    {
        checkSlow(!ObjId.IsEmpty());
        checkSlow(!PlayerId.IsEmpty());
        
        FRecordInfo RecInfo(Value, PlayerId);
        Post(TEXT("api/recs/") + FGenericPlatformHttp::UrlEncode(ObjId), ToJSON(RecInfo), [OnSuccess](const FAPIResult Result) {
            FRecordInfo RecInfo;
            FromJSON(Result, &RecInfo);

            if (OnSuccess)
                OnSuccess(RecInfo);
        });
    }

    TSharedRef<IHttpRequest> Fetch(
        const FString Path, 
        const FAPIOnResult& OnSuccess,
        const FAPIOnError& OnError = [](FAPIError Error) -> void {
            UE_LOG(UT, Error, TEXT("An error occurred! %s"), *Error);
        }
    );

    TSharedRef<IHttpRequest> Post(
        const FString Path,
        const FString Content,
        const FAPIOnResult& OnSuccess,
        const FAPIOnError& OnError = [](FAPIError Error) -> void {
            UE_LOG(UT, Error, TEXT("An error occurred! %s"), *Error);
        }
    );

    template<typename OutStructType>
    static bool FromJSON(const TSharedPtr<FJsonObject>& Data, OutStructType* OutStruct)
    {
        return FJsonObjectConverter::JsonObjectToUStruct(Data.ToSharedRef(), OutStructType::StaticStruct(), OutStruct, 0, 0);
    }

    template<typename OutStructType>
    static FString ToJSON(const OutStructType& Params)
    {
        FString JsonOut;
        if (!FJsonObjectConverter::UStructToJsonObjectString(OutStructType::StaticStruct(), &Params, JsonOut, 0, 0))
        {
            UE_LOG(UT, Warning, TEXT("Failed to serialize JSON"));
            return FString();
        }
        return JsonOut;
    }

private:
    TSharedRef<IHttpRequest> CreateRequest(const FString& Verb, const FString& Path) const;
    void SendRequest(TSharedRef<IHttpRequest>& HttpRequest, const TFunction<bool(const FHttpResponsePtr& HttpResponse)>& OnComplete);
    void OnRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<bool(const FHttpResponsePtr& HttpResponse)> OnComplete) const;
};
