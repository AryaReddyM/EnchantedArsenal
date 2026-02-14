#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "TimerManager.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Engine/TimerHandle.h"

void AProjectileWeapon::Shoot() {
    Super::Shoot();

    if (!HasAuthority()) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < ShootRate) {
        return;
    }

    LastFireTime = CurrentTime;


    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());

    if (MuzzleFlashSocket) {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        
        if (ProjectileClass && InstigatorPawn) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = InstigatorPawn;

            FHitResult CrosshairHitResult = InstigatorPawn->CombatComp->TraceUnderCrosshairs();

            FVector ToTarget = (CrosshairHitResult.ImpactPoint - SocketTransform.GetLocation());
            FRotator TargetRotation = ToTarget.Rotation();

            GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
        }
    }
}