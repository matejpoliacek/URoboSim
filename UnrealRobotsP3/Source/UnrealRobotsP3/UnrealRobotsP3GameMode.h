// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameModeBase.h"
#include "EngineUtils.h"
#include "RRobot.h"
#include "UnrealRobotsP3GameMode.generated.h"

UCLASS(minimalapi)
class AUnrealRobotsP3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUnrealRobotsP3GameMode();

    void Tick(float DeltaSeconds) override;
};



