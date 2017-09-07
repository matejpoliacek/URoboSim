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

	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(IPADDRESS, PORT));
	UE_LOG(LogTemp, Log, TEXT("Handler for OdometryMover Created. "));

	OdomSubscriber = MakeShareable<FROSOdometrySubScriber>(new FROSOdometrySubScriber(TEXT("/odom")));
	Handler->AddSubscriber(OdomSubscriber);
	UE_LOG(LogTemp, Log, TEXT("Added odom subscriber in OdometryMover. "));

	Handler->Connect();
	UE_LOG(LogTemp, Log, TEXT("OdometryMover handler connected to WebSocket server. "));
	
	Owner = GetOwner();
	
	RV_TraceParams.AddIgnoredActor(Owner);

	do {		
		Handler->Render();

		// extract initial location
		OdometryMessage = OdomSubscriber->GetMessage();

		JsonObject = OdometryMessage->ToJsonObject();
		ExtractPositionAndRotation(JsonObject, ExtractedLocation, QuaternionData);

		ConvertLocationUnits(ExtractedLocation);
		OldMsgLocation = ExtractedLocation;

		toEulerianAngle(QuaternionData, roll, pitch, yaw);
		RadiansToDegrees(pitch);
		RadiansToDegrees(yaw);
		RadiansToDegrees(roll);

		OldMsgRotation = FRotator(pitch, yaw, roll);
		ConvertRotationUnits(OldMsgRotation);
	} while (isinf(OldMsgLocation.X)); // until a message is received

	FString old = FString::SanitizeFloat(OldMsgLocation.X) + " " + FString::SanitizeFloat(OldMsgLocation.Y) + " " + FString::SanitizeFloat(OldMsgLocation.Z);
	UE_LOG(LogTemp, Log, TEXT("Old location at BeginPlay: %s"), *old);

	FString oldrot = FString::SanitizeFloat(pitch) + " " + FString::SanitizeFloat(yaw) + " " + FString::SanitizeFloat(roll);
	UE_LOG(LogTemp, Log, TEXT("Old rotation pre-conversion at BeginPlay: %s"), *oldrot);

	FString oldrotconv = FString::SanitizeFloat(OldMsgRotation.Pitch) + " " + FString::SanitizeFloat(OldMsgRotation.Yaw) + " " + FString::SanitizeFloat(OldMsgRotation.Roll);
	UE_LOG(LogTemp, Log, TEXT("Old rotation post-conversion at BeginPlay: %s"), *oldrotconv);
}


// Called every frame
void UOdometryMover::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Handler->Render();

	OdometryMessage = OdomSubscriber->GetMessage();

	JsonObject = OdometryMessage->ToJsonObject();

	ExtractPositionAndRotation(JsonObject, ExtractedLocation, QuaternionData);

	// adapt units and store
	// ROS provided y value and yaw is opposite in UE, hence we need to set them negative

	ConvertLocationUnits(ExtractedLocation);
	NewMsgLocation = ExtractedLocation;

	FString extract = FString::SanitizeFloat(NewMsgLocation.X) + " " + FString::SanitizeFloat(NewMsgLocation.Y) + " " + FString::SanitizeFloat(NewMsgLocation.Z);
	UE_LOG(LogTemp, Log, TEXT("This is the extracted location: %s"), *extract);

	toEulerianAngle(QuaternionData, roll, pitch, yaw);
	RadiansToDegrees(pitch);
	RadiansToDegrees(yaw);
	RadiansToDegrees(roll);
	
	NewMsgRotation = FRotator(pitch, yaw, roll);
	ConvertRotationUnits(NewMsgRotation);
	
	// obtain differences from previous state
	LocationMsgDifference = CalculatePositionDifference(OldMsgLocation, NewMsgLocation);
	RotationMsgDifference = CalculateOrientationDifference(OldMsgRotation, NewMsgRotation);

	FHitResult RV_CurrentVerticalHit(ForceInit);
	FHitResult RV_TargetVerticalHit(ForceInit);

	CurrentLocation = Owner->GetActorLocation();
	CurrentRotation = Owner->GetActorRotation();

	FString old = FString::SanitizeFloat(OldMsgLocation.X) + " " + FString::SanitizeFloat(OldMsgLocation.Y) + " " + FString::SanitizeFloat(OldMsgLocation.Z);
	UE_LOG(LogTemp, Log, TEXT("Old location: %s"), *old);

	FString current = FString::SanitizeFloat(CurrentLocation.X) + " " + FString::SanitizeFloat(CurrentLocation.Y) + " " + FString::SanitizeFloat(CurrentLocation.Z);
	UE_LOG(LogTemp, Log, TEXT("Current location: %s"), *current);

	FString diff = FString::SanitizeFloat(LocationMsgDifference.X) + " " + FString::SanitizeFloat(LocationMsgDifference.Y) + " " + FString::SanitizeFloat(LocationMsgDifference.Z);
	UE_LOG(LogTemp, Log, TEXT("Difference in location: %s"), *diff);

	// apply positional changes to the actor
	TargetLocation = CurrentLocation + LocationMsgDifference;
	TargetRotation = CurrentRotation + RotationMsgDifference;

	FString target = FString::SanitizeFloat(TargetLocation.X) + " " + FString::SanitizeFloat(TargetLocation.Y) + " " + FString::SanitizeFloat(TargetLocation.Z);
	UE_LOG(LogTemp, Log, TEXT("Target location: %s"), *target);

	// create points high above terrain for current and end locations
	CurrentLocationHigh = CurrentLocation + FVector(0, 0, 100000);
	TargetLocationHigh = TargetLocation + FVector(0, 0, 100000);

	// similarly, create points below the terrain for current and end locations
	CurrentLocationLow = CurrentLocation + FVector(0, 0, -100000);
	TargetLocationLow = TargetLocation + FVector(0, 0, -100000);

	// a vertical trace is run between both points (top-down) for new and old position
	// to identify the change in the height of terrain, so that bot's Z position can be adjusted

	bool bCurrentVerticalCollision = DoTrace(&RV_CurrentVerticalHit, &RV_TraceParams, &RV_ResponseParams, CurrentLocationHigh, CurrentLocationLow);
	bool bTargetVerticalCollision = DoTrace(&RV_TargetVerticalHit, &RV_TraceParams, &RV_ResponseParams, TargetLocationHigh, TargetLocationLow);
		
	// find the altitude difference between current location and target location
	double Z_diff = RV_TargetVerticalHit.Location.Z - RV_CurrentVerticalHit.Location.Z;
	
	FString currentvert = FString::SanitizeFloat(RV_CurrentVerticalHit.Location.X) + " " + FString::SanitizeFloat(RV_CurrentVerticalHit.Location.Y) + " " + FString::SanitizeFloat(RV_CurrentVerticalHit.Location.Z);
	UE_LOG(LogTemp, Log, TEXT("Current vertical hit location: %s"), *currentvert);

	FString targetvert = FString::SanitizeFloat(RV_TargetVerticalHit.Location.X) + " " + FString::SanitizeFloat(RV_TargetVerticalHit.Location.Y) + " " + FString::SanitizeFloat(RV_TargetVerticalHit.Location.Z);
	UE_LOG(LogTemp, Log, TEXT("Target vertical hit location: %s"), *targetvert);

	UE_LOG(LogTemp, Log, TEXT("Z difference: %s"), *FString::SanitizeFloat(Z_diff));

	// check for collision in the horizontal plane

	FHitResult RV_Hit(ForceInit);
	
	bool bCollision = DoTrace(&RV_Hit, &RV_TraceParams, &RV_ResponseParams, CurrentLocation, TargetLocation);

	
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
			bool bAtCollision = ((RV_Hit.Location.Size() - CurrentLocation.Size()) > (TargetLocation.Size() - CurrentLocation.Size()));

			// if the next location if before the collision spot, move there
			if (!bAtCollision) {
				TargetLocation = TargetLocation + FVector(0, 0, Z_diff);
				Owner->SetActorLocation(TargetLocation);
				UE_LOG(LogTemp, Log, TEXT("Bot headed towards collision location."));
			}
			// if the next location is further, it is beyond the collision, thus go only up to the collision point
			else {
				Owner->SetActorLocation(RV_Hit.Location);
				UE_LOG(LogTemp, Log, TEXT("Target location beyond collision location."));
			}
		}		
	}
	// otherwise, if no collision takes place, move to the destination location, at an appropriate altitude
	else {
		
		//add the Z axis difference
		TargetLocation = TargetLocation + FVector(0, 0, Z_diff);
		Owner->SetActorLocation(TargetLocation);
	}

	Owner->SetActorRotation(TargetRotation);

	// set the data of current tick as the old data for the next tick
	// the Z difference is omitted here, as any Z-coordinate difference coming from ROS will be calculated when comparing old and new messages
	OldMsgLocation = NewMsgLocation;
	OldMsgRotation = NewMsgRotation;


	FString endTick = FString::SanitizeFloat(Owner->GetActorLocation().X) + " " + FString::SanitizeFloat(Owner->GetActorLocation().Y) + " " + FString::SanitizeFloat(Owner->GetActorLocation().Z);
	UE_LOG(LogTemp, Log, TEXT("Bot position at the end of tick: %s"), *endTick);
}


void UOdometryMover::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Handler->Disconnect();
}


