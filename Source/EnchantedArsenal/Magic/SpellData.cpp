#include "SpellData.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

UFXSystemComponent* FSpellFX::SpawnAtLocation(const UObject* WorldContext, FVector Location, FRotator Rotation, FVector Scale) const {
	if (UNiagaraSystem* Niagara = Cast<UNiagaraSystem>(System)) {
		return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, Niagara, Location, Rotation, Scale);
	}
	if (UParticleSystem* Cascade = Cast<UParticleSystem>(System)) {
		return UGameplayStatics::SpawnEmitterAtLocation(WorldContext, Cascade, Location, Rotation, Scale);
	}
	return nullptr;
}

UFXSystemComponent* FSpellFX::SpawnAttached(USceneComponent* Parent, FName Socket) const {
	if (UNiagaraSystem* Niagara = Cast<UNiagaraSystem>(System)) {
		return UNiagaraFunctionLibrary::SpawnSystemAttached(Niagara, Parent, Socket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}
	if (UParticleSystem* Cascade = Cast<UParticleSystem>(System)) {
		return UGameplayStatics::SpawnEmitterAttached(Cascade, Parent, Socket);
	}
	return nullptr;
}

float USpellData::GetDamage(const FGameplayTagContainer& ActiveTags) const {
	float Total = Damage;
	for (const FTagModifier& Mod : TagModifiers) {
		if (ActiveTags.HasTag(Mod.Tag)) {
			Total += Mod.BonusDamage;
		}
	}
	return Total;
}

float USpellData::GetExplosionDamage(const FGameplayTagContainer& ActiveTags, float DistanceFromCenter) const {
	const float Base = GetDamage(ActiveTags);
	if (GetExplosionRadius(ActiveTags) <= 0.0f) return Base;

	const float Alpha = FMath::Clamp(DistanceFromCenter / GetExplosionRadius(ActiveTags), 0.0f, 1.0f);
	return Base * FMath::Lerp(1.0f, MinDamageMultiplier, Alpha);
}

float USpellData::GetExplosionRadius(const FGameplayTagContainer& ActiveTags) const {
	float Total = ExplosionRadius;
	for (const FTagModifier& Mod : TagModifiers) {
		if (ActiveTags.HasTag(Mod.Tag)) {
			Total += Mod.BonusExplosionRadius;
		}
	}
	
	return Total;
}

UStaticMesh* USpellData::GetMesh(const FGameplayTagContainer& ActiveTags) const {
	UStaticMesh* M = Mesh;
	for (const FTagModifier& Mod : TagModifiers) {
		if (ActiveTags.HasTag(Mod.Tag)) {
			M = Mod.MeshOverride;
		}
	}
	return M;
}

UMaterialInterface* USpellData::GetMaterial(const FGameplayTagContainer& ActiveTags) const {
	UMaterialInterface* Mat = Material;
	for (const FTagModifier& Mod : TagModifiers) {
		if (ActiveTags.HasTag(Mod.Tag)) {
			Mat = Mod.MaterialOverride;
		}
	}
	return Mat;
}
