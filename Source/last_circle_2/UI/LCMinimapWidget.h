#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "LCMinimapWidget.generated.h"

USTRUCT()
struct FEntityDot
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Entity = nullptr;

    UPROPERTY()
    FLinearColor Color = FLinearColor::Red;
};

UCLASS()
class LAST_CIRCLE_2_API ULCMinimapWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override
    {
        Super::NativeConstruct();
        CanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
        if (CanvasPanel) WidgetTree->RootWidget = CanvasPanel;

        // Background (biome map)
        BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
        if (BackgroundImage)
        {
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(BackgroundImage));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); CPSlot->SetAutoSize(false); }
        }

        // Player dot
        PlayerDot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerDot"));
        if (PlayerDot)
        {
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(PlayerDot));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.48f, 0.48f, 0.52f, 0.52f)); CPSlot->SetAutoSize(false); }
        }

        // Direction indicator
        DirectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Direction"));
        if (DirectionText)
        {
            DirectionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(DirectionText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.3f, 0.85f, 0.7f, 1.f)); CPSlot->SetAutoSize(true); }
        }

        // Zone circle info
        ZoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneInfo"));
        if (ZoneText)
        {
            ZoneText->SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 1.f, 1.f, 1.f)));
            UCanvasPanelSlot* CPSlot = Cast<UCanvasPanelSlot>(CanvasPanel->AddChildToCanvas(ZoneText));
            if (CPSlot) { CPSlot->SetAnchors(FAnchors(0.1f, 0.f, 0.9f, 0.1f)); CPSlot->SetAutoSize(true); }
        }
    }

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override
    {
        Super::NativeTick(MyGeometry, InDeltaTime);

        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC || !PC->GetPawn()) return;

        FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
        FRotator PlayerRot = PC->GetPawn()->GetActorRotation();

        // Update direction text
        if (DirectionText)
        {
            float Yaw = PlayerRot.Yaw;
            if (Yaw < 0) Yaw += 360.f;
            FString Dir;
            if (Yaw >= 337.5f || Yaw < 22.5f) Dir = TEXT("N");
            else if (Yaw < 67.5f) Dir = TEXT("NE");
            else if (Yaw < 112.5f) Dir = TEXT("E");
            else if (Yaw < 157.5f) Dir = TEXT("SE");
            else if (Yaw < 202.5f) Dir = TEXT("S");
            else if (Yaw < 247.5f) Dir = TEXT("SW");
            else if (Yaw < 292.5f) Dir = TEXT("W");
            else Dir = TEXT("NW");
            DirectionText->SetText(FText::FromString(FString::Printf(TEXT("%s %.0f°"), *Dir, Yaw)));
        }

        // Update minimap entity dots
        UpdateEntityDots(PlayerLoc);
    }

    UFUNCTION(BlueprintCallable)
    void AddEntityDot(AActor* Entity, FLinearColor Color)
    {
        if (Entity)
        {
            FEntityDot Dot;
            Dot.Entity = Entity;
            Dot.Color = Color;
            EntityDots.Add(Dot);
        }
    }

    UFUNCTION(BlueprintCallable)
    void ClearEntityDots() { EntityDots.Empty(); }

protected:
    UPROPERTY() UCanvasPanel* CanvasPanel = nullptr;
    UPROPERTY() UImage* BackgroundImage = nullptr;
    UPROPERTY() UImage* PlayerDot = nullptr;
    UPROPERTY() UTextBlock* DirectionText = nullptr;
    UPROPERTY() UTextBlock* ZoneText = nullptr;

    UPROPERTY() TArray<FEntityDot> EntityDots;

    void UpdateEntityDots(const FVector& PlayerLoc)
    {
        // Remove dead entities
        EntityDots.RemoveAll([](const FEntityDot& D) { return !D.Entity || !D.Entity->IsValidLowLevel(); });
    }
};
