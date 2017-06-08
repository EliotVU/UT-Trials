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
    FString Name;

    UPROPERTY()
    TArray<FObjectiveInfo> Objs;

    UPROPERTY()
    bool IsRanked;

    FMapInfo() : IsRanked(false) {}
};

UCLASS()
class TRIALS_API ATrialsAPI : public AActor
{
	GENERATED_BODY()

public:
    typedef TFunction<void()> FAuthenticate;
    void Authenticate(const FString& APIBaseURL, const FString& APIToken, const FAuthenticate& OnResponse);

public:
    typedef TFunction<void(FMapInfo& MapInfo)> FGetMap;
    void GetMap(const FString MapName, const FGetMap& OnResponse);
	
public:	
    FString BaseURL;
    FString AuthToken;

    ATrialsAPI();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
    TSharedRef<IHttpRequest> CreateRequest(const FString& Verb, const FString& Path) const;
    void SendRequest(TSharedRef<IHttpRequest>& HttpRequest, const TFunction<bool(const FHttpResponsePtr& HttpResponse)>& OnComplete);
    void OnRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<bool(const FHttpResponsePtr& HttpResponse)> OnComplete);
    void RequestError(FString Error);
};
