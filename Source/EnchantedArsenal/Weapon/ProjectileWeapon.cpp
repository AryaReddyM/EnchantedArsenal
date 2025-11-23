#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "TimerManager.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"

void AProjectileWeapon::Shoot() {
    Super::Shoot();

    if (!HasAuthority()) return;

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());

    if (MuzzleFlashSocket) {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        
        if (ProjectileClass && InstigatorPawn) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = InstigatorPawn;

            FHitResult CrosshairHitResult;
            InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);

            FVector ToTarget = (CrosshairHitResult.ImpactPoint - SocketTransform.GetLocation());
            FRotator TargetRotation = ToTarget.Rotation();

            GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
        }
    }
}

void AProjectileWeapon::StartShoot() {
    Super::StartShoot();

    if (!HasAuthority()) return;

    if (!GetWorld()->GetTimerManager().IsTimerActive(ShootTimerHandle)) {
        FTimerDelegate ShootTimerDelegate = FTimerDelegate::CreateUObject(this, &AProjectileWeapon::Shoot);
        GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, ShootTimerDelegate, ShootRate, false);
    }
}