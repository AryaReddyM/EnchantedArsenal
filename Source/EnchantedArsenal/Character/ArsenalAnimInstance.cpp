// Fill out your copyright notice in the Description page of Project Settings.


#include "ArsenalAnimInstance.h"
#include "ArsenalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UArsenalAnimInstance::NativeInitializeAnimation() {
	Super::NativeInitializeAnimation();

	ArsenalCharacter = Cast<AArsenalCharacter>(TryGetPawnOwner());
}

void UArsenalAnimInstance::NativeUpdateAnimation(float DeltaSeconds) {
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (ArsenalCharacter == nullptr) {
		ArsenalCharacter = Cast<AArsenalCharacter>(TryGetPawnOwner());
	}

	if (ArsenalCharacter == nullptr) return;

	FVector Velocity = ArsenalCharacter->GetVelocity();
	Velocity.Z = 0.0F;
	Speed = Velocity.Size();

	bIsInAir = ArsenalCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = ArsenalCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bIsCrouching = ArsenalCharacter->GetMovementComponent()->IsCrouching();
}
