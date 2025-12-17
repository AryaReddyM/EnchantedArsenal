#include "ArsenalAnimInstance.h"
#include "ArsenalCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnchantedArsenal/Weapon/Weapon.h"

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

	bIsInAir = ArsenalCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = ArsenalCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bIsCrouching = ArsenalCharacter->GetMovementComponent()->IsCrouching();

	bWeaponEquipped = ArsenalCharacter->IsWeaponEquipped();
	EquippedWeapon = ArsenalCharacter->GetWeapon();

	bAiming = ArsenalCharacter->IsAiming();

	FVector Velocity = ArsenalCharacter->GetVelocity();
	float LastSpeed = Speed;
	if (bWeaponEquipped) {
		Speed = Velocity.Size();
	}
	else {
		Speed = FMath::FInterpTo(LastSpeed, Velocity.Size(), DeltaSeconds, ArsenalCharacter->IdleWalkRunInterpSpeed);
	}

	FRotator AimRotation = ArsenalCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ArsenalCharacter->GetVelocity());
	float LastYawOffset = YawOffset;

	if (Speed > 0.0f) {
		YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	}
	else {
		YawOffset = FMath::FInterpTo(LastYawOffset, 0.0f, DeltaSeconds, 2.0f);
	}

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ArsenalCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float LeanInterp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
	Lean = FMath::Clamp(LeanInterp, -90.0f, 90.0f);

	Pitch = ArsenalCharacter->GetBaseAimRotation().Pitch;
	if (Pitch >= 180.0f) {
		Pitch -= 360.0f;
	}

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ArsenalCharacter->GetMesh()) {
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"));
		FVector OutPosition;
		FRotator OutRotation;

		ArsenalCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}
