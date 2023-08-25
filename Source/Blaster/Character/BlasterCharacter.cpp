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
#include "../Component/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "../Blaster.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "../GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "../Weapon/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	// 이걸 설정해줘야 웅크리기가 가능해진다.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// 캡슐충돌체와 스켈레탈 메쉬가 카메라와 충돌하지 않는다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// 콜리전 프로파일을 SkeletalMesh 프로파일로 설정
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.0, 0.0, 850.0);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	// 네트워크 업데이트 빈도 기본 ~ 최소
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LocalPlayerName = FString(TEXT("Player"));
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	// 서버에서만 데미지 줘야함
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);

	HideCameraIfCharacterClose();
	PollInit();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction(TEXT("ThrowGrenade"), IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlasterCharacter::LookUp);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 어떤 클라이언트가 이 변수를 복제할지 제어할 수 있다.
	// 속성이 오직 소유자에게만 복제되어야 함을 의미
	// .즉, OverlappingWeapon 속성은 ABlasterCharacter를 소유한 클라이언트에게만 복제될 것입니다.
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimproxiesTurn();
	// 움직임을 복제했으니 0으로 설정
	TimeSinceLastMovementReplication = 0.f;
}
void ABlasterCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	// 게임 모드는 서버에만 존재하고 서버에서 Elim을 실행하기 때문에 여긴 서버에 해당
	MulticastElim();
	// 부활 요청
	GetWorldTimerManager().SetTimer(ElimTimer,
		this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	// Dissolve를 적용하기위해 메테리얼을 변경
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		// 파라미터 설정
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 500.f);
	}
	StartDissolve();

	// 캐릭터 무브먼트 Disable
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	// 입력 Disable
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false); 
	}
	// Disalbe Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn ElimBot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
	if (IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon
		&& Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::Destroyed()
{	
	Super::Destroyed();

	if (ElimBotComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ElimBotComponent is not null"));
		UE_LOG(LogTemp, Warning, TEXT("ElimBotComponent Start DestroyComponent"));
		ElimBotComponent->DestroyComponent();
	}
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));

	// 죽었을때 무기를 제거하는 부분은 Cooldown이후 GameReset할때 처리하는 부분이라서
	// InProgress 즉 게임이 진행중에는 죽어도 무기가 사라지면 안된다 그걸 방지하기 위한 bool값
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}


void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeed(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		// FireWeaponMontage에 지정해놓은 몽타주 섹션 이름을 얻어온다.
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		// 해당 함수에 몽타주 섹션 이름을 넣어주면 해당 위치의 섹션으로 건너뒤어서 실행한다.
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		// 장전 애니메이션은 라이플 로켓 둘다 같다.
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SMG:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}


void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		// FireWeaponMontage에 지정해놓은 몽타주 섹션 이름을 얻어온다.
		FName SectionName("FromFront");

		// 해당 함수에 몽타주 섹션 이름을 넣어주면 해당 위치의 섹션으로 건너뒤어서 실행한다.
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, 
	AController* InstigatorController, AActor* DamageCauser)
{
	// 이미 죽어있는경우 리턴
	if (bElimmed)
		return;

	// Health는 복제중 그래서 값이 바뀌는 순간 OnRep_Health()함수가 실행됨
	Health = FMath::Clamp(FMath::CeilToInt(Health - Damage), 0, FMath::CeilToInt(MaxHealth));
	// HUD 업데이트하고 몽타주 실행
	UpdateHUDHealth();
	PlayHitReactMontage();

	// 죽었을때
	if (Health == 0.f)
	{
		// 현재 해당 ReceiveDamage함수는 서버에서 실행되고 있기 때문에 게임모드를 얻어올 수 있다.
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			// 공격한 사람의 플레이어 컨트롤러
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}


void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay)
		return;

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
	if (bDisableGameplay)
		return;

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
	if (bDisableGameplay)
		return;

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

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}


void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay)
		return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay)
		return;

	// R키를 누름과 동시에 모든 시스템(서버, 클라이언트)에서 해당 함수가 호출이 된다.
	// 따라서 서버에서만 리로드 하는것이 적절한지 확인하기 위해 유효성 검사를 수행한다.
	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0;

	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// 들고있는 무기가 있어야됨.
	if (Combat && Combat->EquippedWeapon == nullptr)
		return;

	// 캐릭터가 움직이지 않을때만 실행되어야함
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// 가만히 서있고 점프를 안할때
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
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
	// 달리거나 뛸때
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		// FRotator로 저장하는 이유는 델타를 얻을것이기 때문
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{

	// 플레이어 AimOffset에서 Pitch가 0보다 작아지면 360으로 변경된다.(압축과정?)
	// 그래서 360 부터 270까지를 0부터 -90으로 변경하는것이다.
	// 그래야 피치의 구간이 -90 ~ 90이 되어 자연스러운 AimOffset이 가능함
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map ptich from [270, 360] to [-90, 0]
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		// 해당 값은 압축되어 네트워크를 통해 전송되고 0~360 사이의 범위로 압축 해제된 후 변경되어 해당 범위(-90 ~ 0)를 갖게됨
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimproxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	// 시뮬레이션 프록시가 아닌경우
	// 서버나 클라이언트에서 로컬로 제어될경우
	bRotateRootBone = false;
	float Speed = CalculateSpeed();

	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}


	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	// 프록시의 Yaw의 절대값이 TurnThreshold보다 크다면
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay)
		return;

	// 스페이스바를 눌렀는데 앉아있는 상태라면 일어서고
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		// 앉아있는 상태가 아니라면 점프를 진행한다.
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay)
		return;

	// 컴뱃에 좌클릭 클릭했다 전달
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
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
		// 어느정도 회전하면 회전을 멈추고 다시 에임로테이션값 지정
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}
void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())
		return;

	// 플레이어와 카메라의 거리가 CameraThreshold보다 작아진다면 카메라와 무기를 안 보이도록 설정
	// 그래야 벽근처에 숨어있어도 이상하게 보이지 않는다.
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	// HUD 업데이트하고 몽타주 실행
	UpdateHUDHealth();

	// 갱신된 HP가 갱신되기 전의 HP보다 작다면
	// 데미지를 입은 상황이므로 HitReactMontage를 실행해야한다.
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}

	if (Health == 0.f)
	{
		MulticastElim();
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

		if (BlasterPlayerState)
		{
			// 0을 넣어 갱신만 해준다.
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;

		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	// 타임라인이 재생되면 매 프레임마다 이 함수를 호출하여 DynamicDissolveMaterialInstance를 갖고오고
	// 유효할 경우 디졸브 매개변수를 타임라인 곡선에서 가져온 값으로 설정한다.
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// 콜백함수 등록
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		// 타임라인을 설정해서 디졸브 곡선을 사용하게 하고 그 곡선과 DissolveTrack에 등록되어있는 콜백을 연결한다.
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		// 플레이
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
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



bool ABlasterCharacter::IsWeaponEquipped()
{
	// Combat이 유효하고 유효하다면 EquipWeapon이 유효한지
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

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr)
		return FVector();

	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr)
		return ECombatState::ECS_MAX;

	return Combat->CombatState;
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
