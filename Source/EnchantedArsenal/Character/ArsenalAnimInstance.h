#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "ArsenalAnimInstance.generated.h"

class AArsenalCharacter;
class AWeapon;
enum class EWeaponType :uint8;

UCLASS()
class ENCHANTEDARSENAL_API UArsenalAnimInstance : public UAnimInstance {
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(BlueprintReadWrite, Category = Character, meta = (AllowPrivateAccess = true))
	AArsenalCharacter* ArsenalCharacter;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	float Speed;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsInAir;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsAccelerating;
	
	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bWeaponEquipped;

	AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = true))
	int WeaponTypeIndex;
	
	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bIsCrouching;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	bool bAiming;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	float YawOffset;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	float Lean;

	UPROPERTY(BlueprintReadWrite, Category = "AimOffset", meta = (AllowPrivateAccess = true))
	float Pitch;

	UPROPERTY(BlueprintReadOnly, Category = "AimOffset", meta = (AllowPrivateAccess = true))
	float Yaw;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = true))
	FTransform GripTransform;

	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
};
