#include "HealthComponent.h"
#include "Components/ProgressBar.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"

void UHealthComponent::BeginPlay() {
	Super::BeginPlay();
	
	if (GetOwner() && GetOwner()->HasAuthority()) {
		ResetHealth();
	}
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

void UHealthComponent::SetHealthBar(UProgressBar* InHealthBar) {
	HealthBar = InHealthBar;
	if (HealthBar) {
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}
}

void UHealthComponent::ResetHealth() {
	CurrentHealth = MaxHealth;
	
	OnRep_CurrentHealth();
}

void UHealthComponent::OnRep_CurrentHealth() {
	if (HealthBar)
	{
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}
}

void UHealthComponent::ApplyDamage(float Damage, AActor* Damager) {
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);

	OnRep_CurrentHealth();

	AArsenalCharacter* Victim = Cast<AArsenalCharacter>(GetOwner());
	if (AArsenalCharacter* Attacker = Cast<AArsenalCharacter>(Damager)) {
		Attacker->ClientShowDamageIndicator(Victim, Damage);
	}

	if (CurrentHealth <= 0) {
		OnDeath.Broadcast(Damager);
	}
}
