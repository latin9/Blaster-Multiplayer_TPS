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
	CameraBoom->TargetArmLength = 600.f;	// 카메라붐 길이
	CameraBoom->bUsePawnControlRotation = true;	// 마우스를 입력할 때 컨트롤러와 함께 카메라 회전 가능

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

	// 어떤 클라이언트가 이 변수를 복제할지 제어할 수 있다.
	// 속성이 오직 소유자에게만 복제되어야 함을 의미
	// .즉, OverlappingWeapon 속성은 ABlasterCharacter를 소유한 클라이언트에게만 복제될 것입니다.
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
		// 로테이터로 회전 메트릭스를 만들 수 있는데 그 회전 메트릭스에 축 정보가 담겨있다.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
		// 로테이터로 회전 메트릭스를 만들 수 있는데 그 회전 메트릭스에 축 정보가 담겨있다.
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
	// 무기 장착은 서버가 해야 한다
	// 서버에 권한이 있음
	// HasAuthority = 서버만 호출 가능
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			// 다른 클라는 권한이 없기 때문에 무기 장착을 할 수 없다
			// 그래서 RPC를 이용하여 장착
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// 플레이어가 Weapon에 Overlap해서 충돌되면 OverlappingWeapon이 nullptr이 아니게 되고
	// 값이 바뀌게 되면서 복제가 되고 복제될때 해당 함수에 들어와서 PickupWidget을 보여주게 만듬
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	// 플레이어가 Weapon이랑 EndOverlap 되면 SetOverlappingWeapon(nullptr);이 호출되어
	// OverlappingWeapon은 nullptr이 되어 값이 바뀌면서 해당 함수로 들어오게되지만
	// LastWeapon에는 복제가 발생하기 전의 OverlappingWeapon이 담겨있기 때문에(nullptr 바뀌기 전의 값)
	// PickWidget을 안 보이게 할 수 있는것이다.
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
	// 호스트 클라이언트일 경우
	if (IsLocallyControlled())
	{
		// 만약 이미 OverlappingWeapon이 있다면, 그 무기의 PickupWidget를 숨김
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}

		// OverlappingWeapon을 새로운 무기로 설정
		OverlappingWeapon = Weapon;

		// 만약 새로 설정된 OverlappingWeapon이 있다면, 그 무기의 PickupWidget를 보여줌
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
	// 호스트 클라이언트가 아닌 경우, OverlappingWeapon만 설정
	else
	{
		OverlappingWeapon = Weapon;
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	// Combat이 유효하고 유효하다면 EquipWeapon이 유효한지
	return (Combat && Combat->EquippedWeapon);
}

void ABlasterCharacter::ClientSetName_Implementation(const FString& Name)
{
	// 플레이어 이름 설정
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
		// 서버에 플레이어 이름 설정
		PlayerController->PlayerState->SetPlayerName(PlayerName);
		// ClientSetName함수를 호출해 클라이언트에서 플레이어 이름을 설정
		ClientSetName(PlayerName);
	}

}
