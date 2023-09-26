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
#include "../GameMode/CapturePointGameMode.h"
#include "../GameMode/TeamsGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "../PlayerState/BlasterPlayerState.h"
#include "../Weapon/WeaponTypes.h"
#include "Components/BoxComponent.h"
#include "../Component/LagCompensationComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "../GameState/BlasterGameState.h"
#include "../PlayerStart/TeamPlayerStart.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	// �̰� ��������� ��ũ���Ⱑ ����������.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// ĸ���浹ü�� ���̷�Ż �޽��� ī�޶�� �浹���� �ʴ´�.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// �ݸ��� ���������� SkeletalMesh �������Ϸ� ����
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	// ue5.1������ Ŭ���̾�Ʈ ĳ���Ͱ� ���� �信 ���� �� ĳ������ ���� �̵��� ������Ʈ���� �ʾ� �ѱ� ��ġ�� �ùٸ��� �ʴٴ� ���� �߰��߽��ϴ�.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 0.0, 850.0);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	// ��Ʈ��ũ ������Ʈ �� �⺻ ~ �ּ�
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Server-side rewind ������ �ǰ���
	// FName�� ��ҹ��� �������� �ʱ� ������ ����?������ �°Ծ��� �ȴ�.
	Head = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadBoxComponent"));
	Head->SetupAttachment(GetMesh(), FName("Head"));
	HitCollisionBoxes.Add(FName("Head"), Head);

	Pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("PelvisBoxComponent"));
	Pelvis->SetupAttachment(GetMesh(), FName("Pelvis"));
	HitCollisionBoxes.Add(FName("Pelvis"), Pelvis);

	Spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine02"));
	Spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), Spine_02);

	Spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine03"));
	Spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), Spine_03);

	UpperArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("UpperArm_L"));
	UpperArm_L->SetupAttachment(GetMesh(), FName("UpperArm_L"));
	HitCollisionBoxes.Add(FName("UpperArm_L"), UpperArm_L);

	UpperArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("UpperArm_R"));
	UpperArm_R->SetupAttachment(GetMesh(), FName("UpperArm_R"));
	HitCollisionBoxes.Add(FName("UpperArm_R"), UpperArm_R);

	LowerArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("LowerArm_L"));
	LowerArm_L->SetupAttachment(GetMesh(), FName("LowerArm_L"));
	HitCollisionBoxes.Add(FName("LowerArm_L"), LowerArm_L);

	LowerArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("LowerArm_R"));
	LowerArm_R->SetupAttachment(GetMesh(), FName("LowerArm_R"));
	HitCollisionBoxes.Add(FName("LowerArm_R"), LowerArm_R);

	Hand_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Hand_L"));
	Hand_L->SetupAttachment(GetMesh(), FName("Hand_L"));
	HitCollisionBoxes.Add(FName("Hand_L"), Hand_L);

	Hand_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Hand_R"));
	Hand_R->SetupAttachment(GetMesh(), FName("Hand_R"));
	HitCollisionBoxes.Add(FName("Hand_R"), Hand_R);

	BackpackBottom = CreateDefaultSubobject<UBoxComponent>(TEXT("BackpackBottom"));
	BackpackBottom->SetupAttachment(GetMesh(), FName("BackpackBottom"));
	HitCollisionBoxes.Add(FName("BackpackBottom"), BackpackBottom);

	BackpackTop = CreateDefaultSubobject<UBoxComponent>(TEXT("BackpackTop"));
	BackpackTop->SetupAttachment(GetMesh(), FName("BackpackTop"));
	HitCollisionBoxes.Add(FName("BackpackTop"), BackpackTop);

	Thigh_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Thigh_L"));
	Thigh_L->SetupAttachment(GetMesh(), FName("Thigh_L"));
	HitCollisionBoxes.Add(FName("Thigh_L"), Thigh_L);

	Thigh_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Thigh_R"));
	Thigh_R->SetupAttachment(GetMesh(), FName("Thigh_R"));
	HitCollisionBoxes.Add(FName("Thigh_R"), Thigh_R);

	Calf_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Calf_L"));
	Calf_L->SetupAttachment(GetMesh(), FName("Calf_L"));
	HitCollisionBoxes.Add(FName("Calf_L"), Calf_L);

	Calf_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Calf_R"));
	Calf_R->SetupAttachment(GetMesh(), FName("Calf_R"));
	HitCollisionBoxes.Add(FName("Calf_R"), Calf_R);

	Foot_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_L"));
	Foot_L->SetupAttachment(GetMesh(), FName("Foot_L"));
	HitCollisionBoxes.Add(FName("Foot_L"), Foot_L);

	Foot_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_R"));
	Foot_R->SetupAttachment(GetMesh(), FName("Foot_R"));
	HitCollisionBoxes.Add(FName("Foot_R"), Foot_R);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}


	LocalPlayerName = FString(TEXT("Player"));
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr)
		return;

	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasterCharacter::SetTeamColor(ETeam _Team)
{
	if (GetMesh() == nullptr || OriginalMaterial == nullptr)
		return;

	switch (_Team)
	{
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMatInstance;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMatInstance;
		break;
	case ETeam::ET_NoTeam:	// ���� �������� �⺻������ ���
		GetMesh()->SetMaterial(0, OriginalMaterial);
		DissolveMaterialInstance = BlueDissolveMatInstance;
		break;
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();

	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();

	// ���������� ������ �����
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


	PlayerInputComponent->BindAction(TEXT("SwitchToPlayerView"), IE_Pressed, this, &ABlasterCharacter::SwitchToPlayerView);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// � Ŭ���̾�Ʈ�� �� ������ �������� ������ �� �ִ�.
	// �Ӽ��� ���� �����ڿ��Ը� �����Ǿ�� ���� �ǹ�
	// .��, OverlappingWeapon �Ӽ��� ABlasterCharacter�� ������ Ŭ���̾�Ʈ���Ը� ������ ���Դϴ�.
	//DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimproxiesTurn();
	// �������� ���������� 0���� ����
	TimeSinceLastMovementReplication = 0.f;
}
void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	// ���� ���� �������� �����ϰ� �������� Elim�� �����ϱ� ������ ���� ������ �ش�
	MulticastElim(bPlayerLeftGame);
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	// Dissolve�� �����ϱ����� ���׸����� ����
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		// �Ķ���� ����
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 500.f);
	}
	StartDissolve();

	// ĳ���� �����Ʈ Disable
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	// �Է� Disable
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false); 
	}
	// Disalbe Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
	// ��Ȱ ��û
	GetWorldTimerManager().SetTimer(ElimTimer,
		this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::ElimTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}

	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}
void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
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
	//ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	// �׾����� ���⸦ �����ϴ� �κ��� Cooldown���� GameReset�Ҷ� ó���ϴ� �κ��̶�
	// InProgress �� ������ �����߿��� �׾ ���Ⱑ ������� �ȵȴ� �װ� �����ϱ� ���� bool��
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
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (Combat)
	{
		Combat->UpdateHUDGrenades();
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
		// FireWeaponMontage�� �����س��� ��Ÿ�� ���� �̸��� ���´�.
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		// �ش� �Լ��� ��Ÿ�� ���� �̸��� �־��ָ� �ش� ��ġ�� �������� �ǳʵھ �����Ѵ�.
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
		// ���� �ִϸ��̼��� ������ ���� �Ѵ� ����.
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

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr || Combat->CombatState != ECombatState::ECS_Unoccupied)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		// FireWeaponMontage�� �����س��� ��Ÿ�� ���� �̸��� ���´�.
		FName SectionName("FromFront");

		// �ش� �Լ��� ��Ÿ�� ���� �̸��� �־��ָ� �ش� ��ġ�� �������� �ǳʵھ �����Ѵ�.
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::SwitchToPlayerView()
{
	//BlasterPlayerController->SwitchViewToOtherPlayer();
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, 
	AController* InstigatorController, AActor* DamageCauser)
{
	// ���� �ش� ReceiveDamage�Լ��� �������� ����ǰ� �ֱ� ������ ���Ӹ�带 ���� �� �ִ�.
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	// �̹� �׾��ִ°�� ����
	if (bElimmed || BlasterGameMode == nullptr)
		return;

	Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0, MaxShield);
			// ���差�� ���� ��������� ������ DamageToHealth �� 0�̵ȴ�
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0, Damage);
			Shield = 0.f;
		}
	}
	// Health�� ������ �׷��� ���� �ٲ�� ���� OnRep_Health()�Լ��� �����
	Health = FMath::Clamp(FMath::CeilToInt(Health - DamageToHealth), 0, FMath::CeilToInt(MaxHealth));

	// HUD ������Ʈ�ϰ� ��Ÿ�� ����
	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	// �׾�����
	if (Health == 0.f)
	{
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			// ������ ����� �÷��̾� ��Ʈ�ѷ�
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
		// �������ͷ� ȸ�� ��Ʈ������ ���� �� �ִµ� �� ȸ�� ��Ʈ������ �� ������ ����ִ�.
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
		// �������ͷ� ȸ�� ��Ʈ������ ���� �� �ִµ� �� ȸ�� ��Ʈ������ �� ������ ����ִ�.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value * MouseSensitivity);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value * MouseSensitivity);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay)
		return;

	// ���� ������ ������ �ؾ� �Ѵ�
	// ������ ������ ����
	// HasAuthority = ������ ȣ�� ����
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
		if (Combat->CombatState == ECombatState::ECS_Unoccupied)
			ServerEquipButtonPressed();
		
		// ���� ���� RPC���� �Լ����� �Ʒ��� �� �ڵ带 �����ϰ� �ֱ� ������
		// ������ �ƴҰ�쿡�� �����ؾ��Ѵ�.
		bool bSwap = Combat->ShouldSwapWeapons() && !HasAuthority() &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;
		if (bSwap)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if(Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}


void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay)
		return;
	if (Combat && Combat->bHoldingTheFlag)
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

	// RŰ�� ������ ���ÿ� ��� �ý���(����, Ŭ���̾�Ʈ)���� �ش� �Լ��� ȣ���� �ȴ�.
	// ���� ���������� ���ε� �ϴ°��� �������� Ȯ���ϱ� ���� ��ȿ�� �˻縦 �����Ѵ�.
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
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
	// ����ִ� ���Ⱑ �־�ߵ�.
	if (Combat && Combat->EquippedWeapon == nullptr)
		return;

	// ĳ���Ͱ� �������� �������� ����Ǿ����
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// ������ ���ְ� ������ ���Ҷ�
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
	// �޸��ų� �۶�
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		// FRotator�� �����ϴ� ������ ��Ÿ�� �������̱� ����
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{

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

void ABlasterCharacter::SimproxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
		return;

	// �ùķ��̼� ���Ͻð� �ƴѰ��
	// ������ Ŭ���̾�Ʈ���� ���÷� ����ɰ��
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

	// ���Ͻ��� Yaw�� ���밪�� TurnThreshold���� ũ�ٸ�
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

	if (Combat && Combat->bHoldingTheFlag)
		return;

	// �����̽��ٸ� �����µ� �ɾ��ִ� ���¶�� �Ͼ��
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		// �ɾ��ִ� ���°� �ƴ϶�� ������ �����Ѵ�.
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay)
		return;

	// �Ĺ ��Ŭ�� Ŭ���ߴ� ����
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay)
		return;

	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
			return;
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
		// ������� ȸ���ϸ� ȸ���� ���߰� �ٽ� ���ӷ����̼ǰ� ����
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

	// �÷��̾�� ī�޶��� �Ÿ��� CameraThreshold���� �۾����ٸ� ī�޶�� ���⸦ �� ���̵��� ����
	// �׷��� ����ó�� �����־ �̻��ϰ� ������ �ʴ´�.
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	// HUD ������Ʈ�ϰ� ��Ÿ�� ����
	UpdateHUDHealth();

	// ���ŵ� HP�� ���ŵǱ� ���� HP���� �۴ٸ�
	// �������� ���� ��Ȳ�̹Ƿ� HitReactMontage�� �����ؾ��Ѵ�.
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}

	if (Health == 0.f)
	{
		MulticastElim(false);
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	// HUD ������Ʈ�ϰ� ��Ÿ�� ����
	UpdateHUDShield();

	// ���ŵ� HP�� ���ŵǱ� ���� HP���� �۴ٸ�
	// �������� ���� ��Ȳ�̹Ƿ� HitReactMontage�� �����ؾ��Ѵ�.
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
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

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if (BlasterPlayerController && Combat && Combat->EquippedWeapon && bUpdateHUDAmmo)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		bUpdateHUDAmmo = false;
	}
	else
	{
		bUpdateHUDAmmo = true;
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	ATeamsGameMode* TeamsGameMode = GetWorld()->GetAuthGameMode<ATeamsGameMode>();

	UWorld* World = GetWorld();

	// ����� ���Ӹ�尡 ���� �ƴ϶�� BlasterLevel�̶�°��� �� �� �ִ�
	// �κ� TitleLevel�� BlasterGameMode�� ��� �� �ϱ⶧���� ���� ��
	if (BlasterGameMode && !TeamsGameMode && World && !bElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon =  World->SpawnActor<AWeapon>(DefaultWeaponClass);

		StartingWeapon->bDestroyWeapon = true;

		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}


void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

		if (BlasterPlayerState)
		{
			OnPlayerStateInitialized();

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
	if (bUpdateHUDAmmo)
	{
		UpdateHUDAmmo();
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (Combat && Combat->EquippedWeapon)
	{
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
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

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr)
		return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
		if (Combat->TheFlag)
		{
			Combat->TheFlag->Dropped();
		}
	}
}

void ABlasterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		// ��ġ�ص� ��� �÷��̾ŸƮ���͸� ���´�.
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			// �÷��̾� ��ŸƮ�� ���̶� �÷��̾��� ���̶� ���ٸ� TeamPlayerStarts�� �� ��ŸƮ ���͸� �����Ѵ�.
			if (TeamStart && TeamStart->GetTeam() == BlasterPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}

		// �ᱹ �迭�� ����� �κ��� ���� �� ��������Ʈ�� ����Ȱű� ������
		// �� �� ������ ���� ��ġ�� �ش� �÷��̾ �����ϴ°��̴�.
		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
		}
	}
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	// 0�� �־� ���Ÿ� ���ش�.
	/*BlasterPlayerState->AddToScore(0.f);
	BlasterPlayerState->AddToDefeats(0.f);*/
	SetTeamColor(BlasterPlayerState->GetTeam());
	SetSpawnPoint();
}

void ABlasterCharacter::ServerHandleWeaponSelection_Implementation(EMainWeapon_Type MainWeapon_Type, ESubWeapon_Type SubWeapon_Type)
{
	UWorld* World = GetWorld();

	// �⺻ ���� ����
	AWeapon* MainWeapon = World->SpawnActor<AWeapon>(MapMainWeapons[EMainWeapon_Type::EMW_AssaultRifle]);
	AWeapon* SubWeapon = World->SpawnActor<AWeapon>(MapSubWeapons[ESubWeapon_Type::ESW_Pistol]);

	switch (MainWeapon_Type)
	{
	case EMainWeapon_Type::EMW_AssaultRifle:
		MainWeapon = World->SpawnActor<AWeapon>(MapMainWeapons[EMainWeapon_Type::EMW_AssaultRifle]);
		break;
	case EMainWeapon_Type::EMW_SniperRifle:
		MainWeapon = World->SpawnActor<AWeapon>(MapMainWeapons[EMainWeapon_Type::EMW_SniperRifle]);
		break;
	case EMainWeapon_Type::EMW_RocketLauncher:
		MainWeapon = World->SpawnActor<AWeapon>(MapMainWeapons[EMainWeapon_Type::EMW_RocketLauncher]);
		break;
	case EMainWeapon_Type::EMW_Shotgun:
		MainWeapon = World->SpawnActor<AWeapon>(MapMainWeapons[EMainWeapon_Type::EMW_Shotgun]);
		break;
	}

	switch (SubWeapon_Type)
	{
	case ESubWeapon_Type::ESW_Pistol:
		SubWeapon = World->SpawnActor<AWeapon>(MapSubWeapons[ESubWeapon_Type::ESW_Pistol]);
		break;
	case ESubWeapon_Type::ESW_SMG:
		SubWeapon = World->SpawnActor<AWeapon>(MapSubWeapons[ESubWeapon_Type::ESW_SMG]);
		break;
	case ESubWeapon_Type::ESW_GrenadeLauncher:
		SubWeapon = World->SpawnActor<AWeapon>(MapSubWeapons[ESubWeapon_Type::ESW_GrenadeLauncher]);
		break;
	}
	MainWeapon->bDestroyWeapon = true;
	SubWeapon->bDestroyWeapon = true;

	if (Combat)
	{
		Combat->EquipWeapon(MainWeapon);
		Combat->EquipWeapon(SubWeapon);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	// Ÿ�Ӷ����� ����Ǹ� �� �����Ӹ��� �� �Լ��� ȣ���Ͽ� DynamicDissolveMaterialInstance�� �������
	// ��ȿ�� ��� ������ �Ű������� Ÿ�Ӷ��� ����� ������ ������ �����Ѵ�.
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// �ݹ��Լ� ���
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		// Ÿ�Ӷ����� �����ؼ� ������ ��� ����ϰ� �ϰ� �� ��� DissolveTrack�� ��ϵǾ��ִ� �ݹ��� �����Ѵ�.
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		// �÷���
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

UBoxComponent* ABlasterCharacter::GetHitColisionBoxFromFName(const FName& Name)
{
	if (HitCollisionBoxes[Name] != nullptr)
		return HitCollisionBoxes[Name];

	return nullptr;
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr)
		return false;

	return Combat->bHoldingTheFlag;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterPlayerState == nullptr)
		return ETeam::ET_NoTeam;

	return BlasterPlayerState->GetTeam();
}

void ABlasterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat == nullptr)
		return;

	Combat->bHoldingTheFlag = bHolding;
}

bool ABlasterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr)
		return false;

	return Combat->GetLocallyReloading();
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
