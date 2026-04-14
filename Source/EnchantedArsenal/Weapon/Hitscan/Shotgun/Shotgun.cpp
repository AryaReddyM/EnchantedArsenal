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

    if (CurrentTime - LastShootTime < ShootRate) return;

    if (FireType == EFireType::EFT_SemiAuto && InstigatorPawn->CombatComp->SemiShotCounter > 0) return;

    LastShootTime = CurrentTime;

    if (InstigatorPawn->IsLocallyControlled() && InstigatorPawn->CombatComp) {
        InstigatorPawn->CombatComp->PlayShootMontage();
    }

    FHitResult CrosshairHitResult = InstigatorPawn->TraceUnderCrosshairs();

    FVector CameraLoc = InstigatorPawn->CameraComp->GetComponentLocation();
    FVector CrosshairImpactPoint = CrosshairHitResult.bBlockingHit ? CrosshairHitResult.ImpactPoint : CrosshairHitResult.TraceEnd;
    FVector AimDir = (CrosshairImpactPoint - CameraLoc).GetSafeNormal();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket) return;
    MuzzleLocation = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();

    FCollisionQueryParams PelletParams;
    PelletParams.AddIgnoredActor(InstigatorPawn);
    PelletParams.AddIgnoredActor(this);

    for (int i = 0; i < Pellets; i++) {
        FVector PelletDir = AimDir;
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::UpVector);
        PelletDir = PelletDir.RotateAngleAxis(FMath::RandRange(-PelletAngle, PelletAngle), FVector::RightVector);

        const FVector End = CameraLoc + PelletDir * 10000.0f;
        FHitResult PelletHit;
        GetWorld()->LineTraceSingleByChannel(PelletHit, CameraLoc, End, ECollisionChannel::ECC_Visibility, PelletParams);

        const bool bPelletHit = PelletHit.bBlockingHit;
        const FVector PelletImpact = bPelletHit ? PelletHit.ImpactPoint : End;

        if (InstigatorPawn->IsLocallyControlled()) {
            LocalShootEffects(MuzzleLocation, PelletImpact, PelletHit);
        }

        if (!HasAuthority()) {
            ServerShoot(bPelletHit, PelletImpact);
        }
        else {
            ServerProcessShot(bPelletHit, PelletImpact);
        }
    }

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);

    if (InstigatorPawn->CombatComp) {
        InstigatorPawn->CombatComp->SetSemiCounter(InstigatorPawn->CombatComp->SemiShotCounter + 1);
    }
}