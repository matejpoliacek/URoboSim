// Fill out your copyright notice in the Description page of Project Settings.

#include "OdometryMover.h"
#include "GameFramework/Actor.h"
// #include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
UOdometryMover::UOdometryMover()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOdometryMover::BeginPlay()
{
	Super::BeginPlay();

	// Create handler for the actor component
	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("192.168.0.16"), 9001));
	UE_LOG(LogTemp, Log, TEXT("Handler for OdometryMover Created. "));

	OdomSubscriber = MakeShareable<FROSOdometrySubScriber>(new FROSOdometrySubScriber(TEXT("/odom")));
	Handler->AddSubscriber(OdomSubscriber);
	UE_LOG(LogTemp, Log, TEXT("Added odom subscriber in OdometryMover. "));

	Handler->Connect();
	UE_LOG(LogTemp, Log, TEXT("OdometryMover handler connected to WebSocket server. "));
	
	Owner = GetOwner();
	
	InitialLocation = Owner->GetActorLocation();
	init_pos_x = InitialLocation.X;
	init_pos_y = InitialLocation.Y;
	init_pos_z = InitialLocation.Z;

	InitialRotation = Owner->GetActorRotation();
	init_pitch = InitialRotation.Pitch;
	init_yaw = InitialRotation.Yaw;
	init_roll = InitialRotation.Roll;

}


// Called every frame
void UOdometryMover::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Handler->Render();

	FString FilePath = "C:\\Users\\Matej\\Documents\\Unreal Projects\\MScProject\\OdomMsgs.txt";
	FString msg;
	FFileHelper::LoadFileToString(msg, *FilePath);

	//UE_LOG(LogTemp, Log, TEXT("This is the extracted string: %s"), *msg);
	
	// convert msg to json 	
	JsonObject = MakeShareable(new FJsonObject());;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(msg);
	
	// extract data of interest
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{

		// top level -> pose
		PoseObject = JsonObject->GetObjectField(FString(TEXT("pose")));

		// pose -> pose (as opposed to covariance)
		PoseObject = PoseObject->GetObjectField(FString(TEXT("pose")));
		
		// pose -> position
		PositionObject = PoseObject->GetObjectField(FString(TEXT("position")));

		pos_x = PositionObject->GetNumberField(FString(TEXT("x")));
		pos_y = PositionObject->GetNumberField(FString(TEXT("y")));
		pos_z = PositionObject->GetNumberField(FString(TEXT("z")));

		OrientationObject = PoseObject->GetObjectField(FString(TEXT("orientation")));
		
		orient_x = OrientationObject->GetNumberField(FString(TEXT("x")));
		orient_y = OrientationObject->GetNumberField(FString(TEXT("y")));
		orient_z = OrientationObject->GetNumberField(FString(TEXT("z")));
		orient_w = OrientationObject->GetNumberField(FString(TEXT("w")));
	}

	Quaternion q;
	q.x = orient_x;
	q.y = orient_y;
	q.z = orient_z;
	q.w = orient_w;

	// adapt units
	pos_x = pos_x * toCM_MULTIPLIER;
	pos_y = pos_y * toCM_MULTIPLIER;
	pos_z = pos_z * toCM_MULTIPLIER;

	toEulerianAngle(q, roll, pitch, yaw);
	RadiansToDegrees(roll);
	RadiansToDegrees(pitch);
	RadiansToDegrees(yaw);

	// ROS provided yaw is opposite in UE
	yaw = -yaw;

	FVector CurrentLocation = Owner->GetActorLocation();

	// apply positional changes to the actor
	FVector EndLocation = FVector(init_pos_x + pos_x, init_pos_y + pos_y, init_pos_z + pos_z);
	FRotator EndRotation = FRotator(init_pitch + pitch, init_yaw + yaw, init_roll + roll);

	// setactorlocation ignores collisions
	// tried UKismetMathLibrary::VInterpTo, though it also uses setactorlocation so collisions are ignored
	// auto delta = GetWorld()->GetDeltaSeconds();
	//FVector DestinationLocation = UKismetMathLibrary::VInterpTo(CurrentLocation, EndLocation, delta, 150.0);
	//Owner->SetActorLocation(DestinationLocation);

	FHitResult RV_Hit(ForceInit);
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, Owner);
	FCollisionResponseParams RV_ResponseParams;
	
	bool bCollision = DoTrace(&RV_Hit, &RV_TraceParams, &RV_ResponseParams, CurrentLocation, EndLocation);

	//if bCollision get RV_hit.Location(), else use EndLocation for destination
	if (bCollision) {
		Owner->SetActorLocation(RV_Hit.Location);
		
		// every time the bot collides, set a new "initial position", so that it doesn't teleport around the place
		// e.g. if the bot rotates and then moves forward in a direction, where there's nothing in its way, 
		// it would just jump forward to the point sent in the message by ROS
			
		// if the bot is at the location where the collision occurs, make it move from this position, if new direction is unobstructed
		if (CurrentLocation == RV_Hit.Location) {
			UE_LOG(LogTemp, Log, TEXT("Bot at collision location."));

			//init_pos_x = RV_Hit.Location.X;
			//init_pos_y = RV_Hit.Location.Y;
			//init_pos_z = RV_Hit.Location.Z;
		}
	}
	else {
		Owner->SetActorLocation(EndLocation);
	}

	Owner->SetActorRotation(EndRotation);

	// log data for debugging
	FString DataLog = "Data applied: pos x:" + FString::SanitizeFloat(pos_x) + " y:" + FString::SanitizeFloat(pos_y) + " z:" + FString::SanitizeFloat(pos_z) +
		" orient x: " + FString::SanitizeFloat(orient_x) + " y : " + FString::SanitizeFloat(orient_y) + " z : " + FString::SanitizeFloat(orient_z) + " w : " + FString::SanitizeFloat(orient_w) +
		" pitch: " + FString::SanitizeFloat(pitch) + " yaw: " + FString::SanitizeFloat(yaw) + " roll: " + FString::SanitizeFloat(roll);
	UE_LOG(LogTemp, Log, TEXT("%s"), *DataLog);
}


void UOdometryMover::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Handler->Disconnect();
}


