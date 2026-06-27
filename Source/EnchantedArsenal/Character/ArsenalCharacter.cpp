#include "ArsenalCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Alembic/AbcGeom/IFaceSet.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/ProgressBar.h"
#include "EnchantedArsenal/Components/HealthComponent.h"
#include "EnchantedArsenal/Weapon/Weapon.h"
#include "EnchantedArsenal/Magic/Spell.h"
#include "EnchantedArsenal/Components/CombatComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnchantedArsenal/Components/MagicComponent.h"
#include "EnchantedArsenal/Gamemode/TeamsGameMode.h"
#include "EnchantedArsenal/GameState/ArsenalGameState.h"
#include "EnchantedArsenal/Physics/PhysicsData.h"
#include "EnchantedArsenal/PlayerStart/TeamPlayerStart.h"
#include "EnchantedArsenal/PlayerState/ArsenalPlayerState.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

////////////////////////////////////// Function Definitions //////////////////////////////////////

class UTextBlock;
//////////////// Constructor ////////////////
AArsenalCharacter::AArsenalCharacter() {
	// Enables Ticking
	PrimaryActorTick.bCanEverTick = true;

	// Enables Replication
	bReplicates = true;
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);

	// Create Components
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm Component"));
	SpringArmComp->SetupAttachment(CapsuleComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComp->SetupAttachment(SpringArmComp);

	HipCameraPosComp = CreateDefaultSubobject<UArrowComponent>(TEXT("Hip Camera Position Component"));
	HipCameraPosComp->SetupAttachment(SpringArmComp);

	AimCameraPosComp = CreateDefaultSubobject<UArrowComponent>(TEXT("Aim Camera Position Component"));
	AimCameraPosComp->SetupAttachment(GetMesh(), "head");
	
	PhysComp = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("Physics Animation Component"));

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	CombatComp->SetIsReplicated(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("Health Component"));
	HealthComp->SetIsReplicated(true);

	MagicComp = CreateDefaultSubobject<UMagicComponent>(TEXT("Magic Component"));
	MagicComp->SetIsReplicated(true);

	HeadshotBoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Headshot Box Collision Component"));
	HeadshotBoxCollisionComp->SetupAttachment(CapsuleComp);
	
	SpellLocationOnWheel = {
		{0, ESpellType::EST_Boulder},
		{45, ESpellType::EST_SpikeAdder}
	};
}

//////////////// Replication ////////////////
void AArsenalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArsenalCharacter, CombatComp);
	DOREPLIFETIME(AArsenalCharacter, HealthComp);
	DOREPLIFETIME(AArsenalCharacter, bIsAiming);
	DOREPLIFETIME(AArsenalCharacter, AttackType);
	DOREPLIFETIME(AArsenalCharacter, bIsReloading);
}

//////////////// Component Initialization ////////////////
void AArsenalCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	// Give Components a Reference to Character
	if (CombatComp) {
		CombatComp->Character = this;
	}
}

//////////////// BeginPlay ////////////////
void AArsenalCharacter::BeginPlay() {
	Super::BeginPlay();

	// Add the Enhanced Input mapping context for the local player
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Start In Default Move Speed
	MoveComp->MaxWalkSpeed = BaseWalkSpeed;

	// Adds HandleDeath Function to OnDeath Delegate in HealthComponent
	HealthComp->OnDeath.AddDynamic(this, &AArsenalCharacter::HandleDeath);

	// Create HUD
	if (IsLocallyControlled() && HUD) {
		HUD->AddToViewport();

		if (UProgressBar* FoundBar = Cast<UProgressBar>(HUD->GetWidgetFromName("HealthBar"))) {
			HealthComp->SetHealthBar(FoundBar);
		}

		if (AArsenalGameState* GS = GetWorld()->GetGameState<AArsenalGameState>()) {
			GS->OnTeamScoreChanged.AddDynamic(this, &AArsenalCharacter::HandleTeamScoreChanged);

			HandleTeamScoreChanged(ETeam::ET_BlueTeam, GS->BlueTeamScore);
			HandleTeamScoreChanged(ETeam::ET_RedTeam,  GS->RedTeamScore);
		}
	}
	
	// Initialize PlayerState
	ArsenalPlayerState = GetPlayerState<AArsenalPlayerState>();
	if (ArsenalPlayerState) {
		OnPlayerStateInit();
	}
}

//////////////// Tick ////////////////
void AArsenalCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Recoil Smoothing
	float NewRecoilPitch = FMath::FInterpTo(CurrentRecoilPitch, TargetRecoilPitch, DeltaTime, RecoilInterpSpeed);
	float NewRecoilYaw = FMath::FInterpTo(CurrentRecoilYaw, TargetRecoilYaw, DeltaTime, RecoilInterpSpeed);

	float DeltaPitch = NewRecoilPitch - CurrentRecoilPitch;
	float DeltaYaw = NewRecoilYaw - CurrentRecoilYaw;

	AddControllerPitchInput(DeltaPitch);
	AddControllerYawInput(DeltaYaw);

	CurrentRecoilPitch = NewRecoilPitch;
	CurrentRecoilYaw = NewRecoilYaw;

	if (FMath::IsNearlyEqual(CurrentRecoilPitch, TargetRecoilPitch, 0.01f)) {
		CurrentRecoilPitch = TargetRecoilPitch = 0.f;
	}

	if (FMath::IsNearlyEqual(CurrentRecoilYaw, TargetRecoilYaw, 0.01f)) {
		CurrentRecoilYaw = TargetRecoilYaw = 0.f;
	}

	// ADS Smoothing
	if (IsAiming()) {
		FVector CameraLocation = FMath::VInterpTo(CameraComp->GetComponentLocation(),
		                                          AimCameraPosComp->GetComponentLocation(), DeltaTime, ADSSpeed);
		FRotator CameraRotation = FMath::RInterpTo(CameraComp->GetComponentRotation(),
		                                           AimCameraPosComp->GetComponentRotation(), DeltaTime, ADSSpeed);
		FVector CameraScale = FMath::VInterpTo(CameraComp->GetComponentScale(), AimCameraPosComp->GetComponentScale(),
		                                       DeltaTime, ADSSpeed);
		CameraComp->SetWorldTransform(FTransform(CameraRotation.Quaternion(), CameraLocation, CameraScale));
	}
	else {
		FVector CameraLocation = FMath::VInterpTo(CameraComp->GetComponentLocation(),
		                                          HipCameraPosComp->GetComponentLocation(), DeltaTime, ADSSpeed);
		FRotator CameraRotation = FMath::RInterpTo(CameraComp->GetComponentRotation(),
		                                           HipCameraPosComp->GetComponentRotation(), DeltaTime, ADSSpeed);
		FVector CameraScale = FMath::VInterpTo(CameraComp->GetComponentScale(), HipCameraPosComp->GetComponentScale(),
		                                       DeltaTime, ADSSpeed);
		CameraComp->SetWorldTransform(FTransform(CameraRotation.Quaternion(), CameraLocation));
	}

	// Dynamic Crosshair
	float Velocity = GetVelocity().Size();
	float BaseSpread = 20.f;
	float MovementMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 600.f), FVector2D(0.f, 60.f), Velocity);
	float ADSMultiplier = (IsAiming() && CombatComp && CombatComp->SpawnedWeapon && CombatComp->SpawnedWeapon-> WeaponType != EWeaponType::EWT_Unarmed) ? 0.5f : 1.0f;

	TargetVisualSpread = (BaseSpread + MovementMultiplier) * ADSMultiplier;

	CurrentVisualSpread = FMath::FInterpTo(CurrentVisualSpread, TargetVisualSpread, DeltaTime, InterpSpeed);

	if (HUD) {
		float WeaponBaseSpread = 15.0f;
		float MovementEffect = 5.0f;
		float InterpSpeedForWeapon = 15.0f;

		if (CombatComp && CombatComp->SpawnedWeapon) {
			WeaponBaseSpread = CombatComp->SpawnedWeapon->CrosshairWeaponBaseSpread;
			MovementEffect = CombatComp->SpawnedWeapon->CrosshairMovementEffect;
			InterpSpeedForWeapon = CombatComp->SpawnedWeapon->CrosshairInterpSpeed;
		}

		float VelocityFactor = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 600.f), FVector2D(0.f, MovementEffect),
		                                                         Velocity);

		float TargetSpread = (WeaponBaseSpread + VelocityFactor) * ADSMultiplier;

		CurrentVisualSpread = FMath::FInterpTo(CurrentVisualSpread, TargetSpread, DeltaTime, InterpSpeedForWeapon);

		UImage* Top = Cast<UImage>(HUD->GetWidgetFromName("CrosshairUp"));
		UImage* Bottom = Cast<UImage>(HUD->GetWidgetFromName("CrosshairDown"));
		UImage* Left = Cast<UImage>(HUD->GetWidgetFromName("CrosshairLeft"));
		UImage* Right = Cast<UImage>(HUD->GetWidgetFromName("CrosshairRight"));

		if (Top && Bottom && Left && Right) {
			Top->SetRenderTranslation(FVector2D(0.f, -CurrentVisualSpread));
			Bottom->SetRenderTranslation(FVector2D(0.f, CurrentVisualSpread));
			Left->SetRenderTranslation(FVector2D(-CurrentVisualSpread, 0.f));
			Right->SetRenderTranslation(FVector2D(CurrentVisualSpread, 0.f));
		}
	}
	
	// Damage Indicator
	if (IsLocallyControlled() && CameraComp && ActiveDamageWidgets.Num() > 0) {
		const FVector CameraLoc = CameraComp->GetComponentLocation();
        
		for (int i = ActiveDamageWidgets.Num() - 1; i >= 0; i--) {
			UWidgetComponent* WidgetComp = ActiveDamageWidgets[i].Get();
            
			if (IsValid(WidgetComp)) {
				if (!WidgetComp->IsVisible()) {
					ActiveDamageWidgets.RemoveAt(i);
					continue;
				}

				const FVector CompLoc = WidgetComp->GetComponentLocation();
				FRotator TargetRot = (CameraLoc - CompLoc).Rotation();
				WidgetComp->SetWorldRotation(TargetRot);
			}
			else {
				ActiveDamageWidgets.RemoveAt(i);
			}
		}
	}
	
	// Spell Wheel
	if (SpellWheel) {
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC) {
			FVector2D MousePos;
			PC->GetMousePosition(MousePos.X, MousePos.Y);
		
			FVector2D CenterOffset = MousePos - UWidgetLayoutLibrary::GetViewportSize(GetWorld()) / 2;
			
			float Radians = FMath::Atan2(CenterOffset.Y, CenterOffset.X);
			float Degrees = FMath::RadiansToDegrees(Radians);
			
			Degrees += 90.0f;
			if (Degrees < 0) Degrees += 360.0f;
			
			CurrAngle = FMath::RoundToInt(Degrees / 45.0f) * 45;
			
			if (CurrAngle == 360) CurrAngle = 0;
		
			if (UImage* RedLine = Cast<UImage>(SpellWheel->GetWidgetFromName("RedLine"))) {
				RedLine->SetRenderTransformAngle(CurrAngle);
			}
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, FString::Printf(TEXT("Angle: %d"), CurrAngle));
		}
	}
}

void AArsenalCharacter::PossessedBy(AController* NewController) {
	Super::PossessedBy(NewController);
	ArsenalPlayerState = GetPlayerState<AArsenalPlayerState>();
	if (ArsenalPlayerState) {
		OnPlayerStateInit();
	}
}

//////////////// Input ////////////////
void AArsenalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Movement + Camera
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Look);

		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);

		// Weapon Equips
		EnhancedInputComponent->BindAction(EquipRifleAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Rifle);
		EnhancedInputComponent->BindAction(EquipSMGAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_SMG);
		EnhancedInputComponent->BindAction(EquipShotgunAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Shotgun);
		EnhancedInputComponent->BindAction(EquipPistolAction, ETriggerEvent::Started, this, &AArsenalCharacter::EquipWeapon, EWeaponType::EWT_Pistol);

		// Spell Equips
		EnhancedInputComponent->BindAction(OpenSpellWheelAction, ETriggerEvent::Started, this, &AArsenalCharacter::OpenSpellWheel);
		EnhancedInputComponent->BindAction(OpenSpellWheelAction, ETriggerEvent::Completed, this, &AArsenalCharacter::CloseSpellWheel);
		EnhancedInputComponent->BindAction(CastSelectedSpellAction, ETriggerEvent::Started, this, &AArsenalCharacter::CastSelectedSpell);
		
		// Reload
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AArsenalCharacter::Reload);

		// Aim
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AArsenalCharacter::Aim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArsenalCharacter::AimReleased);

		// Shoot
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AArsenalCharacter::Shoot);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AArsenalCharacter::StopShoot);
	}
}

////////////////////////////////////// Input Functions //////////////////////////////////////

//////////////// Move ////////////////
void AArsenalCharacter::Move(const FInputActionValue& Value) {
	FVector2D MoveAxisVector = Value.Get<FVector2D>();

	AddMovementInput(GetActorRightVector(), MoveAxisVector.X);
	AddMovementInput(GetActorForwardVector(), MoveAxisVector.Y);
}

//////////////// Look ////////////////
void AArsenalCharacter::Look(const FInputActionValue& Value) {
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X * -0.4F);
	AddControllerPitchInput(LookAxisVector.Y * 0.4F);
}

////////////////////////////////////// Combat / Magic Functions //////////////////////////////////////

//////////////// EquipWeapon / ServerEquipWeapon ////////////////
void AArsenalCharacter::EquipWeapon(EWeaponType WeaponType) {
	if (!CombatComp || bIsEquipping || CombatComp->SpawnedWeapon && CombatComp->SpawnedWeapon->WeaponType == WeaponType) return;

	if (bIsReloading) {
		if (HasAuthority()) {
			GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		}
		bIsReloading = false;
	}

	if (HasAuthority()) {
		ServerSetAttackType(EAttackType::EAT_Weapon);
		CombatComp->EquipWeapon(WeaponType);

		bIsEquipping = true;
		GetWorldTimerManager().SetTimer(EquipTimerHandle, FTimerDelegate::CreateLambda([this]() {
			bIsEquipping = false;
		}), CombatComp->GetEquipMontageLength(), false);
	}
	else {
		ServerEquipWeapon(WeaponType);
	}
}

void AArsenalCharacter::ServerEquipWeapon_Implementation(EWeaponType WeaponType) {
	EquipWeapon(WeaponType);
}

//////////////// EquipSpell / ServerEquipSpell ////////////////
void AArsenalCharacter::EquipSpell(ESpellType SpellType) {
	if (!MagicComp || MagicComp->IsSpellOnCooldown(SpellType) || bIsEquipping) return;
	
	if (bIsReloading) {
		if (HasAuthority()) {
			GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		}
		bIsReloading = false;
	}
	
	if (IsLocallyControlled() && HUD) {
		if (UTextBlock* AmmoText = Cast<UTextBlock>(HUD->GetWidgetFromName("AmmoText"))) {
			AmmoText->SetText(FText::FromString(""));
		}
	}

	if (HasAuthority()) {
		ServerSetAttackType(EAttackType::EAT_Magic);
		MagicComp->EquipSpell(SpellType);
		
		bIsEquipping = true;
		GetWorldTimerManager().SetTimer(EquipTimerHandle, FTimerDelegate::CreateLambda([this]() {
			bIsEquipping = false;
		}), MagicComp->GetEquipMontageLength(), false);
	}
	else {
		ServerEquipSpell(SpellType);
	}
}

void AArsenalCharacter::ServerEquipSpell_Implementation(ESpellType SpellType) {
	EquipSpell(SpellType);
}

//////////////// Reload / ServerReload ////////////////
void AArsenalCharacter::Reload() {
	if (bIsReloading || bIsEquipping) return;

	if (HasAuthority()) {
		bIsReloading = true;
		
		CombatComp->PlayReloadMontage();
        
		GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, FTimerDelegate::CreateLambda([this]() {
			bIsReloading = false;
			
			if (CombatComp && CombatComp->SpawnedWeapon) {
				CombatComp->SpawnedWeapon->Reload();
			}
		}), CombatComp->GetReloadMontageLength(), false);
	}
	else {
		bIsReloading = true;

		CombatComp->PlayReloadMontage();

		ServerReload();
	}
}

void AArsenalCharacter::ServerReload_Implementation() {
	Reload();
}

//////////////// Aim / AimReleased ////////////////
void AArsenalCharacter::Aim() {
	switch (AttackType) {
	case EAttackType::EAT_Unarmed:
		// Unarmed: No Aiming
		break;
	default:
		// Everything Else: Allow Aiming
		SetAiming(true);
	}
}

void AArsenalCharacter::AimReleased() {
	SetAiming(false);
}

//////////////// Shoot / StopShoot ////////////////
void AArsenalCharacter::Shoot() {
	if (bIsEquipping) return;

	// Shoots for Corresponding Attack Type
	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (CombatComp && !bIsReloading) {
			CombatComp->Shoot();
		}
		break;
	case EAttackType::EAT_Magic:
		if (MagicComp) {
			MagicComp->Cast();
		}
		break;
	default:
		break; 
	}
}

void AArsenalCharacter::StopShoot() {
	if (CombatComp) {
		CombatComp->StopShoot();
	}
}

//////////////// OpenSpellWheel / CastSelectedSpell ////////////////

void AArsenalCharacter::OpenSpellWheel()
{
    SpellWheel = CreateWidget<UUserWidget>(GetWorld(), SpellWheelRef, "SpellWheel");
    if (!SpellWheel) return;

    SpellWheel->AddToViewport();

    TArray<int32> AngleOrder = {0, 45, 90, 135, 180, 225, 270, 315};

    float Radius = 370.0f; 
    TArray<FVector2D> SlotOffsets;
    SlotOffsets.Add(FVector2D(0.0f, -Radius));
    SlotOffsets.Add(FVector2D(Radius * 0.707f, -Radius * 0.707f));
    SlotOffsets.Add(FVector2D(Radius, 0.0f));
    SlotOffsets.Add(FVector2D(Radius * 0.707f, Radius * 0.707f));
    SlotOffsets.Add(FVector2D(0.0f, Radius));
    SlotOffsets.Add(FVector2D(-Radius * 0.707f, Radius * 0.707f));
    SlotOffsets.Add(FVector2D(-Radius, 0.0f));
    SlotOffsets.Add(FVector2D(-Radius * 0.707f, -Radius * 0.707f));

    for (int32 i = 0; i < 8; ++i) {
        int32 TargetAngle = AngleOrder[i];
        
		FString IconWidgetName = FString::Printf(TEXT("Icon%d"), i + 1);
        UImage* FoundIconImage = Cast<UImage>(SpellWheel->GetWidgetFromName(*IconWidgetName));

        if (SpellLocationOnWheel.Contains(TargetAngle)) {
            ESpellType ConfiguredType = SpellLocationOnWheel[TargetAngle];
            UTexture2D* SpellTexture = nullptr;

            if (MagicComp) {
                SpellTexture = MagicComp->GetSpellIconForType(ConfiguredType);
            }

            if (FoundIconImage && SpellTexture) {
                FoundIconImage->SetBrushFromTexture(SpellTexture);
                FoundIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

                if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FoundIconImage->Slot)) {
                    CanvasSlot->SetPosition(SlotOffsets[i]);
                }
                continue; 
            }
        }

        if (FoundIconImage) {
            FoundIconImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;
    PC->SetShowMouseCursor(true);
    FVector2D HalfViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld()) / 2.0f;
    PC->SetMouseLocation(FMath::TruncToInt(HalfViewportSize.X), FMath::TruncToInt(HalfViewportSize.Y));
}

void AArsenalCharacter::CloseSpellWheel() {
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC) {
		PC->SetShowMouseCursor(false);
	}
	
	if (SpellWheel) SpellWheel->RemoveFromParent();
	SpellWheel = nullptr;
	
	if (SpellLocationOnWheel.Contains(CurrAngle)) {
		EquipSpell(SpellLocationOnWheel[CurrAngle]);
	}
}

void AArsenalCharacter::CastSelectedSpell() {
}

////////////////////////////////////// Utility Functions //////////////////////////////////////

//////////////// Handling Death And Reset ////////////////
void AArsenalCharacter::HandleDeath(AActor* Damager) {
	// Destroys Character Equipment
	if (CombatComp) CombatComp->UnequipWeapon();
	if (MagicComp) MagicComp->UnequipSpell();
	
	if (HasAuthority()) {
		if (ATeamsGameMode* GM = GetWorld()->GetAuthGameMode<ATeamsGameMode>()) {
			GM->HandleScore(Damager);
		}
		
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC) {
			PC->UnPossess(); 

			if (GetWorld() && CameraComp) {
				// Player -> Spectator
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				ASpectatorPawn* SpawnedSpectator = GetWorld()->SpawnActor<ASpectatorPawn>(ASpectatorPawn::StaticClass(), 
					CameraComp->GetComponentLocation(), CameraComp->GetComponentRotation(), SpawnParams);

				if (SpawnedSpectator) {
					PC->Possess(SpawnedSpectator); 
				}
				
				// Ragdolls Body
				MulticastStartRagdoll();
				
				// Timer Until Respawn
				GetWorldTimerManager().SetTimer(DeathTimer, FTimerDelegate::CreateLambda([this, PC, SpawnedSpectator]() {
					ResetPlayer(PC, SpawnedSpectator);
				}), DeathDelay, false);
			}
		}
	}
}

void AArsenalCharacter::MulticastStartRagdoll_Implementation() {
	if (!PhysicsData || !PhysComp || !GetMesh()) return;

	PhysComp->SetSkeletalMeshComponent(GetMesh());
	PhysComp->ApplyPhysicalAnimationSettings("pelvis", PhysicsData->RagdollPhysicalAnimData);

	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (MoveComp) MoveComp->DisableMovement();
}

void AArsenalCharacter::MulticastEndRagdoll_Implementation() {
	if (!GetMesh()) return;

	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeTransform(FTransform(
		FQuat(FRotator(0.0f, -90.0f, 0.0f)), // Rotation
		FVector(0.0f, 1.0f, -90.0f), // Location
		FVector(0.9375f, 0.9375f, 0.9375f) // Scale
	));

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (MoveComp) MoveComp->SetMovementMode(MOVE_Walking);

}

void AArsenalCharacter::HandleTeamScoreChanged(ETeam Team, float NewScore) {
	if (!HUD) return;

	const FString WidgetName = (Team == ETeam::ET_BlueTeam) ? TEXT("BlueScoreText") : TEXT("RedScoreText");
	const FString LabelText  = (Team == ETeam::ET_BlueTeam) ? TEXT("Blue Score: {0}") : TEXT("Red Score: {0}");

	if (UTextBlock* ScoreTextBlock = Cast<UTextBlock>(HUD->GetWidgetFromName(*WidgetName))) {
		ScoreTextBlock->SetText(FText::Format(FText::FromString(LabelText), FMath::FloorToInt(NewScore)));
	}
}

void AArsenalCharacter::ClientShowDamageIndicator_Implementation(AArsenalCharacter* Victim, float Damage) { 
	if (!Victim) return; 

    UUserWidget* Indicator = nullptr; 
    UWidgetComponent* TargetWidgetComp = nullptr;
    
    TArray<UWidgetComponent*> WidgetComps; 
    Victim->GetComponents<UWidgetComponent>(WidgetComps); 
    
    for (UWidgetComponent* WC : WidgetComps) { 
        UUserWidget* W = WC ? WC->GetWidget() : nullptr; 
        if (W && W->GetWidgetFromName(TEXT("DamageText"))) { 
            Indicator = W; 
            TargetWidgetComp = WC;
            break; 
        } 
    } 

    if (!Indicator || !TargetWidgetComp) return; 

    TargetWidgetComp->SetWidgetSpace(EWidgetSpace::World);

    if (IsLocallyControlled()) {
        TargetWidgetComp->SetVisibility(true);
        TargetWidgetComp->SetActive(true);
        TargetWidgetComp->UpdateWidget();
    	
    	TargetWidgetComp->SetTranslucentSortPriority(10);
        
        if (!ActiveDamageWidgets.Contains(TargetWidgetComp)) {
            ActiveDamageWidgets.Add(TargetWidgetComp);
        }
    }
	
	const float Now = GetWorld()->GetTimeSeconds(); 
    FDamageTally& Tally = DamageTallies.FindOrAdd(Victim); 
    if (Now - Tally.LastTime > DamageAccumulateWindow) { 
        Tally.Total = 0.f; 
    } 
    Tally.Total += Damage; 
    Tally.LastTime = Now; 

    if (UTextBlock* DamageText = Cast<UTextBlock>(Indicator->GetWidgetFromName(TEXT("DamageText")))) { 
        DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Tally.Total))); 
    } 

    if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Indicator->GetClass())) { 
        for (UWidgetAnimation* Anim : WidgetClass->Animations) { 
            if (!Anim) continue; 
            FString AnimName = Anim->GetName(); 
            AnimName.RemoveFromEnd(TEXT("_INST")); 
            if (AnimName == TEXT("DamageAnimation")) { 
                Indicator->PlayAnimation(Anim); 
                break; 
            } 
        } 
    }
}

void AArsenalCharacter::ResetPlayer(APlayerController* PC, APawn* Spectator) {
	MulticastEndRagdoll();
	SetSpawnPoint();

	PC->UnPossess();
	Spectator->Destroy();
	PC->Possess(this);

	HealthComp->ResetHealth();
	CombatComp->ResetAmmo();
	EquipWeapon(EWeaponType::EWT_Rifle);
}

//////////////// AddRecoil ////////////////
void AArsenalCharacter::AddRecoil(float Min, float Max) {
	TargetRecoilPitch += FMath::RandRange(Min, Max);
	TargetRecoilYaw += FMath::RandRange(Min, Max);
}

//////////////// SetAiming / ServerSetAiming ////////////////
void AArsenalCharacter::SetAiming(bool bInAiming) {
	bIsAiming = bInAiming;
	ServerSetAiming(bInAiming);

	MoveComp->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void AArsenalCharacter::ServerSetAiming_Implementation(bool bInAiming) {
	bIsAiming = bInAiming;

	MoveComp->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

//////////////// ServerSetAttackType ////////////////
void AArsenalCharacter::ServerSetAttackType_Implementation(EAttackType NewType) {
	if (AttackType == NewType) return;

	AttackType = NewType;

	switch (AttackType) {
	case EAttackType::EAT_Weapon:
		if (MagicComp) MagicComp->UnequipSpell();
		break;
	case EAttackType::EAT_Magic:
		if (CombatComp) CombatComp->UnequipWeapon();
		break;
	default:
		if (CombatComp) CombatComp->UnequipWeapon();
		if (MagicComp) MagicComp->UnequipSpell();
		break;
	}
}

FHitResult AArsenalCharacter::TraceUnderCrosshairs() {
	FHitResult TraceHitResult;

	if (!IsLocallyControlled()) return FHitResult();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return FHitResult();

	int32 SizeX = 0, SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return FHitResult();

	const FVector2D CrosshairLocation(SizeX * 0.5f, SizeY * 0.5f);

	FVector CrosshairWorldPos;
	FVector CrosshairWorldDir;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC, CrosshairLocation, CrosshairWorldPos, CrosshairWorldDir))
		return FHitResult();

	const FVector Start = CrosshairWorldPos;
	const FVector End = Start + CrosshairWorldDir * 80000;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (CombatComp && CombatComp->SpawnedWeapon) Params.AddIgnoredActor(CombatComp->SpawnedWeapon);

	if (MagicComp && MagicComp->HeldSpell) {
		Params.AddIgnoredActor(MagicComp->HeldSpell);
	}

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility, Params);

	if (!TraceHitResult.bBlockingHit) {
		TraceHitResult.ImpactPoint = End;
	}

	return TraceHitResult;
}

void AArsenalCharacter::SetTeamColor(ETeam Team) {
	if (GetMesh() == nullptr) return;

	switch (Team) {
	case ETeam::ET_BlueTeam:
		for (int i = 0; i < BlueMaterials.Num(); i++) {
			GetMesh()->SetMaterial(i, BlueMaterials[i]);
		}
		BluePlayers++;
		break;
	case ETeam::ET_RedTeam:
		for (int i = 0; i < RedMaterials.Num(); i++) {
			GetMesh()->SetMaterial(i, RedMaterials[i]);
		}
		RedPlayers++;
		break;
	default:
		for (int i = 0; i < NoMaterials.Num(); i++) {
			GetMesh()->SetMaterial(i, NoMaterials[i]);
		}
		break;
	}
}

void AArsenalCharacter::SetSpawnPoint() {
	if (HasAuthority() && ArsenalPlayerState->GetTeam() != ETeam::ET_NoTeam) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayerStarts;

		for (AActor* Start : PlayerStarts) {
			ATeamPlayerStart* TeamPlayerStart = Cast<ATeamPlayerStart>(Start);
			if (TeamPlayerStart && TeamPlayerStart->Team == ArsenalPlayerState->GetTeam()) {
				TeamPlayerStarts.Add(TeamPlayerStart);
			}
		}
		if (TeamPlayerStarts.Num() > 0) {
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
		}
	}
}

void AArsenalCharacter::OnPlayerStateInit() {
	SetTeamColor(ArsenalPlayerState->GetTeam());
	SetSpawnPoint();
}

////////////////////////////////////// Getters //////////////////////////////////////

//////////////// GetWeapon ////////////////
AWeapon* AArsenalCharacter::GetWeapon() {
	if (CombatComp == nullptr) return nullptr;

	return CombatComp->SpawnedWeapon;
}

//////////////// IsWeaponEquipped ////////////////
bool AArsenalCharacter::IsWeaponEquipped() {
	return (CombatComp && CombatComp->SpawnedWeapon);
}

//////////////// IsAiming ////////////////
bool AArsenalCharacter::IsAiming() {
	return bIsAiming;
}

//////////////// IsShooting ////////////////
bool AArsenalCharacter::IsShooting() {
	return (CombatComp && GetWorldTimerManager().IsTimerActive(CombatComp->ShootTimer));
}

////////////////////////////////////// Replication Notifies //////////////////////////////////////

//////////////// OnRep_AttackType ////////////////
void AArsenalCharacter::OnRep_AttackType() {
}

//////////////// OnRep_PlayerState ////////////////
void AArsenalCharacter::OnRep_PlayerState() {
	Super::OnRep_PlayerState();
	ArsenalPlayerState = GetPlayerState<AArsenalPlayerState>();
	if (ArsenalPlayerState) {
		OnPlayerStateInit();
	}
}

//////////////// OnRep_bIsReloading ////////////////
void AArsenalCharacter::OnRep_bIsReloading() {
	if (!CombatComp) return;

	if (bIsReloading) {
		CombatComp->PlayReloadMontage();
	}
	else {
		CombatComp->StopReloadMontage();
	}
}
