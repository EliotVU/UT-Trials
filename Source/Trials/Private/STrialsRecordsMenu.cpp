#include "Trials.h"
#include "STrialsRecordsMenu.h"

#if !UE_SERVER

void STrialsRecordsMenu::Construct(const FArguments& InArgs)
{
    PlayerOwner = InArgs._PlayerOwner;
    checkSlow(PlayerOwner);

    FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);

    SAssignNew(RecordsWebBrowser, SWebBrowser)
        .ShowControls(false)
        .ShowAddressBar(false)
        .InitialURL(TEXT("http://localhost:8080/maps/") + InArgs._MapName + TEXT("/objs/") + InArgs._ObjName)
        .OnLoadCompleted(FSimpleDelegate::CreateSP(this, &STrialsRecordsMenu::OnLoadRecordsCompleted))
        .OnLoadError(FSimpleDelegate::CreateSP(this, &STrialsRecordsMenu::OnLoadRecordsError));

    ChildSlot
        .HAlign(HAlign_Fill).VAlign(VAlign_Fill)
        .Padding(10.0f, 20.0f, 10.0f, 20.0f)
        [
            SNew(SBox).HeightOverride(890)
            [
                RecordsWebBrowser.ToSharedRef()
            ]
        ];

    AUTGameState* UTGameState = (PlayerOwner.IsValid() && PlayerOwner->GetWorld() != nullptr) ? PlayerOwner->GetWorld()->GetGameState<AUTGameState>() : nullptr;
    if (UTGameState != nullptr) UTGameState->bLocalMenusAreActive = true;
    FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::Keyboard);
}

void STrialsRecordsMenu::OnLoadRecordsCompleted()
{
}

void STrialsRecordsMenu::OnLoadRecordsError()
{
}

#endif
