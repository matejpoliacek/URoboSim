// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ROSBridgeHandler.h"
#include "ROSOdometrySubscriber.h"



#include "OdometryMover.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROSBRIDGETEST_API UOdometryMover : public UActorComponent
{
	GENERATED_BODY()

public:	

	struct Quaternion
	{
		double x = 0.0, y = 0.0, z = 0.0, w = 0.0;
	};

	double toCM_MULTIPLIER = 100.0;

	AActor* Owner;
	
	TSharedPtr<FROSBridgeHandler> Handler;
	TSharedPtr<FROSOdometrySubScriber> OdomSubscriber;

	TSharedPtr<FJsonObject> JsonObject, PoseObject, PositionObject, OrientationObject;

	FVector NewLocation;
	FVector OldLocation;
	FVector LocationDifference;

	FRotator NewRotation;
	FRotator OldRotation;
	FRotator RotationDifference;

	double pos_x = 0.0, pos_y = 0.0, pos_z = 0.0;
	double orient_x = 0.0, orient_y = 0.0, orient_z = 0.0, orient_w = 0.0, yaw = 0.0, pitch = 0.0, roll = 0.0;
	
	// Sets default values for this component's properties
	UOdometryMover();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// helper to calculate eulerian angles
	static void toEulerianAngle(const Quaternion& q, double& roll, double& pitch, double& yaw) 
	{
		double ysqr = q.y * q.y;
	
		// roll (x-axis rotation)
		double t0 = +2.0 * (q.w * q.x + q.y * q.z);
		double t1 = +1.0 - 2.0 * (q.x * q.x + ysqr);
		roll = atan2(t0, t1);

		// pitch (y-axis rotation)
		double t2 = +2.0 * (q.w * q.y - q.z * q.x);
		t2 = ((t2 > 1.0) ? 1.0 : t2);
		t2 = ((t2 < -1.0) ? -1.0 : t2);
		pitch = asin(t2);

		// yaw (z-axis rotation)
		double t3 = +2.0 * (q.w * q.z + q.x * q.y);
		double t4 = +1.0 - 2.0 * (ysqr + q.z * q.z);
		yaw = atan2(t3, t4);
	};

	static void RadiansToDegrees(double& angle)
	{
		angle = angle * 180 / M_PI;
	};

	bool DoTrace(FHitResult* RV_Hit, FCollisionQueryParams* RV_TraceParams, FCollisionResponseParams* RV_ResponseParams, FVector StartLoc, FVector EndLoc)
	{
		RV_TraceParams->bTraceComplex = true;
		RV_TraceParams->bTraceAsyncScene = true;
		RV_TraceParams->bReturnPhysicalMaterial = true;

		//  do the line trace
		bool DidTrace = GetWorld()->LineTraceSingleByChannel(
			*RV_Hit,        //result
			StartLoc,        //start
			EndLoc,        //end
			ECC_Pawn,    //collision channel
			*RV_TraceParams,
			*RV_ResponseParams
			);

		return DidTrace;
	};

	// calculates change in position 
	void CalculatePositionDifference(FVector PreviousLocation, FVector NewLocation, FVector& PositionDifference) {
		PositionDifference = NewLocation - PreviousLocation;
		return;
	};

	// calculates change in orientation
	void CalculateOrientationDifference(FRotator PreviousOrientation, FRotator NewOrientation, FRotator& OrientationDifference) {
		OrientationDifference = NewOrientation - PreviousOrientation;
		return;
	};

};