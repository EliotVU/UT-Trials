#pragma once

#if !UE_SERVER

#include "SWebBrowser.h"

class TRIALS_API STrialsRecordsMenu : public SCompoundWidget
{
    SLATE_BEGIN_ARGS(STrialsRecordsMenu) 
    {}

    SLATE_ARGUMENT(UUTLocalPlayer*, PlayerOwner)
    SLATE_ARGUMENT(FString, MapName)
    SLATE_ARGUMENT(FString, ObjName)
    SLATE_END_ARGS()

public:
    void Construct(const FArguments& InArgs);

protected:
    TWeakObjectPtr<class UUTLocalPlayer> PlayerOwner;

    TSharedPtr<class SWebBrowser> RecordsWebBrowser;

    virtual void OnLoadRecordsCompleted();
    virtual void OnLoadRecordsError();
};

#endif