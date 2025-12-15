#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon() {
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponType);
	DOREPLIFETIME(AWeapon, FireType);
}

void AWeapon::BeginPlay() {
	Super::BeginPlay();
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