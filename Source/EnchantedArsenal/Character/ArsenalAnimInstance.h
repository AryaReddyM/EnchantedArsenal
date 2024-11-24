// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ArsenalAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ENCHANTEDARSENAL_API UArsenalAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(BlueprintReadWrite, Category = Character, meta = (AllowPrivateAccess = true))
	class AArsenalCharacter* ArsenalCharacter;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	float Speed;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsInAir;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsAccelerating;
	
	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsCrouching;
};
