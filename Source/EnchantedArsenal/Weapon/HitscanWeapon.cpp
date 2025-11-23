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

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    if (!MuzzleFlashSocket) return;

    FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

    FVector TraceStart = SocketTransform.GetLocation();

    FHitResult CrosshairHitResult;
    InstigatorPawn->CombatComp->TraceUnderCrosshairs(CrosshairHitResult);
    FVector TraceEnd = CrosshairHitResult.ImpactPoint;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Camera));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Visibility));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    FHitResult HitResult;
    UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), TraceStart, TraceEnd, ObjectTypes, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.1f);

    if (HitResult.bBlockingHit && HitResult.GetActor())
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Hit: " + HitResult.GetActor()->GetName());

        if (HasAuthority()) {
            if (UHealthComponent* HealthComp = HitResult.GetActor()->FindComponentByClass<UHealthComponent>()) {
                HealthComp->ApplyDamage(10.0f);
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Health: " + FString::SanitizeFloat(HealthComp->CurrentHealth));
            }

            if (ImpactParticles) {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.ImpactPoint);
            }

            if (ImpactSound) {
                UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, HitResult.ImpactPoint);
            }
        }

        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 8.f, 12, FColor::Yellow, false, 1.0f);
    }
}

void AHitscanWeapon::StartShoot() {
    Super::StartShoot();

    if (!HasAuthority()) return;

    if (!GetWorld()->GetTimerManager().IsTimerActive(ShootTimerHandle)) {
        FTimerDelegate ShootTimerDelegate = FTimerDelegate::CreateUObject(this, &AHitscanWeapon::Shoot);
        GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, ShootTimerDelegate, ShootRate, false);
    }
}