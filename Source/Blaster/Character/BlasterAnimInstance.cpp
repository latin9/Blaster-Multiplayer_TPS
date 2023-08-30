// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Weapon/Weapon.h"
#include "../BlasterType/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	
	// 처음 널이라면 얻어옴
	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	// 위에서 얻어왔는데도 널이라면 리턴
	if (BlasterCharacter == nullptr)
		return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.0;

	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	// 삼항 연산자와 현재의 속도를 판단해 가속중인지 판단
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0 ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();

	// 글로벌 Rotation이다. 캐릭터 축 기준이 아님 범위 : -180 ~ 180
	// Offset Yaw For Strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());

	// 좌 우 움직일때 애니메이션 부드럽게 보간되도록 보간된 Yaw값 추출
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	// FABRIK IK
	// 무기를 장착했고 장착한 무기가 있고 장착한 무기의 스켈레탈메쉬가 있고 플레이어의 스켈레탈 메쉬도 있을경우에만 허용
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// 소켓의 정보를 월드위치로 얻어옴
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		// 값을 추출해 저장하기 위한 변수
		FVector OutPosition;
		FRotator OutRotation;
		//TransformToBoneSpace 함수를 사용하여 왼손의 위치와 회전을 무기의 소켓 위치로 변환
		// 해당 함수는 지정한 본 ("hand_r")의 공간에 대한 상대적인 변환을 계산합니다.
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(),
			FRotator::ZeroRotator, OutPosition, OutRotation);
		// 계산된 위치 OutPosition와 회전 OutRotation으로 LeftHandTransform의 위치와 회전을 업데이트
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			// 오른손의 회전을 타겟에 맞춰야함
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("RightHandSocket"), ERelativeTransformSpace::RTS_World);
			// 왼손에서 타겟의 로테이션값을 추출
			//RightHandRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}

		// 리로딩할때는 FABRIK을 사용하면 안된다
		bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
		if (BlasterCharacter->IsLocallyControlled() && BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade)
		{
			bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
		}
		bUseAimOffsets = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
		bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();



		//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		//// 총구 머즐소켓의 X충 방향의 FVector값
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//// 총구의 끝과 타겟이 얼마나 어긋나 있는지 확인하기 위한 디버그 라인
		//// 총구의 전방 라인
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		//// 머즐에서 타겟 라인
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);
	}
}
