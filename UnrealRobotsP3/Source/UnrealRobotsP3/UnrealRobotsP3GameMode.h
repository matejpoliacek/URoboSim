// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameModeBase.h"
#include "EngineUtils.h"
#include "RRobot.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"
#include "UnrealRobotsP3GameMode.generated.h"

UCLASS(minimalapi)
class AUnrealRobotsP3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
    TSharedPtr<FROSBridgeHandler> Handler;
    TSharedPtr<FROSBridgePublisher> TimePublisher;

    UPROPERTY()
    FString ROSBridgeServerIPAddr;

    UPROPERTY()
    uint32 ROSBridgeServerPort;

	AUnrealRobotsP3GameMode();

    void BeginPlay() override;

    void Tick(float DeltaSeconds) override;

    void Logout(AController *Exiting) override;
};



