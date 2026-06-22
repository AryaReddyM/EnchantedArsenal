#include "HitscanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "DrawDebugHelpers.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "Components/BoxComponent.h"

void AHitscanWeapon::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn || !InstigatorPawn->IsLocallyControlled()) return;

    if (CurrentAmmo <= 0) {
        if (InstigatorPawn->CombatComp) {
            InstigatorPawn->CombatComp->StopShoot();
        }
        InstigatorPawn->Reload();
        return;
    }

    FHitResult CrosshairHit = InstigatorPawn->TraceUnderCrosshairs();
    const bool bHit = CrosshairHit.bBlockingHit;
    const FVector ImpactPoint = bHit ? CrosshairHit.ImpactPoint : CrosshairHit.TraceEnd;

    const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleSocket) {
        const FVector MuzzleLoc = MuzzleSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
        LocalShootEffects(MuzzleLoc, ImpactPoint, CrosshairHit);
    }

    InstigatorPawn->AddRecoil(RecoilMin, RecoilMax);

    if (HasAuthority()) {
        AuthoritativeShot(bHit, ImpactPoint);
    }
    else {
        ServerShoot(bHit, ImpactPoint);
    }
    
    if (CurrentAmmo <= 0) {
        InstigatorPawn->Reload();
    }
}

void AHitscanWeapon::ServerShoot_Implementation(bool bHitSomething, const FVector_NetQuantize& ImpactPoint) {
    AuthoritativeShot(bHitSomething, ImpactPoint);
}

void AHitscanWeapon::AuthoritativeShot(bool bHitSomething, const FVector& ImpactPoint) {
    if (!TryConsumeAmmo(1)) return;

    ServerProcessShot(bHitSomething, ImpactPoint);
    MulticastImpactEffects(bHitSomething, ImpactPoint);
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

    if (BestTarget && AArsenalPlayerState::IsHostile(InstigatorPawn, BestTarget)) {
        if (UHealthComponent* HealthComp = BestTarget->FindComponentByClass<UHealthComponent>()) {
            if (CheckForHeadshot(BestTarget, ImpactPoint)) {
                HealthComp->ApplyDamage(HeadshotDamage, GetOwner());
            }
            else {
                HealthComp->ApplyDamage(Damage, GetOwner());
            }

            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Health: ") + FString::SanitizeFloat(HealthComp->CurrentHealth));
        }
    }
}


void AHitscanWeapon::MulticastImpactEffects_Implementation(bool bHit, FVector_NetQuantize ImpactPoint) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn || InstigatorPawn->IsLocallyControlled()) return;

    const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleSocket && MuzzleFlashParticles) {
        const FVector MuzzleLoc = MuzzleSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, MuzzleLoc);
    }
    
    if (MuzzleSocket && MuzzleSound) {
        const FVector MuzzleLoc = MuzzleSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), MuzzleSound, MuzzleLoc, 0.1f);
    }

    if (bHit) {
        if (ImpactParticles) {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, ImpactPoint);
        }

        if (ImpactSound) {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, ImpactPoint);
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
    if (InstigatorPawn && !InstigatorPawn->IsLocallyControlled()) {
        InstigatorPawn->CombatComp->PlayShootMontage();
    }
}

void AHitscanWeapon::LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& CrosshairHitResult) {
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.1f);
    DrawDebugSphere(GetWorld(), TraceEnd, 8.0f, 12, FColor::Yellow, false, 0.1f);

    if (MuzzleFlashParticles) {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashParticles, TraceStart);
    }
    
    if (MuzzleSound) {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), MuzzleSound, TraceStart, 0.5f);
    }

    if (CrosshairHitResult.bBlockingHit) {
        if (ImpactParticles) {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, CrosshairHitResult.ImpactPoint);
        }

        if (ImpactSound) {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, CrosshairHitResult.ImpactPoint);
        }
    }
}