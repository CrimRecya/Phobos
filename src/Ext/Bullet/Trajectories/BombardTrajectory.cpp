#include "BombardTrajectory.h"
#include "Memory.h"

#include <AnimClass.h>
#include <Ext/Anim/Body.h>
#include <Ext/Bullet/Body.h>

std::unique_ptr<PhobosTrajectory> BombardTrajectoryType::CreateInstance() const
{
	return std::make_unique<BombardTrajectory>(this);
}

template<typename T>
void BombardTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->Height)
		.Process(this->FallPercent)
		.Process(this->FallPercentShift)
		.Process(this->FallScatter_Max)
		.Process(this->FallScatter_Min)
		.Process(this->FallSpeed)
		.Process(this->DetonationDistance)
		.Process(this->DetonationHeight)
		.Process(this->EarlyDetonation)
		.Process(this->TargetSnapDistance)
		.Process(this->FreeFallOnTarget)
		.Process(this->LeadTimeCalculate)
		.Process(this->NoLaunch)
		.Process(this->TurningPointAnims)
		.Process(this->OffsetCoord)
		.Process(this->RotateCoord)
		.Process(this->MirrorCoord)
		.Process(this->UseDisperseBurst)
		.Process(this->AxisOfRotation)
		.Process(this->SubjectToGround)
		;
}

bool BombardTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool BombardTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<BombardTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void BombardTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	INI_EX exINI(pINI);

	this->Height.Read(exINI, pSection, "Trajectory.Bombard.Height");
	this->FallPercent.Read(exINI, pSection, "Trajectory.Bombard.FallPercent");
	this->FallPercentShift.Read(exINI, pSection, "Trajectory.Bombard.FallPercentShift");
	this->FallScatter_Max.Read(exINI, pSection, "Trajectory.Bombard.FallScatter.Max");
	this->FallScatter_Min.Read(exINI, pSection, "Trajectory.Bombard.FallScatter.Min");
	this->FallSpeed.Read(exINI, pSection, "Trajectory.Bombard.FallSpeed");
	this->DetonationDistance.Read(exINI, pSection, "Trajectory.Bombard.DetonationDistance");
	this->DetonationHeight.Read(exINI, pSection, "Trajectory.Bombard.DetonationHeight");
	this->EarlyDetonation.Read(exINI, pSection, "Trajectory.Bombard.EarlyDetonation");
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.Bombard.TargetSnapDistance");
	this->FreeFallOnTarget.Read(exINI, pSection, "Trajectory.Bombard.FreeFallOnTarget");
	this->LeadTimeCalculate.Read(exINI, pSection, "Trajectory.Bombard.LeadTimeCalculate");
	this->NoLaunch.Read(exINI, pSection, "Trajectory.Bombard.NoLaunch");
	this->TurningPointAnims.Read(exINI, pSection, "Trajectory.Bombard.TurningPointAnims");
	this->OffsetCoord.Read(exINI, pSection, "Trajectory.Bombard.OffsetCoord");
	this->RotateCoord.Read(exINI, pSection, "Trajectory.Bombard.RotateCoord");
	this->MirrorCoord.Read(exINI, pSection, "Trajectory.Bombard.MirrorCoord");
	this->UseDisperseBurst.Read(exINI, pSection, "Trajectory.Bombard.UseDisperseBurst");
	this->AxisOfRotation.Read(exINI, pSection, "Trajectory.Bombard.AxisOfRotation");
	this->SubjectToGround.Read(exINI, pSection, "Trajectory.Bombard.SubjectToGround");
}

template<typename T>
void BombardTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->Height)
		.Process(this->FallPercent)
		.Process(this->FallSpeed)
		.Process(this->OffsetCoord)
		.Process(this->IsFalling)
		.Process(this->ToFalling)
		.Process(this->RemainingDistance)
		.Process(this->LastTargetCoord)
		.Process(this->WaitOneFrame)
		.Process(this->AscendTime)
		;
}

bool BombardTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->Serialize(Stm);
	return true;
}

bool BombardTrajectory::Save(PhobosStreamWriter& Stm) const
{
	const_cast<BombardTrajectory*>(this)->Serialize(Stm);
	return true;
}

void BombardTrajectory::OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity)
{
	const BombardTrajectoryType* const pType = this->Type;
	this->LastTargetCoord = pBullet->TargetCoords;
	pBullet->Velocity = BulletVelocity::Empty;

	if (TechnoClass* const pOwner = pBullet->Owner)
	{
		if (pType->MirrorCoord && pOwner->CurrentBurstIndex % 2 == 1)
			this->OffsetCoord.Y = -(this->OffsetCoord.Y);
	}

	if (!pType->NoLaunch || !pType->LeadTimeCalculate || !abstract_cast<FootClass*>(pBullet->Target))
		this->PrepareForOpenFire(pBullet);
	else
		this->WaitOneFrame.Start(1);
}

bool BombardTrajectory::OnAI(BulletClass* pBullet)
{
	if (this->WaitOneFrame.IsTicking() && this->BulletPrepareCheck(pBullet))
		return false;

	if (this->BulletDetonatePreCheck(pBullet))
		return true;

	// Extra check for trajectory falling
	auto const pOwner = pBullet->Owner ? pBullet->Owner->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

	if (this->IsFalling && !this->Type->FreeFallOnTarget && this->BulletDetonateRemainCheck(pBullet, pOwner))
		return true;

	this->BulletVelocityChange(pBullet);

	return false;
}

void BombardTrajectory::OnAIPreDetonate(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;
	auto pTarget = abstract_cast<ObjectClass*>(pBullet->Target);
	auto pCoords = pTarget ? pTarget->GetCoords() : pBullet->Data.Location;

	if (pCoords.DistanceFrom(pBullet->Location) <= pType->TargetSnapDistance.Get())
	{
		auto const pExt = BulletExt::ExtMap.Find(pBullet);
		pExt->SnappedToTarget = true;
		pBullet->SetLocation(pCoords);
	}
}

void BombardTrajectory::OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition)
{
	pSpeed->Z += BulletTypeExt::GetAdjustedGravity(pBullet->Type); // We don't want to take the gravity into account
}

TrajectoryCheckReturnType BombardTrajectory::OnAITargetCoordCheck(BulletClass* pBullet)
{
	return TrajectoryCheckReturnType::SkipGameCheck; // Bypass game checks entirely.
}

TrajectoryCheckReturnType BombardTrajectory::OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno)
{
	return TrajectoryCheckReturnType::SkipGameCheck; // Bypass game checks entirely.
}

void BombardTrajectory::PrepareForOpenFire(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;
	this->Height += pBullet->TargetCoords.Z;
	// use scaling since RandomRanged only support int
	this->FallPercent += ScenarioClass::Instance->Random.RandomRanged(0, static_cast<int>(200 * pType->FallPercentShift)) / 100.0;

	const double rotateAngle = this->CalculateTargetCoords(pBullet);

	if (!pType->NoLaunch)
	{
		const CoordStruct middleLocation = this->CalculateMiddleCoords(pBullet);

		pBullet->Velocity.X = static_cast<double>(middleLocation.X - pBullet->SourceCoords.X);
		pBullet->Velocity.Y = static_cast<double>(middleLocation.Y - pBullet->SourceCoords.Y);
		pBullet->Velocity.Z = static_cast<double>(middleLocation.Z - pBullet->SourceCoords.Z);
		pBullet->Velocity *= pType->Trajectory_Speed / pBullet->Velocity.Magnitude();

		WeaponTypeClass* const pWeapon = pBullet->WeaponType;
		const int countOfBurst = pWeapon ? pWeapon->Burst : 0;

		if (!pType->UseDisperseBurst && abs(pType->RotateCoord) > 1e-10 && countOfBurst > 1)
		{
			const CoordStruct axis = pType->AxisOfRotation;

			BulletVelocity rotationAxis
			{
				axis.X * Math::cos(rotateAngle) + axis.Y * Math::sin(rotateAngle),
				axis.X * Math::sin(rotateAngle) - axis.Y * Math::cos(rotateAngle),
				static_cast<double>(axis.Z)
			};

			const double rotationAxisLengthSquared = rotationAxis.MagnitudeSquared();

			if (abs(rotationAxisLengthSquared) > 1e-10)
			{
				double extraRotate = 0.0;
				rotationAxis *= 1 / sqrt(rotationAxisLengthSquared);

				TechnoClass* const pFirer = pBullet->Owner;
				const int currentBurst = pFirer ? pFirer->CurrentBurstIndex : 0;

				if (pType->MirrorCoord)
				{
					if (currentBurst % 2 == 1)
						rotationAxis *= -1;

					extraRotate = Math::Pi * (pType->RotateCoord * ((currentBurst / 2) / (countOfBurst - 1.0) - 0.5)) / 180;
				}
				else
				{
					extraRotate = Math::Pi * (pType->RotateCoord * (currentBurst / (countOfBurst - 1.0) - 0.5)) / 180;
				}

				const double cosRotate = Math::cos(extraRotate);
				pBullet->Velocity = (pBullet->Velocity * cosRotate) + (rotationAxis * ((1 - cosRotate) * (pBullet->Velocity * rotationAxis))) + (rotationAxis.CrossProduct(pBullet->Velocity) * Math::sin(extraRotate));
			}
		}
	}
	else
	{
		this->IsFalling = true;
		CoordStruct middleLocation = CoordStruct::Empty;

		if (!pType->FreeFallOnTarget)
		{
			middleLocation = this->CalculateMiddleCoords(pBullet);

			pBullet->Velocity.X = static_cast<double>(pBullet->TargetCoords.X - middleLocation.X);
			pBullet->Velocity.Y = static_cast<double>(pBullet->TargetCoords.Y - middleLocation.Y);
			pBullet->Velocity.Z = static_cast<double>(pBullet->TargetCoords.Z - middleLocation.Z);
			pBullet->Velocity *= this->FallSpeed / pBullet->Velocity.Magnitude();

			this->RemainingDistance += static_cast<int>(pBullet->TargetCoords.DistanceFrom(middleLocation) + this->FallSpeed);
		}
		else
		{
			middleLocation = CoordStruct { pBullet->TargetCoords.X, pBullet->TargetCoords.Y, static_cast<int>(this->Height) };
		}

		pBullet->SetLocation(middleLocation);

		HouseClass* const pOwner = pBullet->Owner ? pBullet->Owner->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
		this->ApplyTurningPointAnim(pType->TurningPointAnims, middleLocation, pBullet->Owner, pOwner, true);
	}
}

CoordStruct BombardTrajectory::CalculateMiddleCoords(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;
	const double angel = ScenarioClass::Instance->Random.RandomDouble() * Math::TwoPi;
	const double length = ScenarioClass::Instance->Random.RandomRanged(pType->FallScatter_Min.Get(), pType->FallScatter_Max.Get());
	const int scatterX = static_cast<int>(length * Math::cos(angel));
	const int scatterY = static_cast<int>(length * Math::sin(angel));

	return CoordStruct
	{
		pBullet->SourceCoords.X + static_cast<int>((pBullet->TargetCoords.X - pBullet->SourceCoords.X) * this->FallPercent) + scatterX,
		pBullet->SourceCoords.Y + static_cast<int>((pBullet->TargetCoords.Y - pBullet->SourceCoords.Y) * this->FallPercent) + scatterY,
		static_cast<int>(this->Height)
	};
}

double BombardTrajectory::CalculateTargetCoords(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;
	double rotateAngle = 0.0;
	const AbstractClass* const pTarget = pBullet->Target;
	CoordStruct theTargetCoords = pBullet->TargetCoords;
	CoordStruct theSourceCoords = pBullet->SourceCoords;

	if (pType->LeadTimeCalculate && pTarget)
	{
		theTargetCoords = pTarget->GetCoords();
		theSourceCoords = pBullet->Location;

		if (theTargetCoords != this->LastTargetCoord)
		{
			if (pType->NoLaunch && pType->FreeFallOnTarget)
			{
				const CoordStruct extraOffsetCoord = pTarget->GetCoords() - this->LastTargetCoord;
				const int travelTime = static_cast<int>(sqrt(2 * (this->Height - pTarget->GetCoords().Z) / BulletTypeExt::GetAdjustedGravity(pBullet->Type)));
				theTargetCoords += extraOffsetCoord * (travelTime + 1);
			}
			else
			{
				CoordStruct extraOffsetCoord = theTargetCoords - this->LastTargetCoord;
				CoordStruct lastSourceCoord = theSourceCoords - this->LastTargetCoord;
				CoordStruct targetSourceCoord = theSourceCoords - theTargetCoords;
				const CoordStruct realExtraOffsetCoord = extraOffsetCoord;

				if (pType->FreeFallOnTarget)
				{
					CoordStruct theTurningPointCoords = theTargetCoords;
					CoordStruct theLastTurningPointCoords = this->LastTargetCoord;
					theTurningPointCoords.Z += static_cast<int>(pType->Height); // Use original height here
					theLastTurningPointCoords.Z += static_cast<int>(pType->Height);

					if (this->FallPercent != 1.0)
					{
						theTurningPointCoords.X = theSourceCoords.X - static_cast<int>(targetSourceCoord.X * this->FallPercent);
						theTurningPointCoords.Y = theSourceCoords.Y - static_cast<int>(targetSourceCoord.Y * this->FallPercent);

						theLastTurningPointCoords.X = theSourceCoords.X - static_cast<int>(lastSourceCoord.X * this->FallPercent);
						theLastTurningPointCoords.Y = theSourceCoords.Y - static_cast<int>(lastSourceCoord.Y * this->FallPercent);

						extraOffsetCoord = theTurningPointCoords - theLastTurningPointCoords;
					}

					targetSourceCoord = theSourceCoords - theTurningPointCoords;
					lastSourceCoord = theSourceCoords - theLastTurningPointCoords;
				}

				const double theDistanceSquared = targetSourceCoord.MagnitudeSquared();

				const double targetSpeedSquared = extraOffsetCoord.MagnitudeSquared();
				const double targetSpeed = sqrt(targetSpeedSquared);

				const double crossFactor = lastSourceCoord.CrossProduct(targetSourceCoord).MagnitudeSquared();
				const double verticalDistanceSquared = crossFactor / targetSpeedSquared;

				const double horizonDistanceSquared = theDistanceSquared - verticalDistanceSquared;
				const double horizonDistance = sqrt(horizonDistanceSquared);

				const double straightSpeed = pType->FreeFallOnTarget ? pType->Trajectory_Speed : this->FallSpeed;
				const double straightSpeedSquared = straightSpeed * straightSpeed;

				const double baseFactor = straightSpeedSquared - targetSpeedSquared;
				const double squareFactor = baseFactor * verticalDistanceSquared + straightSpeedSquared * horizonDistanceSquared;

				if (squareFactor > 1e-10)
				{
					const double minusFactor = -(horizonDistance * targetSpeed);
					int travelTime = 0;

					if (abs(baseFactor) < 1e-10)
					{
						travelTime = abs(horizonDistance) > 1e-10 ? (static_cast<int>(theDistanceSquared / (2 * horizonDistance * targetSpeed)) + 1) : 0;
					}
					else
					{
						const int travelTimeM = static_cast<int>((minusFactor - sqrt(squareFactor)) / baseFactor);
						const int travelTimeP = static_cast<int>((minusFactor + sqrt(squareFactor)) / baseFactor);

						if (travelTimeM > 0 && travelTimeP > 0)
							travelTime = travelTimeM < travelTimeP ? travelTimeM : travelTimeP;
						else if (travelTimeM > 0)
							travelTime = travelTimeM;
						else if (travelTimeP > 0)
							travelTime = travelTimeP;

						if (pType->FreeFallOnTarget)
							travelTime += static_cast<int>(sqrt(2 * (this->Height - theTargetCoords.Z) / BulletTypeExt::GetAdjustedGravity(pBullet->Type)));

						travelTime += this->AscendTime;

						if (targetSourceCoord.MagnitudeSquared() >= lastSourceCoord.MagnitudeSquared())
							travelTime += 1;
					}

					theTargetCoords += realExtraOffsetCoord * travelTime / this->AscendTime;
				}
			}
		}
	}

	pBullet->TargetCoords = theTargetCoords;

	if (!pType->LeadTimeCalculate && theTargetCoords == theSourceCoords && pBullet->Owner) //For disperse.
	{
		const CoordStruct theOwnerCoords = pBullet->Owner->GetCoords();
		rotateAngle = Math::atan2(theTargetCoords.Y - theOwnerCoords.Y , theTargetCoords.X - theOwnerCoords.X);
	}
	else
	{
		rotateAngle = Math::atan2(theTargetCoords.Y - theSourceCoords.Y , theTargetCoords.X - theSourceCoords.X);
	}

	if (this->OffsetCoord != CoordStruct::Empty)
	{
		pBullet->TargetCoords.X += static_cast<int>(this->OffsetCoord.X * Math::cos(rotateAngle) + this->OffsetCoord.Y * Math::sin(rotateAngle));
		pBullet->TargetCoords.Y += static_cast<int>(this->OffsetCoord.X * Math::sin(rotateAngle) - this->OffsetCoord.Y * Math::cos(rotateAngle));
		pBullet->TargetCoords.Z += this->OffsetCoord.Z;
	}

	if (pBullet->Type->Inaccurate)
	{
		auto const pTypeExt = BulletTypeExt::ExtMap.Find(pBullet->Type);
		const double offsetMult = 0.0004 * pBullet->SourceCoords.DistanceFrom(pBullet->TargetCoords);
		const int offsetMin = static_cast<int>(offsetMult * pTypeExt->BallisticScatter_Min.Get(Leptons(0)));
		const int offsetMax = static_cast<int>(offsetMult * pTypeExt->BallisticScatter_Max.Get(Leptons(RulesClass::Instance->BallisticScatter)));
		const int offsetDistance = ScenarioClass::Instance->Random.RandomRanged(offsetMin, offsetMax);
		pBullet->TargetCoords = MapClass::GetRandomCoordsNear(pBullet->TargetCoords, offsetDistance, false);
	}

	return rotateAngle;
}

bool BombardTrajectory::BulletPrepareCheck(BulletClass* pBullet)
{
	if (this->WaitOneFrame.HasTimeLeft())
		return true;

	this->PrepareForOpenFire(pBullet);
	this->WaitOneFrame.Stop();

	return false;
}

bool BombardTrajectory::BulletDetonatePreCheck(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;

	// Close enough
	if (pBullet->TargetCoords.DistanceFrom(pBullet->Location) < pType->DetonationDistance.Get())
		return true;

	// Height
	if (pType->DetonationHeight >= 0)
	{
		if (pType->EarlyDetonation && (pBullet->Location.Z - pBullet->SourceCoords.Z) > pType->DetonationHeight)
			return true;
		else if (this->IsFalling && (pBullet->Location.Z - pBullet->SourceCoords.Z) < pType->DetonationHeight)
			return true;
	}

	// Ground, must be checked when free fall
	if (pType->SubjectToGround || (this->IsFalling && pType->FreeFallOnTarget))
	{
		if (MapClass::Instance->GetCellFloorHeight(pBullet->Location) >= (pBullet->Location.Z + 15))
			return true;
	}

	return false;
}

bool BombardTrajectory::BulletDetonateRemainCheck(BulletClass* pBullet, HouseClass* pOwner)
{
	this->RemainingDistance -= static_cast<int>(this->FallSpeed);

	if (this->RemainingDistance < 0)
		return true;

	if (this->RemainingDistance < this->FallSpeed)
	{
		pBullet->Velocity *= this->RemainingDistance / this->FallSpeed;
		this->RemainingDistance = 0;
	}

	return false;
}

void BombardTrajectory::BulletVelocityChange(BulletClass* pBullet)
{
	const BombardTrajectoryType* const pType = this->Type;

	if (!this->IsFalling)
	{
		if (pBullet->Location.Z + pBullet->Velocity.Z >= this->Height)
		{
			if (this->ToFalling)
			{
				this->IsFalling = true;
				TechnoClass* const pTechno = pBullet->Owner;
				auto const pExt = BulletExt::ExtMap.Find(pBullet);
				CoordStruct middleLocation = CoordStruct::Empty;

				if (!pType->FreeFallOnTarget)
				{
					middleLocation = CoordStruct
					{
						static_cast<int>(pBullet->Location.X + pBullet->Velocity.X),
						static_cast<int>(pBullet->Location.Y + pBullet->Velocity.Y),
						static_cast<int>(pBullet->Location.Z + pBullet->Velocity.Z)
					};
					pExt->LaserTrails.clear();

					pBullet->Velocity.X = static_cast<double>(pBullet->TargetCoords.X - middleLocation.X);
					pBullet->Velocity.Y = static_cast<double>(pBullet->TargetCoords.Y - middleLocation.Y);
					pBullet->Velocity.Z = static_cast<double>(pBullet->TargetCoords.Z - middleLocation.Z);
					pBullet->Velocity *= this->FallSpeed / pBullet->Velocity.Magnitude();

					this->RemainingDistance += static_cast<int>(pBullet->TargetCoords.DistanceFrom(middleLocation) + this->FallSpeed);
				}
				else
				{
					middleLocation = pBullet->TargetCoords;

					if (this->FallPercent != 1.0)
					{
						pExt->LaserTrails.clear();
						middleLocation.Z += static_cast<int>(pType->Height); // Use original height here
					}
					else
					{
						middleLocation.Z = pBullet->Location.Z;
					}

					pBullet->Velocity = BulletVelocity::Empty;
				}

				pBullet->SetLocation(middleLocation);
				this->ApplyTurningPointAnim(pType->TurningPointAnims, middleLocation, pTechno, pTechno ? pTechno->Owner : pExt->FirerHouse, true);
			}
			else
			{
				this->ToFalling = true;
				pBullet->Velocity *= abs((this->Height - pBullet->Location.Z) / pBullet->Velocity.Z);
			}
		}
		else
		{
			this->AscendTime += 1;
		}
	}
	else if (pType->FreeFallOnTarget)
	{
		pBullet->Velocity.Z -= BulletTypeExt::GetAdjustedGravity(pBullet->Type);
	}
}

void BombardTrajectory::ApplyTurningPointAnim(const std::vector<AnimTypeClass*>& AnimList, CoordStruct coords, TechnoClass* pTechno, HouseClass* pHouse, bool invoker, bool ownedObject)
{
	if (AnimList.empty())
		return;

	auto const pAnimType = AnimList[ScenarioClass::Instance->Random.RandomRanged(0, AnimList.size() - 1)];

	if (!pAnimType)
		return;

	auto const pAnim = GameCreate<AnimClass>(pAnimType, coords);

	if (!pAnim || !pTechno)
		return;

	AnimExt::SetAnimOwnerHouseKind(pAnim, pHouse ? pHouse : pTechno->Owner, nullptr, false, true);

	if (ownedObject)
		pAnim->SetOwnerObject(pTechno);

	if (invoker)
	{
		auto const pAnimExt = AnimExt::ExtMap.Find(pAnim);

		if (!pAnimExt)
			return;

		if (pHouse)
			pAnimExt->SetInvoker(pTechno, pHouse);
		else
			pAnimExt->SetInvoker(pTechno);
	}
}
