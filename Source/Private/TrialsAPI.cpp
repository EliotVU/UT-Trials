#include "Trials.h"
#include "TrialsAPI.h"

ATrialsAPI::ATrialsAPI()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATrialsAPI::BeginPlay()
{
	Super::BeginPlay();
}

void ATrialsAPI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// API based on UTMcpUtils.h
TSharedRef<IHttpRequest> ATrialsAPI::CreateRequest(const FString& Verb, const FString& Path) const
{
    // Ensure we have passed authentication.
    check(!BaseURL.IsEmpty());

    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(BaseURL + Path);
    HttpRequest->SetVerb(Verb);
    return HttpRequest;
}

void ATrialsAPI::SendRequest(TSharedRef<IHttpRequest>& HttpRequest, const TFunction<bool (const FHttpResponsePtr& HttpResponse)>& OnComplete)
{
    UE_LOG(UT, Log, TEXT("Making a request %s"), *HttpRequest->GetURL());

    HttpRequest->OnProcessRequestComplete().BindUObject(this, &ATrialsAPI::OnRequestComplete, OnComplete);

    if (!AuthToken.IsEmpty())
    {
        HttpRequest->SetHeader("Auth-Token", AuthToken);
    }
    HttpRequest->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
    HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
    HttpRequest->ProcessRequest();
}

void ATrialsAPI::OnRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, TFunction<bool (const FHttpResponsePtr& HttpResponse)> OnComplete)
{
    if (HttpResponse.IsValid() && bSucceeded)
    {
        UE_LOG(UT, Log, TEXT("Received response from request %s with %s"), *HttpRequest->GetURL(), *HttpResponse->GetContentAsString());
        if (OnComplete(HttpResponse))
        {
        }
        else
        {
            UE_LOG(UT, Error, TEXT("Request response, denied!."));
        }
    }
    else
    {
        UE_LOG(UT, Warning, TEXT("Request failed. No response from %s"), *HttpRequest->GetURL());
        OnComplete(FHttpResponsePtr());
    }
}

TSharedRef<IHttpRequest> ATrialsAPI::Fetch(const FString Path, const FAPIOnResult& OnSuccess, const FAPIOnError& OnError)
{
    auto HttpRequest = CreateRequest(TEXT("GET"), Path);
    SendRequest(HttpRequest, [this, Path, OnSuccess, OnError](const FHttpResponsePtr& HttpResponse) -> bool {
        if (!HttpResponse.IsValid())
        {
            OnError(TEXT("No response! Invalid."));
            return false;
        }

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            OnError(TEXT("Couldn't deserialize response from ") + Path);
            return false;
        }

        OnSuccess(JsonObject);
        return true;
    });
    return HttpRequest;
}

TSharedRef<IHttpRequest> ATrialsAPI::Post(const FString Path, const FString Content, const FAPIOnResult& OnSuccess, const FAPIOnError& OnError)
{
    auto HttpRequest = CreateRequest(TEXT("POST"), Path);
    HttpRequest->SetContentAsString(Content);
    UE_LOG(UT, Log, TEXT("Content: %s"), *Content);
    SendRequest(HttpRequest, [this, OnSuccess, OnError](const FHttpResponsePtr& HttpResponse) -> bool {
        if (!HttpResponse.IsValid())
        {
            OnError(TEXT("No response! Invalid."));
            return false;
        }

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            OnError(TEXT("Couldn't deserialize response!"));
            return false;
        }

        OnSuccess(JsonObject);
        return true;
    });
    return HttpRequest;
}

void ATrialsAPI::Authenticate(const FString& APIBaseURL, const FString& APIToken, const FString& ClientName, const FAuthenticate& OnResponse)
{
    UE_LOG(UT, Log, TEXT("Making an authentication request URL: %s token: %s"), *APIBaseURL, *APIToken);

    BaseURL = APIBaseURL;
    Fetch(TEXT("api/authenticate?token=") 
        + FGenericPlatformHttp::UrlEncode(APIToken) 
        + TEXT("&name=") + FGenericPlatformHttp::UrlEncode(ClientName),
        [this, OnResponse](const FAPIResult& Data) {
            AuthToken = Data->GetStringField("token");
            OnResponse();
        }
    );
}

void ATrialsAPI::GetMap(const FString MapName, const FGetMap& OnResponse)
{
    Fetch(TEXT("api/maps/") + FGenericPlatformHttp::UrlEncode(MapName) + TEXT("?create=1"), [this, OnResponse](const FAPIResult& Data) {
        FMapInfo MapInfo;
        FromJSON(Data, &MapInfo);
        OnResponse(MapInfo);
    });
}