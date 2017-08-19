// Fill out your copyright notice in the Description page of Project Settings.

#include "OdometryMover.h"
#include "GameFramework/Actor.h"


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
	
	OldLocation = Owner->GetActorLocation();
	OldRotation = Owner->GetActorRotation();
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

	// adapt units and store
	// ROS provided y value and yaw is opposite in UE, hence we need to set them negative
	NewLocation = FVector(pos_x * toCM_MULTIPLIER, -pos_y * toCM_MULTIPLIER, pos_z * toCM_MULTIPLIER);

	toEulerianAngle(q, roll, pitch, yaw);
	RadiansToDegrees(pitch);
	RadiansToDegrees(yaw);
	RadiansToDegrees(roll);

	yaw = -yaw;

	NewRotation = FRotator(pitch, yaw, roll);
	
	// obtain differences from previous state
	CalculatePositionDifference(OldLocation, NewLocation, LocationDifference);
	CalculateOrientationDifference(OldRotation, NewRotation, RotationDifference);


	FVector CurrentLocation = Owner->GetActorLocation();
	FRotator CurrentRotation = Owner->GetActorRotation();

	// apply positional changes to the actor
	FVector EndLocation = CurrentLocation + LocationDifference;
	FRotator EndRotation = CurrentRotation + RotationDifference;


	FHitResult RV_Hit(ForceInit);
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, Owner);
	FCollisionResponseParams RV_ResponseParams;
	
	bool bCollision = DoTrace(&RV_Hit, &RV_TraceParams, &RV_ResponseParams, CurrentLocation, EndLocation);

	//if bCollision (i.e. robot has collided)... 
	if (bCollision) {

		//... and the robot is at the location of the collision
		if (CurrentLocation == RV_Hit.Location) {
			// don't move, only log collision (new rotation will be applied)
			UE_LOG(LogTemp, Log, TEXT("Bot at collision location."));
		}

		// ... and the robot is yet to reach the location of the collision
		else {
			// check if the new location is before the collision location
			bool bAtCollision = ((RV_Hit.Location.Size() - CurrentLocation.Size()) > (NewLocation.Size() - CurrentLocation.Size()));
			
			// if the next location if before the collision spot, move there
			if (!bAtCollision) {
				Owner->SetActorLocation(EndLocation);
			}
			// if the next location is further, it is beyond the collision, thus go only up to the collision point
			else {
				Owner->SetActorLocation(RV_Hit.Location);
			}
		}		
	}
	// otherwise, if no collision takes place, move to the destination location
	else {
		Owner->SetActorLocation(EndLocation);
	}

	Owner->SetActorRotation(EndRotation);

	// set the data of current tick as the old data for the next tick
	OldLocation = NewLocation;
	OldRotation = NewRotation;


	// log data for debugging
	// TODO update
	//FString DataLog = "Data applied: pos x:" + FString::SanitizeFloat(pos_x) + " y:" + FString::SanitizeFloat(pos_y) + " z:" + FString::SanitizeFloat(pos_z) +
	//	" orient x: " + FString::SanitizeFloat(orient_x) + " y : " + FString::SanitizeFloat(orient_y) + " z : " + FString::SanitizeFloat(orient_z) + " w : " + FString::SanitizeFloat(orient_w) +
	//	" pitch: " + FString::SanitizeFloat(pitch) + " yaw: " + FString::SanitizeFloat(yaw) + " roll: " + FString::SanitizeFloat(roll);
	//UE_LOG(LogTemp, Log, TEXT("%s"), *DataLog);
}


void UOdometryMover::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Handler->Disconnect();
}


