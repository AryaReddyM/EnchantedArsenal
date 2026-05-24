#include "Shotgun.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

void AShotgun::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn || !InstigatorPawn->IsLocallyControlled()) return;

    if (CurrentAmmo <= 0) {
        if (InstigatorPawn->CombatComp) {
            InstigatorPawn->CombatComp->StopShoot();
        }
        return;
    }

    const FVector CameraLoc = InstigatorPawn->CameraComp->GetComponentLocation();
    const FHitResult CrosshairHit = InstigatorPawn->TraceUnderCrosshairs();
    const FVector CrosshairImpact = CrosshairHit.bBlockingHit ? CrosshairHit.ImpactPoint : CrosshairHit.TraceEnd;
    const FVector AimDir = (CrosshairImpact - CameraLoc).GetSafeNormal();

    const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    const FVector MuzzleLoc = MuzzleSocket ? MuzzleSocket->GetSocketLocation(GetWeaponMesh()) : GetActorLocation();

    const bool bAuth = HasAuthority();
    if (bAuth && !TryConsumeAmmo(Pellets)) return;

    for (int i = 0; i < Pellets; i++) {
        FVector PelletDir = AimDir;
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::UpVector);
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::RightVector);
        const FVector End = CameraLoc + (PelletDir * 10000.0f);

        FHitResult PelletHit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(InstigatorPawn);
        GetWorld()->LineTraceSingleByChannel(PelletHit, CameraLoc, End, ECC_Visibility, Params);

        const bool bHit = PelletHit.bBlockingHit;
        const FVector ImpactPoint = bHit ? PelletHit.ImpactPoint : End;

        LocalShootEffects(MuzzleLoc, ImpactPoint, PelletHit);

        if (bAuth) {
            ServerProcessShot(bHit, ImpactPoint);
        }
    }

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);

    if (bAuth) {
        MulticastImpactEffects(false, FVector::ZeroVector);
        MulticastPlayShootAnimation();
    }
    else {
        ServerShotgunFire(CameraLoc, CrosshairImpact);
    }
}

void AShotgun::ServerShotgunFire_Implementation(FVector_NetQuantize CameraLoc, FVector_NetQuantize CrosshairImpact) {
    AuthoritativeShotgunFire(CameraLoc, CrosshairImpact);
}

void AShotgun::AuthoritativeShotgunFire(const FVector& CameraLoc, const FVector& CrosshairImpact) {
    if (!TryConsumeAmmo(Pellets)) return;

    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    const FVector AimDir = (CrosshairImpact - CameraLoc).GetSafeNormal();

    for (int i = 0; i < Pellets; i++) {
        FVector PelletDir = AimDir;
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::UpVector);
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::RightVector);
        const FVector End = CameraLoc + (PelletDir * 10000.0f);

        FHitResult PelletHit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(InstigatorPawn);
        GetWorld()->LineTraceSingleByChannel(PelletHit, CameraLoc, End, ECC_Visibility, Params);

        const bool bHit = PelletHit.bBlockingHit;
        const FVector ImpactPoint = bHit ? PelletHit.ImpactPoint : End;

        ServerProcessShot(bHit, ImpactPoint);
    }

    MulticastImpactEffects(false, FVector::ZeroVector);
    MulticastPlayShootAnimation();
}
