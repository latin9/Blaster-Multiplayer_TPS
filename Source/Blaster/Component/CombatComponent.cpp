// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			// ĳ������ ī�޶� ������� FOV��
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;

	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	// �������� �÷��̾� �̵��ӵ� ������ �Ѱ�����Ѵ�.
	// �׷��� �������� ��� Ŭ���̾�Ʈ�� ������ ��������
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (bCanFire)
	{
		bCanFire = false;

		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}

		// ���� �߻��� �� FireTimer�� �����Ѵ�.
		// �׷��� ���� �� ���� ������ ��ŭ ����
		StartFireTimer();
	}
}

/*
FVector_NetQuantize
1. ����ȭ: ������ �� ���� ��Ҹ� ���� ũ���� ������ ����ȭ�մϴ�. 
�̸� ����, ��Ʈ��ũ�� ���� ���۵Ǵ� �������� ũ�Ⱑ �پ��� �̷��� ����ȭ�� 
Ư�� ����� �����̳� ��Ը� ��Ƽ�÷��̾� ���ӿ��� �߿�

2. �ս��ִ� ����: ����ȭ�� �Ϲ������� �ս��� �ִ� ������ ���
���� ���� FVector�� FVector_NetQuantize ���̿� �ణ�� ���̰� �߻��� �� �ִ�.
�� ���̴� ��κ��� ��� ���� ���� ���� ������ �۽��ϴ�.

3. ��Ʈ��ũ ����ȭ: FVector_NetQuantize�� ��Ʈ��ũ ������ ���� Ư���� ����Ǿ����Ƿ�, 
�ٸ� ���� Ÿ�Կ� ���� ��Ʈ��ũ���� �� ȿ�������� ���۵� �� �ֽ��ϴ�.

4. ��Ʈ�� ����: �𸮾� ������ ��Ʈ��ũ �ý����� �� Ÿ���� ���Ϳ� ���� ������ ��Ʈ�εǾ� �����Ƿ�, 
�����ڴ� ������ ������ �ڵ� ���̵� �̷��� ����ȭ�� ���� �̿��� �� �ִ�.

FVector_NetQuantize�� ��ġ, �ӵ� ��� ���� ���� ������ ��Ʈ��ũ�� ���� ���� ���۵Ǵ� ���� ���� �����ϰ� ���� �� ������, �̷��� ���� ������ �� �߻��ϴ� ������ ũ���� ���ϸ� ���� �� �ֽ��ϴ�.
*/
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// �������� ��Ƽ�ɽ�Ʈ RPC�� ȣ���ϸ� �������� ����ǰ� ��� Ŭ���̾�Ʈ���� ���� ����ȴ�.
	MulticastFire(TraceHitTarget);

	// ���� MulticastFire �ȿ� �ִ� �ڵ带 ���⿡ �ۼ��ϰ� �����ϸ�
	// Ŭ�󿡼� ���� ��� �ѱ� �ִϸ��̼� ���õ� �κ��� ���������ۿ� �� ���δ�
	// �׷��� ��Ƽ�ɽ�Ʈ RPC�� Ȱ���Ͽ� ���� �����ϰ� �ϴ°�.
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)
		return;

	if (Character)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// ȭ������� 2D��ǥ CrosshairLocation�� 3D ���� ������ �� ��ġ�� �������� ��ȯ
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
			//DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End,
			ECollisionChannel::ECC_Visibility);

		// Character�� Interface�� ��ӹ��� ���±� ������
		// TraceHit�� ��ü�� Character��� Crosshair�� ���������� ����
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			// Trace�� ���� �浹�ߴٸ� White�� ����
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
		// Ʈ���̽� ���� �浹���� �ʾ������� ������ �� �浹��ġ�� End�� ����
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
		return;

	// ��Ʈ�ѷ� ����
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	// HUD ����
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;

		if (HUD)
		{
			// ������ ���Ⱑ �ִٸ� HUD�����ϰ� ������ ���⿡ ���� ũ�ν� ��� ����
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				// ������ ���Ⱑ ���ٸ� HUD�� ũ�ν���� �ؽ�ó nullptr
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}
			// ũ�ν���� ���ڼ� Ȯ�� ���
			// �÷��̾� �ִ�ӵ�
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			// �÷��̾� ���� ����
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			// �÷��̾� ���� �ӵ� : z�� 0
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			// �÷��̾� ������ ���� ���ε� ���� ����
			// 0 ~ 1�� ���� �����Ե�
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			// ���� �߻��Ҷ� CrosshairShootingFactor�� 0.2f�� �����س����� �˾Ƽ� ���⼭ 0���� �����Ǹ� �پ��
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 30.f);

			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + 
				CrosshairInAirFactor - 
				CrosshairAimFactor + 
				CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
		return;

	if (bAiming)
	{
		// ���� �ϴ¼��� ���⸶���� �� �ӵ��� ���� �ش� �� �ӵ���ŭ ���� �ȴ�
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		// ���� ������ ������ FOV������ �ǵ�����
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	// ���������� ī�޶� FOV ����
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
		return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
		return;

	bCanFire = true;
	// �ڵ�ȭ�� ���⿩������ ����
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
}


void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
		return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	// ������ �����ڸ� �����������
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

