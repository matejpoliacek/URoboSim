#pragma once

#include "ROSBridgeSubscriber.h"
#include "nav_msgs/Odometry.h"
#include "Core.h"

static TSharedPtr<FROSBridgeMsgNavmsgsOdometry> Message = MakeShareable<FROSBridgeMsgNavmsgsOdometry>(new FROSBridgeMsgNavmsgsOdometry());

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
		Message = Odometry;
		UE_LOG(LogTemp, Log, TEXT("Message received! Content: %s"), *Odometry->ToString());
		//OutputMsgToFile(Odometry->ToJsonObject());

		return;
	}

	TSharedPtr<FROSBridgeMsgNavmsgsOdometry> GetMessage() {
		return Message;
	}

//private:
//
//	void OutputMsgToFile(TSharedPtr<FJsonObject> MsgJson)  const
//	{
//		FString FilePath = "C:\\Users\\Matej\\Documents\\Unreal Projects\\MScProject\\OdomMsgs.txt";
//		FString MsgString;
//		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&MsgString);
//
//		if (FJsonSerializer::Serialize(MsgJson.ToSharedRef(), Writer)) {
//			FFileHelper::SaveStringToFile(MsgString, *FilePath);
//		}
//	}
};