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
			// 캐릭터의 카메라가 사용중인 FOV값
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

		// 총을 발사한 뒤 FireTimer를 실행한다.
		// 그래야 총을 쏜 뒤의 딜레이 만큼 실행
		StartFireTimer();
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
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 서버에서 멀티케스트 RPC를 호출하면 서버에서 실행되고 모든 클라이언트에서 같이 실행된다.
	MulticastFire(TraceHitTarget);

	// 만약 MulticastFire 안에 있는 코드를 여기에 작성하고 실행하면
	// 클라에서 총을 쏘면 총기 애니메이션 관련된 부분이 서버에서밖에 안 보인다
	// 그래서 멀티케스트 RPC를 활용하여 전부 실행하게 하는것.
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
	// 무기의 소유자를 지정해줘야함
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

