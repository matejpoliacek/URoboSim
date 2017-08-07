// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealRobotsP3.h"
#include "UnrealRobotsP3GameMode.h"
#include "UnrealRobotsP3HUD.h"
#include "UnrealRobotsP3Character.h"

AUnrealRobotsP3GameMode::AUnrealRobotsP3GameMode()
	: Super()
{
    PrimaryActorTick.bCanEverTick = true;

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AUnrealRobotsP3HUD::StaticClass();
}

void AUnrealRobotsP3GameMode::Tick(float DeltaSeconds)
{
//    for (TActorIterator<ARRobot> ActorItr(GetWorld()); ActorItr; ++ActorItr)
//    {
//        // Same as with the Object Iterator, access the subclass instance with the * or -> operators.
//        ARRobot *Mesh = *ActorItr;
//        UE_LOG(LogTemp, Log, TEXT("Actor Name: [%s], Actor Location: [%s]"),
//               *ActorItr->GetName(), *ActorItr->GetActorLocation().ToString());
//    }
}
