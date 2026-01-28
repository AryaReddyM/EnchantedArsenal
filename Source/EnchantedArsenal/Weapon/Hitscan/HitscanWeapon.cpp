#include "HitscanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "TimerManager.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "DrawDebugHelpers.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Components/BoxComponent.h"

void AHitscanWeapon::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn || !InstigatorPawn->CombatComp) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < ShootRate) return;

    if (FireType == EFireType::EFT_SemiAuto && InstigatorPawn->CombatComp->SemiShotCounter > 0) return;

    LastFireTime = CurrentTime;

    if (InstigatorPawn->IsLocallyControlled()) {
        InstigatorPawn->PlayShootMontage();

        FHitResult CrosshairHit;
        InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHit);

        const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
        if (!MuzzleSocket) return;
        MuzzleLocation = MuzzleSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();

        LocalShootEffects(MuzzleLocation, CrosshairHit.bBlockingHit ? CrosshairHit.ImpactPoint : CrosshairHit.TraceEnd, CrosshairHit);
    }

    bool bHitSomething = false;
    FVector ImpactPoint = FVector::ZeroVector;
    FHitResult CrosshairHit;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHit);
    bHitSomething = CrosshairHit.bBlockingHit;
    ImpactPoint = bHitSomething ? CrosshairHit.ImpactPoint : CrosshairHit.TraceEnd;

    if (!HasAuthority()) {
        ServerShoot(bHitSomething, ImpactPoint);
    }
    else {
        ServerProcessShot(bHitSomething, ImpactPoint);
    }

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);

    if (InstigatorPawn->CombatComp) {
        InstigatorPawn->CombatComp->SetSemiCounter(InstigatorPawn->CombatComp->SemiShotCounter + 1);
    }
}

void AHitscanWeapon::ServerShoot_Implementation(bool bHitSomething, const FVector_NetQuantize& ImpactPoint) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());

    ServerProcessShot(bHitSomething, ImpactPoint);

    MulticastImpactEffects(ImpactPoint);

    MulticastPlayShootAnimation();
}

void AHitscanWeapon::ServerProcessShot(bool bHitSomething, const FVector& ImpactPoint) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    if (!bHitSomething) {
        return;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(InstigatorPawn);
    ActorsToIgnore.Add(this);

    TArray<AActor*> OverlappedActors;
    const float HitRadius = 30.f;

    bool bAnyOverlap = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactPoint, HitRadius, ObjectTypes, nullptr, ActorsToIgnore, OverlappedActors);

    if (!bAnyOverlap) {
        return;
    }

    AActor* BestTarget = nullptr;
    float BestDistSqr = FLT_MAX;

    for (AActor* Actor : OverlappedActors) {
        float DistSqr = FVector::DistSquared(Actor->GetActorLocation(), ImpactPoint);
        if (DistSqr < BestDistSqr) {
            BestDistSqr = DistSqr;
            BestTarget = Actor;
        }
    }

    if (BestTarget) {
        if (UHealthComponent* HealthComp = BestTarget->FindComponentByClass<UHealthComponent>()) {
            if (CheckForHeadshot(BestTarget, ImpactPoint)) {
                HealthComp->ApplyDamage(HeadshotDamage);
            }
            else {
                HealthComp->ApplyDamage(Damage);
            }

            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Health: ") + FString::SanitizeFloat(HealthComp->CurrentHealth));
        }
    }
}


void AHitscanWeapon::MulticastImpactEffects_Implementation(FVector_NetQuantize ImpactPoint) {
    if (ImpactParticles) {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, ImpactPoint);
    }

    if (ImpactSound) {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, ImpactPoint);
    }

    const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleSocket && MuzzleFlashParticles) {
        FVector MuzzleLoc = MuzzleSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, MuzzleLoc);
    }
}

void AHitscanWeapon::LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& CrosshairHitResult) {
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.1f);
    DrawDebugSphere(GetWorld(), TraceEnd, 8.0f, 12, FColor::Yellow, false, 0.1f);

    if (CrosshairHitResult.bBlockingHit) {
        if (ImpactParticles) {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, CrosshairHitResult.ImpactPoint);
        }

        if (ImpactSound) {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, CrosshairHitResult.ImpactPoint);
        }

        if (MuzzleFlashParticles) {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, MuzzleLocation);
        }
    }
}

bool AHitscanWeapon::CheckForHeadshot(AActor* HitActor, FVector ImpactPoint) {
    AArsenalCharacter* Character = Cast<AArsenalCharacter>(HitActor);

    if (!Character) return false;

    FBox CharacterHeadshotCollision = Character->HeadshotBoxCollisionComp->CalcBounds(Character->HeadshotBoxCollisionComp->GetComponentTransform()).GetBox();

    if (CharacterHeadshotCollision.IsInside(ImpactPoint)) {
        return true;
    }

    return false;
}

void AHitscanWeapon::MulticastPlayShootAnimation_Implementation() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (InstigatorPawn) {
        InstigatorPawn->PlayShootMontage();
    }
}