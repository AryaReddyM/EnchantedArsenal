#include "CombatComponent.h"

#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SpawnedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
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
	if (Character == nullptr) return;

	AWeapon* TempWeapon = NewObject<AWeapon>(this, AWeapon::StaticClass());
	TempWeapon->SetWeaponType(WeaponType);
		
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		FActorSpawnParameters SpawnInfo;
		FTransform HandSocketTransform = HandSocket->GetSocketTransform(Character->GetMesh());

		SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(Weapon, HandSocketTransform, SpawnInfo);

		HandSocket->AttachActor(SpawnedWeapon, Character->GetMesh());
	}
		
	SpawnedWeapon->SetOwner(Character);
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
