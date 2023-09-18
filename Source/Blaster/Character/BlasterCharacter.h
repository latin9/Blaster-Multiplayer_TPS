// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterType/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "../BlasterType/CombatState.h"
#include "../BlasterType/Team.h"
#include "BlasterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents()	override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	// 시뮬레이션 프록시에 대한 캐릭터 회전의 델타를 확인할 때 틱 기능 대신 이것을 사용
	virtual void OnRep_ReplicatedMovement() override;

	// 게임 모드는 서버에만 존재하고 서버에서 Elim을 실행하기 때문에 여긴 서버에 해당
	void Elim(bool bPlayerLeftGame);
	// 죽는것은 신뢰해야됨 중요한작업
	// 서버에서만 실행되면 안된다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);
	
	/*UFUNCTION(Server, Reliable)
	void ServerElimDestroyed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimDestroyed();*/


	virtual void Destroyed() override;

	// BlueprintImplementableEvent : 블루프린트에서 구현할 수 있음
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();
	
	FOnLeftGame OnLeftGame;

	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	void SetTeamColor(ETeam _Team);

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	// E 버튼 클릭(장비 장착)
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void GrenadeButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	float CalculateSpeed();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimproxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	// 관련된 모든 클래스에 대한 Poll, HUD에 유효한 데이터를 초기화 하는 함수
	void PollInit();
	
	void RotateInPlace(float DeltaTime);
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

	void SetSpawnPoint();
	void OnPlayerStateInitialized();

public:

protected:

private:
	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(EditAnywhere, Category = "Player Name")
	FString LocalPlayerName = TEXT("Unknown Player");

	// 서버에서 변경될때 모든 클라이언트에서 변경되고 무기에 대한 포인터를 복제할 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	// LastWeapon에는 복제되기전의 OverlappingWeapon이 들어간다
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);


	// BlasterComponents
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;
	
	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	// 서버 RPC를 신뢰할 수 있도록 Reliable 선언
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltaTime);

	// Anim Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* SwapMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.25f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	// Player Health
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	// 현재 체력
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	// Player Shield
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	// 현재 쉴드
	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 1000.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;
	// 무조건 초기값으로만 설정가능
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	// Dissolve Effect
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// 런타임에 생성하는 동적 인스턴스를 저장하기 위해 사용
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// 블루프린트에 설정된 머티리얼 인스턴스, 동적 미터리얼 인스턴스와 함께 사용
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	// Team Colors
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueDissolveMatInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueMaterial;
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedDissolveMatInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedMaterial;
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* OriginalMaterial;

	// Elim Effect
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	// 수류탄 관련
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* AttachedGrenade;

	// 디폴트 무기
	// 캐릭터 생성시 바로 장착되는 기본 무기
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	// 히트박스 Server-side rewind용
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Head;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Pelvis;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Spine_02;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Spine_03;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperArm_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperArm_R;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerArm_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerArm_R;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Hand_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Hand_R;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* BackpackBottom;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* BackpackTop;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Thigh_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Thigh_R;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Calf_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Calf_R;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Foot_L;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Foot_R;

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishedSwapping = false;

	bool bLeftGame = false;


public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget()	const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const{ return Health; }
	FORCEINLINE void SetHealth(float _Health) {Health = _Health; }
	FORCEINLINE float GetMaxHealth() const{ return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float _Shield) { Shield = _Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE void SetDisableGameplay(bool Enable) { bDisableGameplay = Enable; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE class UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE class UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE class UBuffComponent* GetBuffComponent() const { return Buff; }

	// 히트박스 Server-side rewind용
	FORCEINLINE class UBoxComponent* GetHeadComponent() const { return Head; }
	FORCEINLINE class UBoxComponent* GetPelvisComponent() const { return Pelvis; }
	FORCEINLINE class UBoxComponent* GetSpine02Component() const { return Spine_02; }
	FORCEINLINE class UBoxComponent* GetSpine03Component() const { return Spine_03; }
	FORCEINLINE class UBoxComponent* GetUpperArmLComponent() const { return UpperArm_L; }
	FORCEINLINE class UBoxComponent* GetUpperArmRComponent() const { return UpperArm_R; }
	FORCEINLINE class UBoxComponent* GetLowerArmLComponent() const { return LowerArm_L; }
	FORCEINLINE class UBoxComponent* GetLowerArmRComponent() const { return LowerArm_R; }
	FORCEINLINE class UBoxComponent* GetHandLComponent() const { return Hand_L; }
	FORCEINLINE class UBoxComponent* GetHandRComponent() const { return Hand_R; }
	FORCEINLINE class UBoxComponent* GetBackpackBottomComponent() const { return BackpackBottom; }
	FORCEINLINE class UBoxComponent* GetBackpackTopComponent() const { return BackpackTop; }
	FORCEINLINE class UBoxComponent* GetThighLComponent() const { return Thigh_L; }
	FORCEINLINE class UBoxComponent* GetThighRComponent() const { return Thigh_R; }
	FORCEINLINE class UBoxComponent* GetCalfLComponent() const { return Calf_L; }
	FORCEINLINE class UBoxComponent* GetCalfRComponent() const { return Calf_R; }
	FORCEINLINE class UBoxComponent* GetFootLComponent() const { return Foot_L; }
	FORCEINLINE class UBoxComponent* GetFootRComponent() const { return Foot_R; }
	FORCEINLINE TMap<FName, class UBoxComponent*> GetHitColisionBoxes() const { return HitCollisionBoxes; }
	FORCEINLINE class ULagCompensationComponent* GetLagCompensationComponent() const { return LagCompensation; }
	FORCEINLINE bool IsSwappingFinished() const { return bFinishedSwapping; }
	FORCEINLINE void SetSwappingFinished(bool _IsFinished) { bFinishedSwapping = _IsFinished; }
	class UBoxComponent* GetHitColisionBoxFromFName(const FName& Name);
	bool IsHoldingTheFlag() const;
	ETeam GetTeam();

	//FORCEINLINE FOnLeftGame GetOnLeftGame() { return OnLeftGame; }

	void SetHoldingTheFlag(bool bHolding);


	bool IsLocallyReloading();
public: 
	UFUNCTION(Client, Reliable)
	void ClientSetName(const FString& Name);

	UFUNCTION(Server, Reliable)
	void ServerSetPlayerName(const FString& PlayerName);


};
