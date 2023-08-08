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
#include "../HUD/BlasterHUD.h"

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
	}

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHUDCrosshairs(DeltaTime);

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
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		ServerFire(HitResult.ImpactPoint);
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
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End,
			ECollisionChannel::ECC_Visibility);

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
			FHUDPackage HUDPackage;
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

			HUD->SetHUDPackage(HUDPackage);
		}
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

