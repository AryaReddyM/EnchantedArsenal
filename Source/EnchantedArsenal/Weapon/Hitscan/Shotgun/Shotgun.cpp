#include "Shotgun.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

void AShotgun::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn || !InstigatorPawn->CombatComp) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < ShootRate) return;

    if (FireType == EFireType::EWT_SemiAuto && InstigatorPawn->CombatComp->SemiShotCounter > 0) return;

    LastFireTime = CurrentTime;

    HitLocations.Reset();

    if (InstigatorPawn->IsLocallyControlled()) {
        InstigatorPawn->PlayShootMontage(WeaponType);
    }

    FHitResult CrosshairHitResult;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);

    FVector CameraLoc = InstigatorPawn->CameraComp->GetComponentLocation();
    FVector CrosshairImpactPoint = CrosshairHitResult.bBlockingHit ? CrosshairHitResult.ImpactPoint : CrosshairHitResult.TraceEnd;
    FVector AimDir = (CrosshairImpactPoint - CameraLoc).GetSafeNormal();

    for (int i = 0; i < Pellets; i++) {
        FVector PelletDir = AimDir;
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::UpVector);
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::RightVector);

        FVector End = CameraLoc + PelletDir * 10000.0f;
        FHitResult PelletHit;
        GetWorld()->LineTraceSingleByChannel(PelletHit, CameraLoc, End, ECollisionChannel::ECC_Visibility);

        HitLocations.Add(PelletHit.bBlockingHit ? PelletHit.ImpactPoint : End);
    }

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket) return;

    bool bHitSomething = false;
    MuzzleLocation = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();

    for (const FVector& ImpactPoint : HitLocations) {
        if (InstigatorPawn->IsLocallyControlled()) {
            bHitSomething = CrosshairHitResult.bBlockingHit;
            LocalShootEffects(MuzzleLocation, ImpactPoint, CrosshairHitResult);
        }

        if (!HasAuthority()) {
            ServerShoot(bHitSomething, ImpactPoint);
        }
        else {
            ServerProcessShot(bHitSomething, ImpactPoint);
        }
    }

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);

    if (InstigatorPawn->CombatComp) {
        InstigatorPawn->CombatComp->SetSemiCounter(InstigatorPawn->CombatComp->SemiShotCounter + 1);
    }
}