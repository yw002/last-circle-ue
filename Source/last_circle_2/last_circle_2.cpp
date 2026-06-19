// Copyright Epic Games, Inc. All Rights Reserved.

#include "last_circle_2.h"
#include "Modules/ModuleManager.h"
#include "Characters/LCBaseCharacter.h"

void Flast_circle_2Module::StartupModule()
{
    FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
    ALCBaseCharacter::GetOrCreateVertexColorMaterial();
#endif
}

IMPLEMENT_PRIMARY_GAME_MODULE(Flast_circle_2Module, last_circle_2, "last_circle_2");
