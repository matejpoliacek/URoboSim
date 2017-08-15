#pragma once

#include "ROSBridgeSubscriber.h"
#include "nav_msgs/Odometry.h"
#include "Core.h"

class FROSOdometrySubScriber : public FROSBridgeSubscriber {

public:
	FROSOdometrySubScriber(FString Topic_) :
		FROSBridgeSubscriber(TEXT("nav_msgs/Odometry"), Topic_)
	{

	}

	~FROSOdometrySubScriber() override {};

	TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override
	{
		TSharedPtr<FROSBridgeMsgNavmsgsOdometry> Odometry =
			MakeShareable<FROSBridgeMsgNavmsgsOdometry>(new FROSBridgeMsgNavmsgsOdometry());
		Odometry->FromJson(JsonObject);
		UE_LOG(LogTemp, Log, TEXT("Odometry: %s"), *Odometry->ToString());

		return StaticCastSharedPtr<FROSBridgeMsg>(Odometry);
	}

	void CallBack(TSharedPtr<FROSBridgeMsg> msg) const override
	{
		TSharedPtr<FROSBridgeMsgNavmsgsOdometry> Odometry = StaticCastSharedPtr<FROSBridgeMsgNavmsgsOdometry>(msg);
		// do something
		UE_LOG(LogTemp, Log, TEXT("Message received! Content: %s"), *Odometry->ToString());

		return;
	}

};