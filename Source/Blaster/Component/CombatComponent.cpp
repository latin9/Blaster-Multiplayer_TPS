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
			// 캐릭터의 카메라가 사용중인 FOV값
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		// 서버에서만
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
	// 매개변수로 들어온 무기 타입이 있다면 해당 무기타입의 휴대하고 있는 총알의 개수를 증가시킨다.
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);

		// 현재 들고있는 무기의 탄약일 경우 HUD를 업데이트 해야한다.
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
	// 로컬 플레이어일때만 스나이퍼 스코프 위젯 애니메이션 실행
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
	// 서버에도 플레이어 이동속도 정보를 넘겨줘야한다.
	// 그래야 서버에서 모든 클라이언트에 정보를 제공해줌
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// 무기가 시뮬레이션 피직스가 활성화 되어있을때는 부착이 안된다 그렇기 때문에
		// OnRep에서 바로 변경을 먼저 해주어야한다.
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		// 클라에서도 장착 사운드를 실행해주어야 서버, 클라 전부 들린다.
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

		// 클라에서도 장착 사운드를 실행해주어야 서버, 클라 전부 들린다.
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
	// 장전이 끝났다면 몽타주 섹션을 ShotgunEnd로 이동해야함
	// 이것을 해야하는 이유는 2발이 있는 상태에서 장전했을때는 남은 총알수
	// 2발만 장전해야하는데 애니메이션은 총 4발 장전하는 애니메이션이기 때문
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		// 해당 함수에 몽타주 섹션 이름을 넣어주면 해당 위치의 섹션으로 건너뒤어서 실행한다.
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

	// HitTarget을 틱에서 IsLocallyController()일경우에만 구하고 있기때문에
	// 여기서도 마찬가지로 로컬플레이어일때만 실행하고 ServerRPC를 실행한다.
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
		// 누가 대미지를 입혔는가 = Instigator
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

		// 총을 발사한 뒤 FireTimer를 실행한다.
		// 그래야 총을 쏜 뒤의 딜레이 만큼 실행
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->GetUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		// 서버가 아닐때 실행해야한다 로컬은 이렇게 안 하면 서버는 총알두 발을 발사하게됨
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

	// 장전중일때는 Fire실행하면 안된다
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
		CombatState = ECombatState::ECS_Unoccupied;	 // Combat상태 변경
	}
}

/*
FVector_NetQuantize
1. 양자화: 벡터의 각 구성 요소를 작은 크기의 정수로 양자화합니다. 
이를 통해, 네트워크를 통해 전송되는 데이터의 크기가 줄어들고 이러한 최적화는 
특히 모바일 게임이나 대규모 멀티플레이어 게임에서 중요

2. 손실있는 압축: 양자화는 일반적으로 손실이 있는 압축을 사용
따라서 원래 FVector와 FVector_NetQuantize 사이에 약간의 차이가 발생할 수 있다.
이 차이는 대부분의 경우 눈에 띄지 않을 정도로 작습니다.

3. 네트워크 최적화: FVector_NetQuantize는 네트워크 전송을 위해 특별히 설계되었으므로, 
다른 벡터 타입에 비해 네트워크에서 더 효율적으로 전송될 수 있습니다.

4. 빌트인 지원: 언리얼 엔진의 네트워크 시스템은 이 타입의 벡터에 대한 지원이 빌트인되어 있으므로, 
개발자는 별도의 복잡한 코드 없이도 이러한 최적화를 쉽게 이용할 수 있다.

FVector_NetQuantize는 위치, 속도 등과 같은 게임 내에서 네트워크를 통해 자주 전송되는 벡터 값에 유용하게 사용될 수 있으며, 이러한 값을 전송할 때 발생하는 데이터 크기의 부하를 줄일 수 있습니다.
*/
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	// 서버에서 멀티케스트 RPC를 호출하면 서버에서 실행되고 모든 클라이언트에서 같이 실행된다.
	MulticastFire(TraceHitTarget);

	// 만약 MulticastFire 안에 있는 코드를 여기에 작성하고 실행하면
	// 클라에서 총을 쏘면 총기 애니메이션 관련된 부분이 서버에서밖에 안 보인다
	// 그래서 멀티케스트 RPC를 활용하여 전부 실행하게 하는것.
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
	// 로컬에서는 이미 해당 작업을 맞췄기때문에 아래의 조건이라면 리턴
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
{// 로컬에서는 이미 해당 작업을 맞췄기때문에 아래의 조건이라면 리턴
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

	// 화면공간의 2D좌표 CrosshairLocation을 3D 월드 공간의 점 위치와 방향으로 변환
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

		// Character가 Interface를 상속받은 상태기 때문에
		// TraceHit된 객체가 Character라면 Crosshair를 빨간색으로 변경
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			// Trace가 벽에 충돌했다면 White로 변경
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
		// 트레이스 끝이 충돌되지 않았을때는 강제로 끝 충돌위치를 End로 지정
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

	// 컨트롤러 지정
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	// HUD 세팅
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;

		if (HUD)
		{
			// 장착된 무기가 있다면 HUD지정하고 장착된 무기에 따른 크로스 헤어 지정
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
				// 장착된 무기가 없다면 HUD의 크로스헤어 텍스처 nullptr
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}
			// 크로스헤어 십자선 확장 계산
			// 플레이어 최대속도
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			// 플레이어 맵핑 범위
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			// 플레이어 현재 속도 : z는 0
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			// 플레이어 현재의 범위 맵핑된 값을 구함
			// 0 ~ 1의 값이 나오게됨
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

			// 총을 발사할때 CrosshairShootingFactor를 0.2f로 지정해놓으면 알아서 여기서 0으로 보간되며 줄어듬
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
		// 줌을 하는순간 무기마다의 줌 속도를 갖고 해당 줌 속도만큼 줌이 된다
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		// 줌이 끝나면 기존의 FOV값으로 되돌린다
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	// 최종적으로 카메라 FOV 변경
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
	// 자동화기 무기여야지만 가능
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
	// 샷건의 리로드중 쏠 수 있게 하기위함
	if (!EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		return true;
	// 로컬 리로드로 패브릭 제어
	if (bLocallyReloading)
		return false;

	// 무기의 탄약개수가 0이 아니고 bCanFire이 false라면
	return !EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// 아래와 같은 상황이 트루일때만 JumpToShotgunEnd를 진행한다
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
	// 무기 타입별 총알 개수 저장
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
	// 갖고있는 탄약은 오로지 소유 클라이언트에서만 사용하기 때문(리로딩) 
	// 이렇게 해야 송신되는 데이터를 최소화하면서 성능 향상이 있다.
	// 서버는 CarriedAmmo를 다른 클라이언트에게 복제하지 않고 소유하는 클라이언트만 복제한다.
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}


// 서버 구간임
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
		return;

	// ECS_Unoccupied상태일때만 무기를 장착할 수 있다
	// 즉 재장전중이거나, 수류탄을 던지고 있을때는 불가능하다.
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
	// 무기의 소유자를 지정해줘야함
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	// 서버에서 호출되지만 CarriedAmmo를 복제 변수로 만들었기 때문에 
	// CarriedAmmo를 설정하는 즉시 클라이언트의 HUD를
	// Controller->SetHUDCarriedAmmo(CarriedAmmo);를 통해 업데이트 할 수 있다.
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
		// 클라이언트는 로컬값이 정확해야한다.
		bAiming = bAimButtonPressed;
	}
}


void UCombatComponent::Reload()
{
	// 클라이언트에 있는 경우 모든 클라이언트에게 다시 리로드 애니메이션을 재생할 시간임을 알리기 전에
	// 서버가 다시 로드할 수 있는지 확인하도록 RPC를 서버로 보내야 한다.
	// Ammo가 0개 이상이고, Cobat상태가 ECS_Unoccupied일때만 가능
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
		// 총알 업데이트
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

	// CombatState상태를 변경하면 OnRep이 실행된다
	CombatState = ECombatState::ECS_Reloading;
	// 즉, 서버에서도 해당 함수를 호출하고 클라에서도 동일하게 호출해줘야함
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
		// CobmatState상태가 변경되었는데 FireButton을 클릭하고 있다면 발사
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		// 클라에서도 해당 함수를 호출
		if (Character && !Character->IsLocallyControlled())
			HandleReload();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// 다른 클라이언트에서만 실행되도록
		// 로컬플레이어는 이미 ThrowGrenade 몽타주를 실행중이기 때문에 아닐때만 실행
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		// 다른 클라이언트에서만 실행되도록
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

	// 장전해야할 총알의 개수
	int32 ReloadAmount = AmountToReload();

	// 현재 들고있는 총알의 개수를 갱신
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 기존에 들고있던 총알의 개수에서 장전해야할 총알의 개수를 빼준다
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		// 들고있는 탄약 복제
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
	// 무기의 총알 갱신
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 기존에 들고있던 총알의 개수에서 장전해야할 총알의 개수를 빼준다
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		// 들고있는 탄약 복제
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// 무기의 총알 갱신
	EquippedWeapon->AddAmmo(1);
	// true로 하면 장전후 중간에 다시 쏠 수 있다.
	bCanFire = true;
	// 총알 장전도중 무기에 들어있는 총알이 최대거나
	// 갖고있는 총알이 0이라면
	if (EquippedWeapon->IsAmmoFull() || CarriedAmmo == 0)
	{
		// 장전이 끝났다면 몽타주 섹션을 ShotgunEnd로 이동해야함
		// 이것을 해야하는 이유는 2발이 있는 상태에서 장전했을때는 남은 총알수
		// 2발만 장전해야하는데 애니메이션은 총 4발 장전하는 애니메이션이기 때문
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	// 컨트롤러 지정
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
	// 현재 들고있는 총의 장전할 수 있는 총알의 개수
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 현재 들고있는 총알의 총 개수
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		// 최소값 (더 넘게 장전하지 않기 위해서)
		// 들고있는 총알이 10개 보다 많아도 장전할수 있는 최대의 총알수가 10이면
		// 총알은 10개를 장전해야한다.
		// 그리고 들고있는 총알이 장전할 수 있는 최대의 총알수 10개보다 작다면
		// 그대로 들고있는 총알의 개수만큼만 장전하는것이다.
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
	// 이미 무기가 있다면 기존에 있던 무기를 버린다.
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
	// 자동장전
	// 처음 무기 먹었을때 무기의 총알이 비어있을 경우 장전
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
