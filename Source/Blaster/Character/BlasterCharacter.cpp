// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "../HUD/OverheadWidget.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "../Weapon/Weapon.h"
#include "../Component/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;	// ī�޶�� ����
	CameraBoom->bUsePawnControlRotation = true;	// ���콺�� �Է��� �� ��Ʈ�ѷ��� �Բ� ī�޶� ȸ�� ����

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamers"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	// �̰� ��������� ��ũ���Ⱑ ����������.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// ĸ���浹ü�� ���̷�Ż �޽��� ī�޶�� �浹���� �ʴ´�.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	LocalPlayerName = FString(TEXT("Player"));
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// � Ŭ���̾�Ʈ�� �� ������ �������� ������ �� �ִ�.
	// �Ӽ��� ���� �����ڿ��Ը� �����Ǿ�� ���� �ǹ�
	// .��, OverlappingWeapon �Ӽ��� ABlasterCharacter�� ������ Ŭ���̾�Ʈ���Ը� ������ ���Դϴ�.
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	//ServerSetPlayerName(LocalPlayerName);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &ABlasterCharacter::AimButtonReleased);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlasterCharacter::LookUp);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}
}


void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
		// �������ͷ� ȸ�� ��Ʈ������ ���� �� �ִµ� �� ȸ�� ��Ʈ������ �� ������ ����ִ�.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
		// �������ͷ� ȸ�� ��Ʈ������ ���� �� �ִµ� �� ȸ�� ��Ʈ������ �� ������ ����ִ�.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	// ���� ������ ������ �ؾ� �Ѵ�
	// ������ ������ ����
	// HasAuthority = ������ ȣ�� ����
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			// �ٸ� Ŭ��� ������ ���� ������ ���� ������ �� �� ����
			// �׷��� RPC�� �̿��Ͽ� ����
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// ����ִ� ���Ⱑ �־�ߵ�.
	if (Combat && Combat->EquippedWeapon == nullptr)
		return;

	// ĳ���Ͱ� �������� �������� ����Ǿ����
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;

	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// ������ ���ְ� ������ ���Ҷ�
	if (Speed == 0.f && !bIsInAir)
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	// �޸��ų� �۶�
	if (Speed > 0.f || bIsInAir)
	{
		// FRotator�� �����ϴ� ������ ��Ÿ�� �������̱� ����
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	// �÷��̾� AimOffset���� Pitch�� 0���� �۾����� 360���� ����ȴ�.(�������?)
	// �׷��� 360 ���� 270������ 0���� -90���� �����ϴ°��̴�.
	// �׷��� ��ġ�� ������ -90 ~ 90�� �Ǿ� �ڿ������� AimOffset�� ������
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map ptich from [270, 360] to [-90, 0]
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		// �ش� ���� ����Ǿ� ��Ʈ��ũ�� ���� ���۵ǰ� 0~360 ������ ������ ���� ������ �� ����Ǿ� �ش� ����(-90 ~ 0)�� ���Ե�
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// �÷��̾ Weapon�� Overlap�ؼ� �浹�Ǹ� OverlappingWeapon�� nullptr�� �ƴϰ� �ǰ�
	// ���� �ٲ�� �Ǹ鼭 ������ �ǰ� �����ɶ� �ش� �Լ��� ���ͼ� PickupWidget�� �����ְ� ����
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	// �÷��̾ Weapon�̶� EndOverlap �Ǹ� SetOverlappingWeapon(nullptr);�� ȣ��Ǿ�
	// OverlappingWeapon�� nullptr�� �Ǿ� ���� �ٲ�鼭 �ش� �Լ��� �����Ե�����
	// LastWeapon���� ������ �߻��ϱ� ���� OverlappingWeapon�� ����ֱ� ������(nullptr �ٲ�� ���� ��)
	// PickWidget�� �� ���̰� �� �� �ִ°��̴�.
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		// ������� ȸ���ϸ� ȸ���� ���߰� �ٽ� ���ӷ����̼ǰ� ����
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// ȣ��Ʈ Ŭ���̾�Ʈ�� ���
	if (IsLocallyControlled())
	{
		// ���� �̹� OverlappingWeapon�� �ִٸ�, �� ������ PickupWidget�� ����
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}

		// OverlappingWeapon�� ���ο� ����� ����
		OverlappingWeapon = Weapon;

		// ���� ���� ������ OverlappingWeapon�� �ִٸ�, �� ������ PickupWidget�� ������
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
	// ȣ��Ʈ Ŭ���̾�Ʈ�� �ƴ� ���, OverlappingWeapon�� ����
	else
	{
		OverlappingWeapon = Weapon;
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	// Combat�� ��ȿ�ϰ� ��ȿ�ϴٸ� EquipWeapon�� ��ȿ����
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr)
		return nullptr; 

	return Combat->EquippedWeapon;
}

void ABlasterCharacter::ClientSetName_Implementation(const FString& Name)
{
	// �÷��̾� �̸� ����
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController != nullptr)
	{
		PlayerController->PlayerState->SetPlayerName(Name);
	}
}

void ABlasterCharacter::ServerSetPlayerName_Implementation(const FString& PlayerName)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		// ������ �÷��̾� �̸� ����
		PlayerController->PlayerState->SetPlayerName(PlayerName);
		// ClientSetName�Լ��� ȣ���� Ŭ���̾�Ʈ���� �÷��̾� �̸��� ����
		ClientSetName(PlayerName);
	}

}
