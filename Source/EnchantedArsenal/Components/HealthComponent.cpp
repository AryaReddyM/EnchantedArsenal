#include "HealthComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Net/UnrealNetwork.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"

UHealthComponent::UHealthComponent() {
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	CurrentHealth = MaxHealth;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

void UHealthComponent::BeginPlay() {
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
}


void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UHealthComponent::SetHealthBar(UProgressBar* InHealthBar) {
	HealthBar = InHealthBar;

	if (HealthBar) {
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}
}

void UHealthComponent::EnsureHealthBar() {
	if (HealthBar) return;

	AArsenalCharacter* Character = Cast<AArsenalCharacter>(GetOwner());
	if (!Character || !Character->IsLocallyControlled() || !Character->HUD) return;

	HealthBar = Cast<UProgressBar>(Character->HUD->GetWidgetFromName("HealthBar"));
}

void UHealthComponent::OnRep_CurrentHealth() {
	EnsureHealthBar();

	if (HealthBar) {
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}
}

void UHealthComponent::ApplyDamage(float Damage) {
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);

	EnsureHealthBar();

	if (HealthBar) {
		HealthBar->SetPercent(CurrentHealth / MaxHealth);
	}

	if (CurrentHealth <= 0) {
		Die();
	}
}

void UHealthComponent::Die() {
	OnDeath.Broadcast();
}

