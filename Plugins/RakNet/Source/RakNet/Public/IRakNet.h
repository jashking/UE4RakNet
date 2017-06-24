// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRakNet, Log, All)

class IRakNet : public IModuleInterface
{
public:
	static inline IRakNet& Get()
	{
		return FModuleManager::LoadModuleChecked<IRakNet>("RakNet");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RakNet");
	}
};