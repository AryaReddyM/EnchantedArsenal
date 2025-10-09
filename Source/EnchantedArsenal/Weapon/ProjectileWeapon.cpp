#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "TimerManager.h"

void AProjectileWeapon::Shoot(const FVector HitTarget) {
    Super::Shoot(HitTarget);

    if (!HasAuthority()) return;

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    APawn* InstigatorPawn = Cast<APawn>(GetOwner());

    if (MuzzleFlashSocket) {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        
        if (ProjectileClass && InstigatorPawn) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = InstigatorPawn;

            FVector ToTarget = (HitTarget - SocketTransform.GetLocation());
            FRotator TargetRotation = ToTarget.Rotation();

            GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
        }
    }
}

void AProjectileWeapon::StartShoot(const FVector& HitTarget) {
    Super::StartShoot(HitTarget);

    if (!HasAuthority()) return;

    if (!GetWorld()->GetTimerManager().IsTimerActive(ShootTimerHandle)) {
        FTimerDelegate ShootTimerDelegate = FTimerDelegate::CreateUObject(this, &AProjectileWeapon::Shoot, HitTarget);
        GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, ShootTimerDelegate, ShootRate, false);
    }
}