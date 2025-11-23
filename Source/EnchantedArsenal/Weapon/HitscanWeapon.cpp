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

void AHitscanWeapon::Shoot() {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket) return;

    FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
    FVector TraceStart = SocketTransform.GetLocation();

    FHitResult CrosshairHitResult;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);

    FVector TraceEnd = CrosshairHitResult.bBlockingHit ?
        CrosshairHitResult.ImpactPoint :
        CrosshairHitResult.TraceEnd;

    LocalShootEffects(TraceStart, TraceEnd);

    if (!HasAuthority()) {
        ServerShoot(TraceEnd);
    }
    else {
        ServerProcessShot(TraceStart, TraceEnd);
    }
}

void AHitscanWeapon::StartShoot() {
    Super::StartShoot();

    if (!GetWorld()->GetTimerManager().IsTimerActive(ShootTimerHandle)) {
        FTimerDelegate ShootTimerDelegate = FTimerDelegate::CreateUObject(this, &AHitscanWeapon::Shoot);
        GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, ShootTimerDelegate, ShootRate, false);
    }
}

void AHitscanWeapon::ServerShoot_Implementation(const FVector_NetQuantize& TraceEnd) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (!MuzzleFlashSocket) return;

    FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
    FVector TraceStart = SocketTransform.GetLocation();

    ServerProcessShot(TraceStart, TraceEnd);
}


void AHitscanWeapon::ServerProcessShot(const FVector& TraceStart, const FVector& TraceEnd) {
    AArsenalCharacter* InstigatorPawn = Cast<AArsenalCharacter>(GetOwner());
    if (!InstigatorPawn) return;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(InstigatorPawn);
    ActorsToIgnore.Add(this);

    FHitResult HitResult;
    UKismetSystemLibrary::LineTraceSingleForObjects(
        GetWorld(),
        TraceStart,
        TraceEnd,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        HitResult,
        true
    );

    if (HitResult.bBlockingHit && HitResult.GetActor()) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Hit:" + HitResult.GetActor()->GetActorLabel());

        if (UHealthComponent* HealthComp = HitResult.GetActor()->FindComponentByClass<UHealthComponent>()) {
            HealthComp->ApplyDamage(10.f);

            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Health:" + FString::SanitizeFloat(HealthComp->CurrentHealth));
        }

        if (ImpactParticles) {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.ImpactPoint);
        }

        if (ImpactSound) {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, HitResult.ImpactPoint);
        }
    }
}

void AHitscanWeapon::LocalShootEffects(const FVector& TraceStart, const FVector& TraceEnd) {
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.1f);
    DrawDebugSphere(GetWorld(), TraceEnd, 8.f, 12, FColor::Yellow, false, 1.0f);
}