#include "Core/LCGameState.h"
#include "Net/UnrealNetwork.h"

ALCGameState::ALCGameState()
{
}

void ALCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALCGameState, CurrentPhase);
    DOREPLIFETIME(ALCGameState, CurrentWave);
    DOREPLIFETIME(ALCGameState, AliveCount);
    DOREPLIFETIME(ALCGameState, ZoneRadius);
    DOREPLIFETIME(ALCGameState, ZoneCenter);
    DOREPLIFETIME(ALCGameState, RestTimeRemaining);
    DOREPLIFETIME(ALCGameState, CurrentWeather);
    DOREPLIFETIME(ALCGameState, DayNightTime);
    DOREPLIFETIME(ALCGameState, TotalKills);
}
