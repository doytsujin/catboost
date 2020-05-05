#pragma once

#include "enums.h"
#include "option.h"

#include <util/generic/cast.h>
#include <util/generic/map.h>
#include <util/generic/xrange.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/system/types.h>


namespace NJson {
    class TJsonValue;
}


struct TLossParams {
    TMap<TString, TString> paramsMap;
    TVector<TString> userSpecifiedKeyOrder;
};

ELossFunction ParseLossType(TStringBuf lossDescription);

TLossParams ParseLossParams(TStringBuf lossDescription);
namespace NCatboostOptions {
    const int MAX_AUTOGENERATED_PAIRS_COUNT = Max<int>();

    class TLossDescription {
    public:
        explicit TLossDescription();

        ELossFunction GetLossFunction() const;

        void Load(const NJson::TJsonValue& options);
        void Save(NJson::TJsonValue* options) const;

        bool operator==(const TLossDescription& rhs) const;
        bool operator!=(const TLossDescription& rhs) const;

        const TMap<TString, TString>& GetLossParamsMap() const;
        TLossParams GetLossParams() const;

    public:
        TOption<ELossFunction> LossFunction;
        TOption<TMap<TString, TString>> LossParams;
    };

    template <typename T>
    T GetParamOrDefault(const TMap<TString, TString>& lossParams, const TString& paramName, T defaultValue) {
        if (lossParams.contains(paramName)) {
            return FromString<T>(lossParams.at(paramName));
        }
        return defaultValue;
    }

    template <typename T>
    T GetParamOrDefault(const TLossDescription& lossFunctionConfig, const TString& paramName, T defaultValue) {
        return GetParamOrDefault(lossFunctionConfig.GetLossParamsMap(), paramName, defaultValue);
    }

    double GetLogLossBorder(const TLossDescription& lossFunctionConfig);

    double GetAlpha(const TMap<TString, TString>& lossParams);
    double GetAlpha(const TLossDescription& lossFunctionConfig);

    double GetAlphaQueryCrossEntropy(const TMap<TString, TString>& lossParams);
    double GetAlphaQueryCrossEntropy(const TLossDescription& lossFunctionConfig);

    int GetYetiRankPermutations(const TLossDescription& lossFunctionConfig);

    double GetYetiRankDecay(const TLossDescription& lossFunctionConfig);

    double GetLqParam(const TLossDescription& lossFunctionConfig);

    double GetHuberParam(const TLossDescription& lossFunctionConfig);

    double GetQuerySoftMaxLambdaReg(const TLossDescription& lossFunctionConfig);

    ui32 GetMaxPairCount(const TLossDescription& lossFunctionConfig);

    double GetStochasticFilterSigma(const TLossDescription& lossDescription);

    int GetStochasticFilterNumEstimations(const TLossDescription& lossDescription);

    double GetTweedieParam(const TLossDescription& lossFunctionConfig);

    TLossDescription ParseLossDescription(TStringBuf stringLossDescription);
}

void ValidateHints(const TMap<TString, TString>& hints);

TMap<TString, TString> ParseHintsDescription(TStringBuf hintsDescription);
TString MakeHintsDescription(const TMap<TString, TString>& hints);

NJson::TJsonValue LossDescriptionToJson(TStringBuf lossDescription);
TString BuildMetricOptionDescription(const NJson::TJsonValue& lossOptions);

void CheckMetric(const ELossFunction metric, const ELossFunction modelLoss);

ELossFunction GetMetricFromCombination(const TMap<TString, TString>& params);

void CheckCombinationParameters(const TMap<TString, TString>& params);

TString GetCombinationLossKey(ui32 idx);

TString GetCombinationWeightKey(ui32 idx);

template <typename TCallable>
void IterateOverCombination(const TMap<TString, TString>& params, const TCallable callable)  {
    const ui32 lossCount = params.size() / 2;
    CB_ENSURE(lossCount > 1, "Combination loss must have two or more parameters");
    for (ui32 idx : xrange(lossCount)) {
        const auto& lossKey = GetCombinationLossKey(idx);
        const auto& weightKey = GetCombinationWeightKey(idx);
        CB_ENSURE(params.contains(lossKey) && params.contains(weightKey), "Mandatory parameter " << lossKey << " or " << weightKey << " is missing");
        float weight;
        CB_ENSURE(TryFromString<float>(params.at(weightKey), weight), "Value of " << weightKey << " must be floating point number");
        if (weight == 0.0f) {
            continue;
        }
        CB_ENSURE(weight > 0.0f, "Value of " << weightKey << " must be positive, not " << weight);
        const auto& loss = NCatboostOptions::ParseLossDescription(params.at(lossKey));
        callable(loss, weight);
    }
}
