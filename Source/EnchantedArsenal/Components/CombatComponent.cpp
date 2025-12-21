#include "CombatComponent.h"

#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EnchantedArsenal/PlayerController/ArsenalPlayerController.h"
#include "EnchantedArsenal/HUD/ArsenalHUD.h"
#include "EnchantedArsenal/Weapon/Weapon.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, SemiShotCounter);
}

void UCombatComponent::BeginPlay() {
	Super::BeginPlay();

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::EquipWeapon(EWeaponType WeaponType) {
	if (!Character || !Character->HasAuthority())
		return;

	if (SpawnedWeapon) {
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}

	TSubclassOf<AWeapon> WeaponClass = nullptr;
	switch (WeaponType) {
	case EWeaponType::EWT_Rifle:   WeaponClass = Rifle; break;
	case EWeaponType::EWT_Shotgun: WeaponClass = Shotgun; break;
	case EWeaponType::EWT_SMG:     WeaponClass = SMG; break;
	case EWeaponType::EWT_Pistol:  WeaponClass = Pistol; break;
	default: return;
	}
	if (!WeaponClass)
		return;

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FTransform::Identity, Params);
	if (!NewWeapon)
		return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	USkeletalMeshComponent* WeaponMesh = NewWeapon->GetWeaponMesh();

	if (!CharMesh || !WeaponMesh) {
		NewWeapon->Destroy();
		return;
	}

	const FName HandSocket(TEXT("RightHandSocket"));
	const FName GripSocket(TEXT("GripSocket"));

	if (!WeaponMesh->DoesSocketExist(GripSocket)) {
		NewWeapon->Destroy();
		return;
	}

	const FVector OriginalScale = NewWeapon->GetActorScale3D();

	FTransform GripToRoot = WeaponMesh->GetSocketTransform(GripSocket, RTS_Component).Inverse();
	FTransform HandWorldTransform = CharMesh->GetSocketTransform(HandSocket, RTS_World);

	FTransform DesiredWeaponTransform = GripToRoot * HandWorldTransform;
	DesiredWeaponTransform.SetScale3D(FVector(1.f));

	NewWeapon->SetActorTransform(DesiredWeaponTransform);

	NewWeapon->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);

	NewWeapon->SetActorScale3D(OriginalScale);

	SpawnedWeapon = NewWeapon;
	SpawnedWeapon->SetOwner(Character);
}

void UCombatComponent::DestroyWeapon() {
	if (SpawnedWeapon) SpawnedWeapon->Destroy();
}

void UCombatComponent::SetAiming(bool bInAiming) {
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming) {
	bAiming = bInAiming;

	if (Character) {
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::Shoot(bool bTriggered) {
	if (!SpawnedWeapon) return;

	bShootButtonPressed = bTriggered;

	if (bShootButtonPressed) {
		ServerShoot();
	}
}

void UCombatComponent::ServerShoot_Implementation() {
	MultiShoot();
}

void UCombatComponent::MultiShoot_Implementation() {
	if (!SpawnedWeapon) return;

	if (Character) {
		if (SpawnedWeapon->FireType == EFireType::EWT_Auto) {
			Character->PlayShootMontage(bAiming);
		}
		SpawnedWeapon->Shoot();
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult) {
	TraceHitResult = FHitResult();

	if (!Character) return;

	if (!Character->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return;

	const FVector2D CrosshairLocation(SizeX * 0.5f, SizeY * 0.5f);

	FVector CrosshairWorldPos;
	FVector CrosshairWorldDir;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC, CrosshairLocation, CrosshairWorldPos, CrosshairWorldDir))
		return;

	const FVector Start = CrosshairWorldPos;
	const FVector End = Start + CrosshairWorldDir * TRACE_LENGTH;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	if (SpawnedWeapon) Params.AddIgnoredActor(SpawnedWeapon);

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility, Params);

	if (!TraceHitResult.bBlockingHit) {
		TraceHitResult.ImpactPoint = End;
	}
}

void UCombatComponent::SetSemiCounter(int Counter) {
	if (!SpawnedWeapon) return;

	if (GetOwnerRole() < ROLE_Authority) {
		ServerSetSemiCounter(Counter);
	}
	else {
		SemiShotCounter = Counter;
		OnRep_SemiShotCounter();
	}
}

void UCombatComponent::ServerSetSemiCounter_Implementation(int32 NewCounter) {
	SemiShotCounter = NewCounter;
	OnRep_SemiShotCounter();
}

void UCombatComponent::ResetSemiCounter() {
	if (GetOwnerRole() < ROLE_Authority) {
		ServerResetSemiCounter();
	}
	else {
		SemiShotCounter = 0;
		OnRep_SemiShotCounter();
	}
}

void UCombatComponent::ServerResetSemiCounter_Implementation() {
	SemiShotCounter = 0;
	OnRep_SemiShotCounter();
}

bool UCombatComponent::ServerResetSemiCounter_Validate() {
	return true;
}

bool UCombatComponent::CanShoot() {
	if (!SpawnedWeapon) return false;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	return (CurrentTime - LastEquipTime) >= SpawnedWeapon->EquipDelay;
}

void UCombatComponent::OnRep_SpawnedWeapon() {
	if (!Character || !SpawnedWeapon)
		return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	USkeletalMeshComponent* WeaponMesh = SpawnedWeapon->GetWeaponMesh();

	if (!CharMesh || !WeaponMesh)
		return;

	const FName HandSocket(TEXT("RightHandSocket"));
	const FName GripSocket(TEXT("GripSocket"));

	const FVector OriginalScale = SpawnedWeapon->GetActorScale3D();

	FTransform GripToRoot = WeaponMesh->GetSocketTransform(GripSocket, RTS_Component).Inverse();
	FTransform HandWorldTransform = CharMesh->GetSocketTransform(HandSocket, RTS_World);

	FTransform DesiredWeaponTransform = GripToRoot * HandWorldTransform;
	DesiredWeaponTransform.SetScale3D(FVector(1.f));

	SpawnedWeapon->SetActorTransform(DesiredWeaponTransform);

	SpawnedWeapon->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);

	SpawnedWeapon->SetActorScale3D(OriginalScale);

	SpawnedWeapon->SetOwner(Character);
}

void UCombatComponent::OnRep_SemiShotCounter() {
}
