#pragma once
class AWarfareFrontlineSoldier;
class UMaterialInterface;
class UWorld;
class USkeletalMesh;
namespace FrontlineSoldierSurfaceV62
{
    // Candidate is opt-in until the fixed-camera review is accepted.
    UMaterialInterface* Resolve(UMaterialInterface* Source);
    // Opt-in geometry correction; the accepted source remains the fallback.
    USkeletalMesh* ResolveMesh(USkeletalMesh* Source);
    // Opt-in read-only asset/render-buffer evidence for the existing fixtures.
    void ScheduleAudit(AWarfareFrontlineSoldier* Unit);
    // Only the isolated review calls this temporary texture-residency request.
    bool PrepareReview(UWorld* World);
    // Synchronously observes the assigned render proxies before a review capture.
    bool WriteAudit(UWorld* World,const TCHAR* Label);
}
