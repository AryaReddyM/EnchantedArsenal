#include "Shotgun.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"

void AShotgun::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < ShootRate) {
        return;
    }

    if (FireType == EFireType::EWT_SemiAuto) {
        if (SemiShotCounter > 0) return;

        InstigatorPawn->PlayShootMontage(InstigatorPawn->CombatComp->bAiming);
    }

    LastFireTime = CurrentTime;

    HitLocations.Reset();

    FHitResult CrosshairHitResult;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);

    for (int i = 1; i <= Pellets; i++) {
        FHitResult TempHitResult;

        FVector Start = InstigatorPawn->CameraComp->GetComponentLocation();

        FVector CrosshairImpactPoint = CrosshairHitResult.bBlockingHit ? CrosshairHitResult.ImpactPoint : CrosshairHitResult.TraceEnd;

        FVector AimDir = (CrosshairImpactPoint - Start).GetSafeNormal();

        FVector PelletDir = AimDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::UpVector);
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::RightVector);

        FVector End = Start + PelletDir * 10000.0f;

        GetWorld()->LineTraceSingleByChannel(TempHitResult, Start, End, ECollisionChannel::ECC_Visibility);

        HitLocations.Add(TempHitResult.bBlockingHit ? TempHitResult.ImpactPoint : End);
    }

    for (int i = 0; i < HitLocations.Num(); i++) {
        bool bHitSomething = CrosshairHitResult.bBlockingHit;
        FVector ImpactPoint = HitLocations[i];

        const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
        if (!MuzzleFlashSocket) return;

        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        MuzzleLocation = SocketTransform.GetLocation();

        if (InstigatorPawn->IsLocallyControlled()) {
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

    SemiShotCounter++;
}