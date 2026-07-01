#include "RLDamageEffect.h"
#include "RLDamageExecution.h"

URLDamageEffect::URLDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecutionDef;
	ExecutionDef.CalculationClass = URLDamageExecution::StaticClass();
	Executions.Add(ExecutionDef);
}
