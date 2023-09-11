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
#include "Sound/SoundCue.h"
#include "../Character/BlasterAnimInstance.h"
#include "../Weapon/Projectile.h"
#include "../Weapon/Shotgun.h"

UCombatComponent::UCombatComponent()	:
	bCanFire(true)
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
		// ����������
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
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

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	// �Ű������� ���� ���� Ÿ���� �ִٸ� �ش� ����Ÿ���� �޴��ϰ� �ִ� �Ѿ��� ������ ������Ų��.
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);

		// ���� ����ִ� ������ ź���� ��� HUD�� ������Ʈ �ؾ��Ѵ�.
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsAmmoEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;

	bAiming = bIsAiming;

	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	// ���� �÷��̾��϶��� �������� ������ ���� �ִϸ��̼� ����
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
	if (Character->IsLocallyControlled())
	{
		bAimButtonPressed = bIsAiming;
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
		// ���Ⱑ �ùķ��̼� �������� Ȱ��ȭ �Ǿ��������� ������ �ȵȴ� �׷��� ������
		// OnRep���� �ٷ� ������ ���� ���־���Ѵ�.
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		// Ŭ�󿡼��� ���� ���带 �������־�� ����, Ŭ�� ���� �鸰��.
		PlayEquipWeaponSound(EquippedWeapon);
		EquippedWeapon->SetHUDAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);

		// Ŭ�󿡼��� ���� ���带 �������־�� ����, Ŭ�� ���� �鸰��.
		PlayEquipWeaponSound(SecondaryWeapon);
	}
}
void UCombatComponent::FinishSwap()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	if(Character)
		Character->SetSwappingFinished(true);
	
}


void UCombatComponent::FinishSwapAttachWeapons()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	// ������ �����ٸ� ��Ÿ�� ������ ShotgunEnd�� �̵��ؾ���
	// �̰��� �ؾ��ϴ� ������ 2���� �ִ� ���¿��� ������������ ���� �Ѿ˼�
	// 2�߸� �����ؾ��ϴµ� �ִϸ��̼��� �� 4�� �����ϴ� �ִϸ��̼��̱� ����
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		// �ش� �Լ��� ��Ÿ�� ���� �̸��� �־��ָ� �ش� ��ġ�� �������� �ǳʵھ �����Ѵ�.
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	// HitTarget�� ƽ���� IsLocallyController()�ϰ�쿡�� ���ϰ� �ֱ⶧����
	// ���⼭�� ���������� �����÷��̾��϶��� �����ϰ� ServerRPC�� �����Ѵ�.
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();

		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		// ���� ������� �����°� = Instigator
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
				);
		}
	}
}


void UCombatComponent::Fire()
{
	if (CanFire() && EquippedWeapon)
	{
		bCanFire = false;

		CrosshairShootingFactor = 0.75f;

		switch (EquippedWeapon->FireType)
		{
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_HitScan:
			FireHitScanWeapon();
			break;
		case EFireType::EFT_Shotgun:
			FireShotgun();
			break;
		}

		// ���� �߻��� �� FireTimer�� �����Ѵ�.
		// �׷��� ���� �� ���� ������ ��ŭ ����
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		// ������ �ƴҶ� �����ؾ��Ѵ� ������ �̷��� �� �ϸ� ������ �Ѿ˵� ���� �߻��ϰԵ�
		if (!Character->HasAuthority())
			LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())
			LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireShotgun()
{
	if (EquippedWeapon)
	{
		AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
		if (Shotgun && Character)
		{
			TArray<FVector_NetQuantize> HitTargets;
			Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
			if (!Character->HasAuthority())
				ShotgunLocalFire(HitTargets);
			ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
		}
	}

}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)
		return;

	// �������϶��� Fire�����ϸ� �ȵȴ�
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}


void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);

	if (Shotgun == nullptr || Character == nullptr)
		return;

	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;	 // Combat���� ����
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
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	// �������� ��Ƽ�ɽ�Ʈ RPC�� ȣ���ϸ� �������� ����ǰ� ��� Ŭ���̾�Ʈ���� ���� ����ȴ�.
	MulticastFire(TraceHitTarget);

	// ���� MulticastFire �ȿ� �ִ� �ڵ带 ���⿡ �ۼ��ϰ� �����ϸ�
	// Ŭ�󿡼� ���� ��� �ѱ� �ִϸ��̼� ���õ� �κ��� ���������ۿ� �� ���δ�
	// �׷��� ��Ƽ�ɽ�Ʈ RPC�� Ȱ���Ͽ� ���� �����ϰ� �ϴ°�.
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}

	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// ���ÿ����� �̹� �ش� �۾��� ����⶧���� �Ʒ��� �����̶�� ����
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())
		return;

	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}

	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{// ���ÿ����� �̹� �ش� �۾��� ����⶧���� �Ʒ��� �����̶�� ����
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())
		return;

	ShotgunLocalFire(TraceHitTargets);
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
	//ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr)
		return false;
	// ������ ���ε��� �� �� �ְ� �ϱ�����
	if (!EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		return true;
	// ���� ���ε�� �к긯 ����
	if (bLocallyReloading)
		return false;

	// ������ ź�ళ���� 0�� �ƴϰ� bCanFire�� false���
	return !EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// �Ʒ��� ���� ��Ȳ�� Ʈ���϶��� JumpToShotgunEnd�� �����Ѵ�
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// ���� Ÿ�Ժ� �Ѿ� ���� ����
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	// �����ִ� ź���� ������ ���� Ŭ���̾�Ʈ������ ����ϱ� ����(���ε�) 
	// �̷��� �ؾ� �۽ŵǴ� �����͸� �ּ�ȭ�ϸ鼭 ���� ����� �ִ�.
	// ������ CarriedAmmo�� �ٸ� Ŭ���̾�Ʈ���� �������� �ʰ� �����ϴ� Ŭ���̾�Ʈ�� �����Ѵ�.
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}


// ���� ������
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
		return;

	// ECS_Unoccupied�����϶��� ���⸦ ������ �� �ִ�
	// �� ���������̰ų�, ����ź�� ������ �������� �Ұ����ϴ�.
	if (CombatState != ECombatState::ECS_Unoccupied)
		return;

	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}

	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::SwapWeapons()
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr)
		return;

	Character->PlaySwapMontage();
	Character->SetSwappingFinished(false);
	CombatState = ECombatState::ECS_SwappingWeapons;
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	// ������ �����ڸ� �����������
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	// �������� ȣ������� CarriedAmmo�� ���� ������ ������� ������ 
	// CarriedAmmo�� �����ϴ� ��� Ŭ���̾�Ʈ�� HUD��
	// Controller->SetHUDCarriedAmmo(CarriedAmmo);�� ���� ������Ʈ �� �� �ִ�.
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(WeaponToEquip);
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(WeaponToEquip);
	SecondaryWeapon->SetOwner(Character);
	PlayEquipWeaponSound(WeaponToEquip);
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		// Ŭ���̾�Ʈ�� ���ð��� ��Ȯ�ؾ��Ѵ�.
		bAiming = bAimButtonPressed;
	}
}


void UCombatComponent::Reload()
{
	// Ŭ���̾�Ʈ�� �ִ� ��� ��� Ŭ���̾�Ʈ���� �ٽ� ���ε� �ִϸ��̼��� ����� �ð����� �˸��� ����
	// ������ �ٽ� �ε��� �� �ִ��� Ȯ���ϵ��� RPC�� ������ ������ �Ѵ�.
	// Ammo�� 0�� �̻��̰�, Cobat���°� ECS_Unoccupied�϶��� ����
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsAmmoFull() && !bLocallyReloading)
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
		//UE_LOG(LogTemp, Error, TEXT("Play Reload"));
	}

}

void UCombatComponent::FinishReloading()
{
	if (!Character->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("FinishReloading"));
	}

	LocalFinishReloading();
	//ServerFinishReloading();
}

void UCombatComponent::ServerFinishReloading_Implementation()
{
	MulticastFinishReloading();
}

void UCombatComponent::MulticastFinishReloading_Implementation()
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())
		return;

	LocalFinishReloading();
}

void UCombatComponent::LocalFinishReloading()
{
	if (Character == nullptr)
		return;

	bLocallyReloading = false;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		// �Ѿ� ������Ʈ
		UpdateAmmoValues();
	}
	
	if (bFireButtonPressed)
	{
		Fire();
	}
}

//void UCombatComponent::ClientUpdateCarriedAmmo_Implementation(int32 ServerCarriedAmmo)
//{
//	if (Character->HasAuthority())
//		return;
//
//	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
//
//	CarriedAmmo = ServerCarriedAmmo;
//
//	if (Controller)
//	{
//		Controller->SetHUDCarriedAmmo(CarriedAmmo);
//	}
//}


void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;

	// CombatState���¸� �����ϸ� OnRep�� ����ȴ�
	CombatState = ECombatState::ECS_Reloading;
	// ��, ���������� �ش� �Լ��� ȣ���ϰ� Ŭ�󿡼��� �����ϰ� ȣ���������
	if (!Character->IsLocallyControlled())
	{
		HandleReload();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// CobmatState���°� ����Ǿ��µ� FireButton�� Ŭ���ϰ� �ִٸ� �߻�
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		// Ŭ�󿡼��� �ش� �Լ��� ȣ��
		if (Character && !Character->IsLocallyControlled())
			HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// �ٸ� Ŭ���̾�Ʈ������ ����ǵ���
		// �����÷��̾�� �̹� ThrowGrenade ��Ÿ�ָ� �������̱� ������ �ƴҶ��� ����
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		// �ٸ� Ŭ���̾�Ʈ������ ����ǵ���
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlaySwapMontage();
		}
		break;
	case ECombatState::ECS_MAX:
		break;
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;

	// �����ؾ��� �Ѿ��� ����
	int32 ReloadAmount = AmountToReload();

	// ���� ����ִ� �Ѿ��� ������ ����
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// ������ ����ִ� �Ѿ��� �������� �����ؾ��� �Ѿ��� ������ ���ش�
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		// ����ִ� ź�� ����
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	/*if (Character->HasAuthority())
	{
		ClientUpdateCarriedAmmo(CarriedAmmo);
	}*/
	// ������ �Ѿ� ����
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// ������ ����ִ� �Ѿ��� �������� �����ؾ��� �Ѿ��� ������ ���ش�
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		// ����ִ� ź�� ����
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// ������ �Ѿ� ����
	EquippedWeapon->AddAmmo(1);
	// true�� �ϸ� ������ �߰��� �ٽ� �� �� �ִ�.
	bCanFire = true;
	// �Ѿ� �������� ���⿡ ����ִ� �Ѿ��� �ִ�ų�
	// �����ִ� �Ѿ��� 0�̶��
	if (EquippedWeapon->IsAmmoFull() || CarriedAmmo == 0)
	{
		// ������ �����ٸ� ��Ÿ�� ������ ShotgunEnd�� �̵��ؾ���
		// �̰��� �ؾ��ϴ� ������ 2���� �ִ� ���¿��� ������������ ���� �Ѿ˼�
		// 2�߸� �����ؾ��ϴµ� �ִϸ��̼��� �� 4�� �����ϴ� �ִϸ��̼��̱� ����
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	// ��Ʈ�ѷ� ����
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

void UCombatComponent::HandleReload()
{
	if (Character)
		Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr)
		return 0;
	// ���� ����ִ� ���� ������ �� �ִ� �Ѿ��� ����
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// ���� ����ִ� �Ѿ��� �� ����
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		// �ּҰ� (�� �Ѱ� �������� �ʱ� ���ؼ�)
		// ����ִ� �Ѿ��� 10�� ���� ���Ƶ� �����Ҽ� �ִ� �ִ��� �Ѿ˼��� 10�̸�
		// �Ѿ��� 10���� �����ؾ��Ѵ�.
		// �׸��� ����ִ� �Ѿ��� ������ �� �ִ� �ִ��� �Ѿ˼� 10������ �۴ٸ�
		// �״�� ����ִ� �Ѿ��� ������ŭ�� �����ϴ°��̴�.
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0)
		return;

	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr)
		return;

	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenad();
	}
	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenad_Implementation()
{
	if (Grenades == 0)
		return;
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}


void UCombatComponent::DropEquippedWeapon()
{
	// �̹� ���Ⱑ �ִٸ� ������ �ִ� ���⸦ ������.
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)
		return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr)
		return;

	bool bUsePistolSocket = Cast<AWeapon>(ActorToAttach)->GetWeaponType() == EWeaponType::EWT_Pistol;
	bool bUseSMGSocket = Cast<AWeapon>(ActorToAttach)->GetWeaponType() == EWeaponType::EWT_SMG;

	FName SocketName;
	if (bUsePistolSocket)
	{
		SocketName = FName("PistolSocket");
	}
	else if (bUseSMGSocket)
	{
		SocketName = FName("SMGSocket");
	}
	else
	{
		SocketName = FName("LeftHandSocket");
	}
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}

}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)
		return;

	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));

	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr)
		return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	// �ڵ�����
	// ó�� ���� �Ծ����� ������ �Ѿ��� ������� ��� ����
	if (EquippedWeapon && EquippedWeapon->IsAmmoEmpty())
	{
		Reload();
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}
