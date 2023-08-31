// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "../Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();


}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		ShowFramePackage(ThisFrame, FColor::Red);
	}
	else
	{
		// 가장 최근에 저장된 데이터의 시간에서 가장 오래된 데이터의 시간을 빼준다.
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime)
		{
			// 가장 오래된 데이터 제거
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			// 가장 오래된 데이터를 제거했기 때문에 HistoryLength는 다시 갱신해야한다.
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		// 저장한 Length시간이 Max보다 작다면 계속 FramePackage데이터 저장
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		// 서버에서만 사용하니 GetWorld()->GetTimeSeconds()를 호출하면 서버 시간을 받아온다.
		Package.Time = GetWorld()->GetTimeSeconds();
		for (auto& BoxPair : Character->GetHitColisionBoxes())
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	// 0에서 1 사이가 되어야함
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;
		// 델타타임 위치에 1을 넣으면 델타타임을 사용하지 않는다는것
		// Hit을 사이로 둔 Older, Young를 보간하여 Hit 부분을 추려낸다.
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

// 해당 함수에 들어온 Package매개변수는 보간된 HitPackage이다
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, 
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr)
		return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	// 박스 충돌 처리를 해야하기 때문에 캐릭터 메시 충돌안되도록 설정
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// 먼저 머리에 대한 충돌을 활성화
	UBoxComponent* HeadBox = HitCharacter->GetHitColisionBoxFromFName(FName("Head"));
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();

	if (World)
	{
		World->LineTraceSingleByChannel(ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);
		// 위에서 먼저 헤드 충돌체만 충돌 되도록 설정했기 때문에 여기서 충돌이 되었다면 무조건 헤드샷이다
		// 헤드샷이라면 return
		if (ConfirmHitResult.bBlockingHit)
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			// 적중되었고 헤드샷이다라고 리턴
			return FServerSideRewindResult{ true, true };
		}
		// 헤드샷이 아니라면 나머지도 체크
		else
		{
			for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECollisionChannel::ECC_Visibility
			);

			// 헤드 제외한 부분 맞은것
			if (ConfirmHitResult.bBlockingHit)
			{
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				// 적중됐고 헤드샷이 아니라고 리턴
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	// 빗나간 경우
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// 적중되지 않았고 헤드샷도 아니라고 리턴
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}

}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn = HitCharacter == nullptr || 
		HitCharacter->GetLagCompensationComponent() == nullptr || 
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr || 
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;

	if (bReturn)
		return FServerSideRewindResult();
	// 적중 여부를 확인하기 위한 프레임 패키지
	FFramePackage FrameToCheck;

	bool bShouldInterpolate = true;

	// HitCharacter의 프레임 히스토리
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;

	if (OldestHistoryTime > HitTime)
	{
		// OldestHistoryTime가 HitTime보다 크다면 서버 측 되감기를 수행하기에는 늦은것
		return FServerSideRewindResult();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	// Young, Old 모두 Head로 가장 최신 데이터를 갖고있는 포인터로 등록
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr)
			break;

		// 여기에 들어왔다면 Older보다 더 시간대가 뒤에있다는 의미이므로
		// Older를 Next노드로 갱신해야함(더 오래된 데이터를 확이해야함)
		Older = Older->GetNextNode();
	
		// 갱신후 다시 체크했는데 그래도 HitTime보다 최신이라면 Young데이터를 Older 데이터로 갱신
		// 헤드에서부터 꼬리까지 시간별 체크하는것
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	// 거의 가능성이 없지만 그래도 체크해야됨
	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		// Young, Old 사이의 보간 진행
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

