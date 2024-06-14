#pragma once

#include "PhobosTrajectory.h"

class StraightTrajectoryType final : public PhobosTrajectoryType
{
public:
	StraightTrajectoryType() : PhobosTrajectoryType(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDamage { 0 }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer { 0 }
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord { { 0, 0, 0 } }
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation { { 0, 0, 1 } }
		, ProximityImpact { 0 }
		, ProximityDamage { 0 }
		, ProximityRadius { Leptons(179) }
		, ProximityAllies { 0.0 }
		, ProximityFlight { false }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
	{}

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;

	virtual void Read(CCINIClass* const pINI, const char* pSection) override;

	Valueable<Leptons> DetonationDistance;
	Valueable<Leptons> TargetSnapDistance;
	Valueable<bool> PassThrough;
	Valueable<bool> PassDetonate;
	Valueable<int> PassDetonateDamage;
	Valueable<int> PassDetonateDelay;
	Valueable<int> PassDetonateTimer;
	Valueable<bool> PassDetonateLocal;
	Valueable<bool> LeadTimeCalculate;
	Valueable<CoordStruct> OffsetCoord;
	Valueable<double> RotateCoord;
	Valueable<bool> MirrorCoord;
	Valueable<bool> UseDisperseBurst;
	Valueable<CoordStruct> AxisOfRotation;
	Valueable<int> ProximityImpact;
	Valueable<int> ProximityDamage;
	Valueable<Leptons> ProximityRadius;
	Valueable<double> ProximityAllies;
	Valueable<bool> ProximityFlight;
	Valueable<bool> ThroughVehicles;
	Valueable<bool> ThroughBuilding;
	Valueable<bool> SubjectToGround;
	Valueable<int> ConfineAtHeight;
	Valueable<double> EdgeAttenuation;
};

class StraightTrajectory final : public PhobosTrajectory
{
public:
	StraightTrajectory() : PhobosTrajectory(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDamage { 0 }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer {}
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord {}
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation {}
		, ProximityImpact { 0 }
		, ProximityDamage { 0 }
		, ProximityRadius { Leptons(179) }
		, ProximityAllies { 0.0 }
		, ProximityFlight { false }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
		, RemainingDistance { 1 }
		, ExtraCheck { nullptr }
		, LastCasualty {}
		, FirepowerMult { 1.0 }
		, LastTargetCoord {}
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }
		, WaitOneFrame {}
	{}

	StraightTrajectory(PhobosTrajectoryType* pType) : PhobosTrajectory(TrajectoryFlag::Straight)
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
		, PassThrough { false }
		, PassDetonate { false }
		, PassDetonateDamage { 0 }
		, PassDetonateDelay { 1 }
		, PassDetonateTimer {}
		, PassDetonateLocal { false }
		, LeadTimeCalculate { false }
		, OffsetCoord {}
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation {}
		, ProximityImpact { 0 }
		, ProximityDamage { 0 }
		, ProximityRadius { Leptons(179) }
		, ProximityAllies { 0.0 }
		, ProximityFlight { false }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, SubjectToGround { false }
		, ConfineAtHeight { 0 }
		, EdgeAttenuation { 1.0 }
		, RemainingDistance { 1 }
		, ExtraCheck { nullptr }
		, LastCasualty {}
		, FirepowerMult { 1.0 }
		, LastTargetCoord {}
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }
		, WaitOneFrame {}
	{}

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;

	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) override;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) override;

	struct CasualtyData
	{
		TechnoClass* pCasualty;
		int RemainTime;
	};

	Leptons DetonationDistance;
	Leptons TargetSnapDistance;
	bool PassThrough;
	bool PassDetonate;
	int PassDetonateDamage;
	int PassDetonateDelay;
	CDTimerClass PassDetonateTimer;
	bool PassDetonateLocal;
	bool LeadTimeCalculate;
	CoordStruct OffsetCoord;
	double RotateCoord;
	bool MirrorCoord;
	bool UseDisperseBurst;
	CoordStruct AxisOfRotation;
	int ProximityImpact;
	int ProximityDamage;
	Leptons ProximityRadius;
	double ProximityAllies;
	bool ProximityFlight;
	bool ThroughVehicles;
	bool ThroughBuilding;
	bool SubjectToGround;
	int ConfineAtHeight;
	double EdgeAttenuation;
	int RemainingDistance;
	TechnoClass* ExtraCheck;
	std::vector<CasualtyData> LastCasualty;
	double FirepowerMult;
	CoordStruct LastTargetCoord;
	int CurrentBurst;
	int CountOfBurst;
	CDTimerClass WaitOneFrame;

private:
	void PrepareForOpenFire(BulletClass* pBullet);
	int GetVelocityZ(BulletClass* pBullet);
	bool CalculateBulletVelocity(BulletClass* pBullet, double StraightSpeed);
	bool BulletPrepareCheck(BulletClass* pBullet);
	bool BulletDetonatePreCheck(BulletClass* pBullet, HouseClass* pOwner, double StraightSpeed);
	void BulletDetonateLastCheck(BulletClass* pBullet, HouseClass* pOwner, double StraightSpeed);
	bool CheckThroughAndSubjectInCell(BulletClass* pBullet, CellClass* pCell, HouseClass* pOwner);
	void PassWithDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	void PrepareForDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	std::vector<CellClass*> GetCellsInProximityRadius(BulletClass* pBullet);
	std::vector<CellStruct> GetCellsInRectangle(CellStruct bStaCell, CellStruct lMidCell, CellStruct rMidCell, CellStruct tEndCell);
	int GetTheTrueDamage(int Damage, BulletClass* pBullet, TechnoClass* pTechno, HouseClass* pOwner, bool Self);
	double GetExtraDamageMultiplier(BulletClass* pBullet, TechnoClass* pTechno, HouseClass* pOwner, bool Self);
	bool PassAndConfineAtHeight(BulletClass* pBullet, double StraightSpeed);
};
