// Fill out your copyright notice in the Description page of Project Settings.

#include "OdometryMover.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"


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
	
	FVector InitialLocation = Owner->GetActorLocation();
	init_pos_x = InitialLocation.X;
	init_pos_y = InitialLocation.Y;
	init_pos_z = InitialLocation.Z;

	FRotator InitialRotation = Owner->GetActorRotation();
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
	pos_x = pos_x * CM_MULTIPLIER;
	pos_y = pos_y * CM_MULTIPLIER;
	pos_z = pos_z * CM_MULTIPLIER;

	toEulerianAngle(q, roll, pitch, yaw);
	RadiansToDegrees(roll);
	RadiansToDegrees(pitch);
	RadiansToDegrees(yaw);

	FVector CurrentLocation = Owner->GetActorLocation();
	auto delta = GetWorld()->GetDeltaSeconds();

	// apply positional changes to the actor
	FVector EndLocation = FVector(init_pos_x + pos_x, init_pos_y + pos_y, init_pos_z + pos_z);
	FRotator EndRotation = FRotator(init_pitch + pitch, init_yaw + yaw, init_roll + roll);

	// setactorlocation ignores collisions
	// tried UKismetMathLibrary::VInterpTo, though it also uses setactorlocation so collisions are ignored
	//FVector DestinationLocation = UKismetMathLibrary::VInterpTo(CurrentLocation, EndLocation, delta, 150.0);
	//Owner->SetActorLocation(DestinationLocation);

	Owner->SetActorLocation(EndLocation);
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