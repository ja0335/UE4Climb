// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "ClimpCharacterMovementComponent.generated.h"


UENUM(BlueprintType)
enum class ECustomMove : uint8
{
	CUSTOMMOVE_None							UMETA(DisplayName = "CUSTOMMOVE_None"),
	CUSTOMMOVE_LedgeGrab					UMETA(DisplayName = "CUSTOMMOVE_LedgeGrab"),
};

USTRUCT(BlueprintType)
struct FLedgeInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	uint8 bIsValid : 1;

	/** Is ledge for braced movement? If not it is a ledge for free hang movement */
	UPROPERTY(BlueprintReadOnly)
		uint8 bIsLedgeForBracedMov : 1;

	UPROPERTY(BlueprintReadOnly)
		uint8 bCanStand : 1;

	UPROPERTY(BlueprintReadOnly)
		FVector EdgePos;

	UPROPERTY(BlueprintReadOnly)
		FVector EdgeNormal;

	UPROPERTY(BlueprintReadOnly)
		FVector PawnEdgePos;

	UPROPERTY(BlueprintReadOnly)
		FVector RightHandIKPos;

	UPROPERTY(BlueprintReadOnly)
		FVector RightHandIKTargetPos;

	UPROPERTY(BlueprintReadOnly)
		FVector LeftHandIKPos;

	UPROPERTY(BlueprintReadOnly)
		FVector LeftHandIKTargetPos;

	UPROPERTY(BlueprintReadOnly)
		FVector StandPoint;
};

USTRUCT(BlueprintType)
struct FLedgeData
{
	GENERATED_BODY()

		uint8 bFindLedgeEnabled : 1;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo CurrentLedge;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo BackSideEdge;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo JumpRightLedge;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo JumpLeftLedge;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo JumpUpLedge;

	UPROPERTY(BlueprintReadOnly)
		FLedgeInfo JumpDownLedge;
	
	FLedgeInfo HelperLedge;
};

/**
 * 
 */
UCLASS()
class CLIMB_API UClimpCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
		
public:
	UClimpCharacterMovementComponent(const FObjectInitializer &ObjectInitializer);

	void BeginPlay() override;
	
	void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;
	
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void SetNormalizedVelocity();

	virtual bool FindLedge(const FVector &Position, const FVector &Forward, FLedgeInfo &OutLedgeInfo, EDrawDebugTrace::Type DebugType = EDrawDebugTrace::Type::None);

	virtual bool FindStandPoint(FLedgeInfo &OutLedgeInfo, float PawnHalfHeightPercentage = 1.0f, EDrawDebugTrace::Type DebugType = EDrawDebugTrace::Type::None);

	bool GetHandIKPos(FVector &OutHandIKPos, FVector &OutHandIKTargetPos, FVector &HandPreIKPos, const FVector &ActorForwardVector, const FVector &ActorRightVector) const;

protected:

	UPROPERTY(BlueprintReadWrite, Category = "Climb")
	float ClimbAngle;

	float CosClimbAngle;

	UPROPERTY(BlueprintReadOnly, Category = "Climb")
	FVector NormalizedVelocity;

	UPROPERTY(BlueprintReadWrite, Category = "Climb")
	float LedgeMove;

	UPROPERTY(BlueprintReadOnly, Category = "Climb")
	FLedgeData LedgeData;

	UPROPERTY(EditDefaultsOnly, Category = "Climb")
	UAnimMontage * BracedHangToCrouchAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Climb")
	FName RightHandPreIKPos_SocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Climb")
	FName LeftHandPreIKPos_SocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Climb")
	FVector HandsIKPosOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Climb")
	FVector HandsIKTargetPosOffset;

	
	float InitialCapsuleRadius;
	float InitialCapsuleHalfHeight;
};
