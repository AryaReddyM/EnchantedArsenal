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
    if (!InstigatorPawn) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < ShootRate) {
        return;
    }

    LastFireTime = CurrentTime;

    FHitResult CrosshairHitResult;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);

    bool bHitSomething = CrosshairHitResult.bBlockingHit;
    FVector ImpactPoint = bHitSomething ? CrosshairHitResult.ImpactPoint : CrosshairHitResult.TraceEnd;

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

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);
}

void AHitscanWeapon::ServerShoot_Implementation(bool bHitSomething, const FVector_NetQuantize& ImpactPoint) {
    ServerProcessShot(bHitSomething, ImpactPoint);
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

        MulticastImpactEffects(ImpactPoint);
    }
}


void AHitscanWeapon::MulticastImpactEffects_Implementation(FVector_NetQuantize ImpactPoint) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (InstigatorPawn && InstigatorPawn->IsLocallyControlled()) {
        return;
    }

    if (ImpactParticles) {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, ImpactPoint);
    }

    if (ImpactSound) {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, ImpactPoint);
    }

    if (MuzzleFlashParticles) {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, MuzzleLocation);
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