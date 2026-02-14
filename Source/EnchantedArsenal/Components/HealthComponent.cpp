#include "HealthComponent.h"

UHealthComponent::UHealthComponent() {
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = MaxHealth;
	CurrentHealth = MaxHealth;
}


void UHealthComponent::BeginPlay() {
	Super::BeginPlay();
}


void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UHealthComponent::ApplyDamage(float Damage) {
	CurrentHealth -= Damage;

	if (CurrentHealth <= 0) {
		Die();
	}
}

void UHealthComponent::Die() {
	OnDeath.Broadcast();
}

