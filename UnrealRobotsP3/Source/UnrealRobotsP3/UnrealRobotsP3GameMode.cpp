// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealRobotsP3GameMode.h"
#include "UnrealRobotsP3.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"
#include "rosgraph_msgs/Clock.h"

AUnrealRobotsP3GameMode::AUnrealRobotsP3GameMode()
	: Super()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUnrealRobotsP3GameMode::BeginPlay()
{
    Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("127.0.0.1"), 9001));
    TimePublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("rosgraph_msgs/Clock"), TEXT("clock")));
    Handler->AddPublisher(TimePublisher);
    Handler->Connect();
    UE_LOG(LogTemp, Log, TEXT("[AUnrealRobotsP3GameMode::BeginPlay()] Websocket server connected."));
}

void AUnrealRobotsP3GameMode::Tick(float DeltaSeconds)
{
    float GameTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    uint64 GameSeconds = (int)GameTime;
    uint64 GameUseconds = (GameTime - GameSeconds) * 1000000000;
    TSharedPtr<FROSBridgeMsgRosgraphmsgsClock> Clock = MakeShareable
            (new FROSBridgeMsgRosgraphmsgsClock(FROSTime(GameSeconds, GameUseconds)));
    Handler->PublishMsg("clock", Clock);

//    for (TActorIterator<ARRobot> ActorItr(GetWorld()); ActorItr; ++ActorItr)
//    {
//        // Same as with the Object Iterator, access the subclass instance with the * or -> operators.
//        ARRobot *Mesh = *ActorItr;
//        UE_LOG(LogTemp, Log, TEXT("Actor Name: [%s], Actor Location: [%s]"),
//               *ActorItr->GetName(), *ActorItr->GetActorLocation().ToString());
//    }
}

void AUnrealRobotsP3GameMode::Logout(AController *Exiting)
{
    Handler->Disconnect();
    AGameModeBase::Logout(Exiting);
}
