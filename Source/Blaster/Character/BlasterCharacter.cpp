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

}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);

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
