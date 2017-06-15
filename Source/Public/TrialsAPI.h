#pragma once

#include "GameFramework/Actor.h"
#include "Runtime/Online/HTTP/Public/Http.h"

#include "TrialsAPI.generated.h"

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
struct FMapInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY()
    FString _id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    TArray<FString> Objs; // Ids

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

    UPROPERTY()
    FString PlayerId;

    UPROPERTY()
    FString ClientId;
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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

    typedef TFunction<void()> FAuthenticate;
    void Authenticate(const FString& APIBaseURL, const FString& APIToken, const FString& ClientName, const FAuthenticate& OnResponse);

    typedef TFunction<void(FMapInfo& MapInfo)> FGetMap;
    void GetMap(const FString MapName, const FGetMap& OnResponse);

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
    void OnRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<bool(const FHttpResponsePtr& HttpResponse)> OnComplete);
};
