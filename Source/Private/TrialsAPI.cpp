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
    UE_LOG(UT, Log, TEXT("Received response from request %s with %s"), *HttpRequest->GetURL(), *HttpResponse->GetContentAsString());
    if (HttpResponse.IsValid() && bSucceeded)
    {
        if (OnComplete(HttpResponse))
        {
        }
    }
    else
    {
        OnComplete(FHttpResponsePtr());
    }
}

void ATrialsAPI::Authenticate(const FString& APIBaseURL, const FString& APIToken, const FAuthenticate& OnResponse)
{
    UE_LOG(UT, Log, TEXT("Making an authentication request URL: %s token: %s"), *APIBaseURL, *APIToken);

    BaseURL = APIBaseURL;

    auto HttpRequest = CreateRequest(TEXT("GET"), "api/authenticate?token=" + FGenericPlatformHttp::UrlEncode(APIToken));
    SendRequest(HttpRequest, [this, OnResponse](const FHttpResponsePtr& HttpResponse) -> bool {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            // ... can't authenticate!
            RequestError(TEXT("Can't authenticate!"));
            return false;
        }

        AuthToken = JsonObject->GetStringField("token");
        OnResponse();
        return true;
    });
}

void ATrialsAPI::GetMap(const FString MapName, const FGetMap& OnResponse)
{
    auto HttpRequest = CreateRequest(TEXT("GET"), "api/maps/" + FGenericPlatformHttp::UrlEncode(MapName) + "?create=1");
    SendRequest(HttpRequest, [this, OnResponse](const FHttpResponsePtr& HttpResponse) -> bool {
        FMapInfo MapInfo;

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            RequestError(TEXT("Received an invalid response!"));
            return false;
        }

        FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &MapInfo, 0, 0);
        OnResponse(MapInfo);
        return true;
    });
}

void ATrialsAPI::RequestError(FString Error)
{
}

