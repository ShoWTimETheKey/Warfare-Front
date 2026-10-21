#pragma once

class AWarfareFrontlineGameMode;
class AWarfareFrontlineController;

// Explicit, bounded packaged-game check; compiled in Shipping as well.
void StartFrontlineDistributionReview(AWarfareFrontlineGameMode* Battle);
// Disposable host-start fixture. It deliberately does not impersonate a real
// selector press or relax its hold-confirmation/authentic-input requirements.
bool StartDistributionCampaignFixtureV8222(AWarfareFrontlineController* Controller);
