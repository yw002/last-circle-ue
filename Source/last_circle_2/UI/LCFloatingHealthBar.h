#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Characters/LCBaseCharacter.h"
#include "LCFloatingHealthBar.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ULCFloatingHealthBar : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FloatHPCanvas"));
        if (CanvasPanel) WidgetTree->RootWidget = CanvasPanel;

        HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HPBar"));
        if (HealthBar)
        {
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(HealthBar));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.1f, 0.3f, 0.9f, 0.7f)); CPSlot->SetAutoSize(false); }
        }

        NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
        if (NameText)
        {
            NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(NameText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.1f, 0.f, 0.9f, 0.3f)); CPSlot->SetAutoSize(true); }
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetTarget(AActor* NewTarget) { TargetActor = NewTarget; }

    UFUNCTION(BlueprintCallable)
    void UpdateHealthBar(float Percent)
    {
        if (HealthBar) HealthBar->SetPercent(Percent);
    }

    UFUNCTION(BlueprintCallable)
    void SetDisplayName(const FString& Name)
    {
        if (NameText) NameText->SetText(FText::FromString(Name));
    }

protected:
    UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;
    UPROPERTY() UProgressBar* HealthBar = nullptr;
    UPROPERTY() UTextBlock* NameText = nullptr;
    UPROPERTY() AActor* TargetActor = nullptr;
};
