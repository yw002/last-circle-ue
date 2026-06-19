#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "LCWeaponSlotWidget.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ULCWeaponSlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WeaponSlotCanvas"));
        if (CanvasPanel) WidgetTree->RootWidget = CanvasPanel;

        // Slot 1
        Slot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Slot1"));
        if (Slot1Text)
        {
            Slot1Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(Slot1Text));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.02f, 0.75f, 0.15f, 0.82f)); CPSlot->SetAutoSize(true); }
        }

        // Slot 2
        Slot2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Slot2"));
        if (Slot2Text)
        {
            Slot2Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(Slot2Text));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.02f, 0.82f, 0.15f, 0.89f)); CPSlot->SetAutoSize(true); }
        }

        // Active slot indicator
        ActiveIndicator = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActiveInd"));
        if (ActiveIndicator)
        {
            ActiveIndicator->SetText(FText::FromString(TEXT("►")));
            ActiveIndicator->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(ActiveIndicator));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.0f, 0.75f, 0.02f, 0.82f)); CPSlot->SetAutoSize(true); }
        }

        // Grenade count
        GrenadeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Grenade"));
        if (GrenadeText)
        {
            GrenadeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.5f, 0.f, 1.f)));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(GrenadeText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.02f, 0.68f, 0.12f, 0.75f)); CPSlot->SetAutoSize(true); }
        }
    }

    UFUNCTION(BlueprintCallable)
    void UpdateSlots(const FString& Slot1Name, int32 Slot1Ammo, int32 Slot1Max,
                    const FString& Slot2Name, int32 Slot2Ammo, int32 Slot2Max,
                    int32 ActiveSlot, int32 GrenadeCount)
    {
        if (Slot1Text)
            Slot1Text->SetText(FText::FromString(FString::Printf(TEXT("[%s] %d/%d"), *Slot1Name, Slot1Ammo, Slot1Max)));
        if (Slot2Text)
            Slot2Text->SetText(FText::FromString(FString::Printf(TEXT("[%s] %d/%d"), *Slot2Name, Slot2Ammo, Slot2Max)));
        if (ActiveIndicator)
        {
            float YPos = (ActiveSlot == 0) ? 0.75f : 0.82f;
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(ActiveIndicator->Slot);
            if (CPSlot) CPSlot->SetAnchors(FAnchors(0.0f, YPos, 0.02f, YPos + 0.07f));
        }
        if (GrenadeText)
            GrenadeText->SetText(FText::FromString(FString::Printf(TEXT("Grenades: %d"), GrenadeCount)));
    }

protected:
    UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;
    UPROPERTY() UTextBlock* Slot1Text = nullptr;
    UPROPERTY() UTextBlock* Slot2Text = nullptr;
    UPROPERTY() UTextBlock* ActiveIndicator = nullptr;
    UPROPERTY() UTextBlock* GrenadeText = nullptr;
};
