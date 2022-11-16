// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ECRCharacter.h"

#include "Gameplay/ECRCharacterAttributesAsset.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gameplay/ActorAttributeComponent.h"
#include "Kismet/KismetSystemLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AECRCharacter

AECRCharacter::AECRCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Rotate when the controller rotates in yaw. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character doesn't move in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 360.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 420.f;
	GetCharacterMovement()->AirControl = 0.05f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create health component
	ParameterHealthComponent = CreateDefaultSubobject<UActorAttributeComponent>(TEXT("Health"));
	ParameterHealthComponent->SetIsReplicated(true);
	ParameterHealthComponent->ProcessPlayerParameterChanged.BindUObject(this, &AECRCharacter::ProcessHealthChange);

	// Bind damage events
	OnTakeRadialDamage.AddDynamic(this, &AECRCharacter::OnReceiveAnyRadialDamage);
}


void AECRCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttributesAsset)
	{
		if (HasAuthority())
		{
			if (ParameterHealthComponent)
			{
				FString Message = FString::Printf(TEXT("new max value is %f"), AttributesAsset->DefaultMaxHealth);
				ParameterHealthComponent->SetMaxValue(AttributesAsset->DefaultMaxHealth);
				ParameterHealthComponent->ResetCurrentValueToMax();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ECRCharacter %s has not initialized health component"),
				       *(UKismetSystemLibrary::GetDisplayName(this)))
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ECRCharacter %s doesn't have attributes asset"),
		       *(UKismetSystemLibrary::GetDisplayName(this)))
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AECRCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	// PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	// PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AECRCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AECRCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AECRCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AECRCharacter::LookUpAtRate);
}

void AECRCharacter::SetupControllerBehaviour(const float Speed, const bool bIsFalling, const bool bMontageIsPlaying)
{
	if (Speed > 0 && !bIsFalling && !bMontageIsPlaying)
	{
		bUseControllerRotationYaw = true;
	}
	else
	{
		bUseControllerRotationYaw = false;
	}
}

void AECRCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AECRCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AECRCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AECRCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}


void AECRCharacter::ProcessHealthChange(const float NewHealth, const float MaxHealth)
{
	if (NewHealth < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dead"))
	}
}


// ReSharper disable once CppMemberFunctionMayBeConst
void AECRCharacter::OnReceiveAnyRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                             FVector Origin, FHitResult HitInfo, AController* InstigatedBy,
                                             AActor* DamageCauser)
{
	GEngine->AddOnScreenDebugMessage(3, 5, {255, 0, 0},
	                                 FString::Printf(TEXT("C Received damage %f"), Damage));
	if (ParameterHealthComponent)
	{
		ParameterHealthComponent->ApplyDamage(Damage);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ECRCharacter %s has not initialized health component"),
		       *(UKismetSystemLibrary::GetDisplayName(this)))
	}
}
