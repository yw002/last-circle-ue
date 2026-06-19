#pragma once
#include "CoreMinimal.h"
#include "LCInteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class ULCInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class LAST_CIRCLE_2_API ILCInteractableInterface
{
    GENERATED_BODY()
public:
    virtual bool CanInteract(AActor* Interactor) { return true; }
    virtual void OnInteract(AActor* Interactor) {}
    virtual FString GetInteractionPrompt() { return TEXT("Press F to interact"); }
};
