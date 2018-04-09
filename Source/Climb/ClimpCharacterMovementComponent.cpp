// Fill out your copyright notice in the Description page of Project Settings.

#include "ClimpCharacterMovementComponent.h"
#include "ClimbCharacter.h"

#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"


#define ECC_Breach_Climbable	ECC_GameTraceChannel1

UClimpCharacterMovementComponent::UClimpCharacterMovementComponent(const FObjectInitializer &ObjectInitializer)
	:Super(ObjectInitializer)
{
	LedgeData.bFindLedgeEnabled = true;
	LedgeMove = 0.0f;
	ClimbAngle = 30;
	HandsIKPosOffset = FVector(-5.f, 0, -10.0f);
	HandsIKTargetPosOffset = FVector(0.0f, 10.0f, 30.0f);
}

void UClimpCharacterMovementComponent::BeginPlay()
{
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(InitialCapsuleRadius, InitialCapsuleHalfHeight);
	CosClimbAngle = FMath::Cos(FMath::DegreesToRadians(ClimbAngle));
}

void UClimpCharacterMovementComponent::SetNormalizedVelocity()
{
	NormalizedVelocity = FVector::ZeroVector;
	const float MaxSpeed = GetMaxSpeed();

	if (MaxSpeed != 0.0f)
	{
		NormalizedVelocity = Velocity / MaxSpeed;
		NormalizedVelocity = CharacterOwner->GetTransform().InverseTransformVector(NormalizedVelocity);
	}
}

void UClimpCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == uint8(ECustomMove::CUSTOMMOVE_LedgeGrab))
	{
		bOrientRotationToMovement = false;
		bUseControllerDesiredRotation = true;
		LedgeData.bFindLedgeEnabled = false;
		LedgeData.CurrentLedge.bIsValid = false;
		
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([this]
		{
			LedgeData.bFindLedgeEnabled = true;
		});

		FTimerHandle TimerHandle;
		CharacterOwner->GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 1.0f, false);
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UClimpCharacterMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	Super::PhysFalling(DeltaTime, Iterations);

	if (FindLedge(CharacterOwner->GetActorLocation(), CharacterOwner->GetActorForwardVector(), LedgeData.CurrentLedge))
	{
		SetMovementMode(MOVE_Custom, uint8(ECustomMove::CUSTOMMOVE_LedgeGrab));
	}
}

void UClimpCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;

	const FVector ActorLocation = CharacterOwner->GetActorLocation();
	const FVector ActorRightVector = CharacterOwner->GetActorRightVector();
	const FVector ActorForwardVector = CharacterOwner->GetActorForwardVector();
	const FRotator ActorRotation = CharacterOwner->GetActorRotation();

	if (CustomMovementMode == uint8(ECustomMove::CUSTOMMOVE_LedgeGrab))
	{
		if (Acceleration.IsZero())
			Velocity = FVector::ZeroVector;

		CalcVelocity(DeltaTime, 0, true, GetMaxBrakingDeceleration());

		const FVector VelocityNormalized = Velocity.GetSafeNormal();
		const FVector RightVelocityNormalized = VelocityNormalized.ProjectOnTo(ActorRightVector);
		const FVector ForwardVelocityNormalized = VelocityNormalized.ProjectOnTo(ActorForwardVector);

		const FVector RightVelocity = Velocity.ProjectOnTo(ActorRightVector) * LedgeMove;
		const FVector ForwardVelocity = Velocity.ProjectOnTo(ActorForwardVector);

		const float RightDot = RightVelocityNormalized | ActorRightVector;
		const float ForwardDot = ForwardVelocityNormalized | ActorForwardVector;

		LedgeData.HelperLedge = LedgeData.CurrentLedge;

		if (FindLedge(ActorLocation + RightVelocity, ActorForwardVector, LedgeData.HelperLedge))
		{
		}
		else if (RightDot > 0.5f && FindLedge(ActorLocation, ActorRightVector, LedgeData.HelperLedge))
		{
		}
		else if (RightDot < -0.5f && FindLedge(ActorLocation, -ActorRightVector, LedgeData.HelperLedge))
		{
		}
		else if (FindLedge(ActorLocation, ActorForwardVector, LedgeData.HelperLedge))
		{
		}

		if (LedgeData.HelperLedge.bIsValid)
		{
			LedgeData.CurrentLedge = LedgeData.HelperLedge;
		}

#if 0
		if (ForwardDot > 0.5f)
		{
			// Check for a stand point over the wall
			if (FindStandPoint(LedgeData.CurrentLedge, 1.0f))
			{
				const float Duration = CharacterOwner->PlayAnimMontage(BracedHangToCrouchAnim);

				if (Duration > 0.0f)
				{
					LedgeData.bFindLedgeEnabled = false;
					SetMovementMode(MOVE_Flying);

					FTimerDelegate TimerDelegate;
					TimerDelegate.BindLambda([this]
					{
						SetMovementMode(MOVE_Walking);
					});

					FTimerHandle TimerHandle;
					CharacterOwner->GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, Duration, false);
				}
			}
			// Check for a chrouched stand point over the wall
			else if (FindStandPoint(LedgeData.CurrentLedge, 0.5f))
			{
				const float Duration = CharacterOwner->PlayAnimMontage(BracedHangToCrouchAnim);

				if (Duration > 0.0f)
				{
					LedgeData.bFindLedgeEnabled = false;
					CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(InitialCapsuleRadius);
					SetMovementMode(MOVE_Flying);

					FTimerDelegate TimerDelegate;
					TimerDelegate.BindLambda([this]
					{
						Crouch();
						SetMovementMode(MOVE_Walking);
					});

					FTimerHandle TimerHandle;
					CharacterOwner->GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, Duration, false);
				}
			}
		}
#endif

		if (LedgeData.CurrentLedge.bIsValid)
		{
			FVector NewLocation = FMath::VInterpTo(ActorLocation, LedgeData.CurrentLedge.PawnEdgePos, DeltaTime, 10.0f);
			NewLocation = NewLocation - ActorLocation;
			FRotator NewRotation = (-1.0f * LedgeData.CurrentLedge.EdgeNormal).Rotation();
			NewRotation = FMath::RInterpTo(ActorRotation, NewRotation, DeltaTime, 10.0f);

			FHitResult Hit(1.0f);
			SafeMoveUpdatedComponent(NewLocation, NewRotation, true, Hit);

			if (Hit.Time < 1.0f)
			{
				// Adjust and try again.
				HandleImpact(Hit, DeltaTime, NewLocation);
				SlideAlongSurface(NewLocation, (1.0f - Hit.Time), Hit.Normal, Hit, true);
			}

			// Setup Hand IKs
			FVector RightHandPreIKPos = CharacterOwner->GetMesh()->GetSocketLocation(RightHandPreIKPos_SocketName);
			GetHandIKPos(LedgeData.CurrentLedge.RightHandIKPos, LedgeData.CurrentLedge.RightHandIKTargetPos, RightHandPreIKPos, ActorForwardVector, ActorRightVector);

			FVector LeftHandPreIKPos = CharacterOwner->GetMesh()->GetSocketLocation(LeftHandPreIKPos_SocketName);
			GetHandIKPos(LedgeData.CurrentLedge.LeftHandIKPos, LedgeData.CurrentLedge.LeftHandIKTargetPos, LeftHandPreIKPos, ActorForwardVector, -ActorRightVector);
		}
	}
}

void UClimpCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetNormalizedVelocity();
}

bool UClimpCharacterMovementComponent::FindLedge(const FVector &Position, const FVector &Forward, FLedgeInfo &OutLedgeInfo, EDrawDebugTrace::Type DebugType)
{
	if (!LedgeData.bFindLedgeEnabled)
		return false;

	const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(ECC_Breach_Climbable);
	const TArray<AActor*> ActorsToIgnore = { CharacterOwner };
	const float SphereTraceRadius = 20.0f;
	const float MinimumLedgeSize = 10.0f;
	
	FVector Helper, StartTrace, EndTrace;
	Helper = Forward * (InitialCapsuleRadius + 10.0f);

	// Find posible edge top surface
	if (!OutLedgeInfo.bIsValid)
	{
		StartTrace = Position + Helper + FVector::UpVector * InitialCapsuleHalfHeight * 1.5f;
		EndTrace = Position + Helper /*+ FVector::UpVector * (InitialCapsuleHalfHeight * 0.5f)*/;
	}
	else
	{
		StartTrace	= Position + Helper + FVector::UpVector * (InitialCapsuleHalfHeight + 10.0f);
		EndTrace	= Position + Helper + FVector::UpVector * (InitialCapsuleHalfHeight * 0.5f);
	}


	FHitResult OutHit(1.0f);
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartTrace, EndTrace, TraceTypeQuery, true, ActorsToIgnore, DebugType, OutHit, true);

	bool bIsValidHit = OutHit.bBlockingHit && !OutHit.bStartPenetrating && (OutHit.ImpactNormal | FVector::UpVector) > CosClimbAngle;

	// Do a Sphere trace to check for lateral walls
	if (!bIsValidHit && 
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace, EndTrace, SphereTraceRadius, TraceTypeQuery, true, ActorsToIgnore, DebugType, OutHit, true) && 
		!OutHit.bStartPenetrating && 
		(OutHit.ImpactNormal | FVector::UpVector) > CosClimbAngle)
	{
		bIsValidHit = true;
	}

	if (bIsValidHit)
	{
		// Find edge front face
		Helper = (OutHit.ImpactPoint - Position).ProjectOnTo(FVector::UpVector);
		StartTrace = Position + Helper;
		StartTrace -= FVector::UpVector;

		Helper = OutHit.ImpactPoint - FVector::UpVector;
		EndTrace = Helper - StartTrace;
		EndTrace *= 1.1f;
		EndTrace += Helper;

		if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartTrace, EndTrace, TraceTypeQuery, true, ActorsToIgnore, DebugType, OutHit, true))
		{
			// Check if the destination has free space for place the actor
			StartTrace = OutHit.ImpactPoint + OutHit.ImpactNormal * (InitialCapsuleRadius * 1.1f);
			StartTrace -= FVector::UpVector * InitialCapsuleHalfHeight;

			FHitResult OutHit2(1.0f);
			UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, StartTrace, InitialCapsuleRadius, InitialCapsuleHalfHeight, UEngineTypes::ConvertToTraceType(UpdatedComponent->GetCollisionObjectType()), true, ActorsToIgnore, DebugType, OutHit2, true);

			if (!OutHit2.bBlockingHit)
			{
				OutLedgeInfo.bIsValid = true;
				OutLedgeInfo.EdgePos = OutHit.ImpactPoint;
				OutLedgeInfo.EdgeNormal = OutHit.ImpactNormal;
				OutLedgeInfo.PawnEdgePos = StartTrace;

				// Determine if it is a ledge for braced movement or a ledge for free hang movement.
				StartTrace = CharacterOwner->GetActorLocation() - (InitialCapsuleHalfHeight * 0.5f * FVector::UpVector);
				EndTrace = StartTrace + (Forward * InitialCapsuleRadius * 1.1f);

				OutLedgeInfo.bIsLedgeForBracedMov = UKismetSystemLibrary::SphereTraceSingle(
					GetWorld(), StartTrace, EndTrace, SphereTraceRadius,
					UEngineTypes::ConvertToTraceType(UpdatedComponent->GetCollisionObjectType()),
					true, ActorsToIgnore, DebugType, OutHit, true);
			}
			else
			{
				OutLedgeInfo.bIsValid = false;
			}

			return OutLedgeInfo.bIsValid;
		}
	}

	OutLedgeInfo.bIsValid = false;

	return false;
}

bool UClimpCharacterMovementComponent::FindStandPoint(FLedgeInfo& OutLedgeInfo, float PawnHalfHeightPercentage, EDrawDebugTrace::Type DebugType)
{
	const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(ECC_Breach_Climbable);
	const TArray<AActor*> ActorsToIgnore = { CharacterOwner };

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
	PawnHalfHeight *= PawnHalfHeightPercentage;

	FVector TestPosition = OutLedgeInfo.EdgePos;
	TestPosition += FVector::UpVector * 5.0f;
	TestPosition += FVector::UpVector * PawnHalfHeight;
	TestPosition -= OutLedgeInfo.EdgeNormal * PawnRadius;

	FHitResult OutHit(1.0f);
	UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), TestPosition, TestPosition, PawnRadius, PawnHalfHeight, UEngineTypes::ConvertToTraceType(UpdatedComponent->GetCollisionObjectType()), true, ActorsToIgnore, DebugType, OutHit, true);

	if (!OutHit.bBlockingHit && !OutHit.bStartPenetrating)
	{
		OutLedgeInfo.StandPoint = TestPosition;
		return true;
	}

	return false;
}

bool UClimpCharacterMovementComponent::GetHandIKPos(
	FVector& OutHandIKPos, 
	FVector &OutHandIKTargetPos,
	FVector &HandPreIKPos,
	const FVector& ActorForwardVector,
	const FVector& ActorRightVector) const
{
	FVector StartTrace = LedgeData.CurrentLedge.EdgePos + ActorForwardVector * 2.5f;
	HandPreIKPos = HandPreIKPos - StartTrace;
	HandPreIKPos = HandPreIKPos.ProjectOnToNormal(ActorRightVector);
	StartTrace += HandPreIKPos;

	const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(ECC_Breach_Climbable);
	const TArray<AActor*> ActorsToIgnore = { CharacterOwner };

	FHitResult OutHit(1.0f);
	UKismetSystemLibrary::LineTraceSingle(GetWorld(),
		StartTrace + FVector::UpVector * 40.0f,
		StartTrace - FVector::UpVector * 40.0f,
		TraceTypeQuery, true, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, OutHit, true);

	if (OutHit.bBlockingHit && !OutHit.bStartPenetrating)
	{
		OutHandIKPos = OutHit.ImpactPoint;
		OutHandIKPos += FVector::UpVector * HandsIKPosOffset.Z;
		OutHandIKPos += ActorForwardVector * HandsIKPosOffset.X;
		
		OutHandIKTargetPos = OutHandIKPos;
		OutHandIKTargetPos -= FVector::UpVector * HandsIKTargetPosOffset.Z;
		OutHandIKTargetPos += ActorRightVector * HandsIKTargetPosOffset.Y;

		return true;
	}

	return false;
}
