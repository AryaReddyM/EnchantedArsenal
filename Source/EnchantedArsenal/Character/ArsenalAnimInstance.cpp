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

	if (!ArsenalCharacter) return;

	bIsInAir = ArsenalCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = ArsenalCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bIsCrouching = ArsenalCharacter->GetMovementComponent()->IsCrouching();

	bWeaponEquipped = ArsenalCharacter->IsWeaponEquipped();
	EquippedWeapon = ArsenalCharacter->GetWeapon();

	bAiming = ArsenalCharacter->IsAiming();
	bShooting = ArsenalCharacter->IsShooting() && EquippedWeapon->FireType != EFireType::EFT_SemiAuto;

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

	const FVector Vel = ArsenalCharacter->GetVelocity();
	const FVector Vel2D(Vel.X, Vel.Y, 0.f);

	const FVector LocalVel = ArsenalCharacter->GetActorTransform()
		.InverseTransformVectorNoScale(Vel2D);

	const float ForwardAxisTarget = FMath::Clamp(LocalVel.X / 600.f, -1.f, 1.f);
	const float RightAxisTarget = FMath::Clamp(LocalVel.Y / 600.f, -1.f, 1.f);

	ForwardAxis = FMath::FInterpTo(ForwardAxis, ForwardAxisTarget, DeltaSeconds, 10.f);
	RightAxis = FMath::FInterpTo(RightAxis, RightAxisTarget, DeltaSeconds, 10.f);

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

	float TargetYaw = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, ArsenalCharacter->GetActorRotation()).Yaw;
	Yaw = FMath::FInterpTo(Yaw, TargetYaw, DeltaSeconds, 6.0f);

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ArsenalCharacter->GetMesh()) {
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;

		ArsenalCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}
