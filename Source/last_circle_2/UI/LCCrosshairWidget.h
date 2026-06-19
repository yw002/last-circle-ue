#pragma once
#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "LCCrosshairWidget.generated.h"

UCLASS()
class LAST_CIRCLE_2_API ULCCrosshairWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrosshairCanvas"));
        if (CanvasPanel) WidgetTree->RootWidget = CanvasPanel;

        // Crosshair is drawn procedurally via lines
        CrosshairColor = FLinearColor::White;
    }

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override
    {
        Super::NativeTick(MyGeometry, InDeltaTime);
        if (HitTimer > 0.f)
        {
            HitTimer -= InDeltaTime;
            CrosshairColor = FLinearColor::Red;
        }
        else
        {
            CrosshairColor = FLinearColor::White;
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetSpread(float NewSpread) { CurrentSpread = NewSpread; }

    UFUNCTION(BlueprintCallable)
    void OnHitConfirmed() { HitTimer = 0.3f; }

    UFUNCTION(BlueprintCallable)
    void SetADS(bool bADS) { bIsADS = bADS; }

    UFUNCTION(BlueprintCallable)
    float GetCurrentSpread() const { return CurrentSpread; }

    // Crosshair geometry
    UFUNCTION(BlueprintCallable)
    void GetCrosshairGeometry(float& OutLength, float& OutGap, float& OutThickness) const
    {
        OutLength = bIsADS ? 8.f : 12.f;
        OutGap = 2.f + CurrentSpread * (bIsADS ? 0.5f : 1.f);
        OutThickness = bIsADS ? 2.f : 1.5f;
    }

protected:
    UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;
    UPROPERTY() FLinearColor CrosshairColor = FLinearColor::White;
    UPROPERTY() float CurrentSpread = 0.f;
    UPROPERTY() float HitTimer = 0.f;
    UPROPERTY() bool bIsADS = false;
};
