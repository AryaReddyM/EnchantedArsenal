#include "Weapon.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon() {
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BaseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BaseRoot"));
	SetRootComponent(BaseRoot);
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
	GripPoint->SetupAttachment(WeaponMesh);
	
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponType);
	DOREPLIFETIME(AWeapon, FireType);
	DOREPLIFETIME(AWeapon, CurrentAmmo);
}

void AWeapon::BeginPlay() {
	Super::BeginPlay();
	
	if (HasAuthority()) {
		CurrentAmmo = MagSize;
	}
}

void AWeapon::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AWeapon::Shoot() {

}

void AWeapon::StartShoot() {

}

void AWeapon::StopShoot() {
}

void AWeapon::SetWeaponType(EWeaponType InWeaponType) {
	WeaponType = InWeaponType;
	SetWeaponMesh.Broadcast();
}

void AWeapon::OnRep_WeaponType() {
	SetWeaponMesh.Broadcast();
}

void AWeapon::OnRep_FireType() {
}

void AWeapon::OnRep_CurrentAmmo() {
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);
}

bool AWeapon::TryConsumeAmmo(int AmmoAmount) {
	if (CurrentAmmo <= 0) return false;

	if (HasAuthority()) {
		CurrentAmmo -= AmmoAmount;
		OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);
	}
	return true;
}