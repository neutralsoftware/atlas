#include "atlas/animation.h"
#include <json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {

using Json = nlohmann::json;

constexpr const char *parameterKinds[] = {"float", "bool", "trigger"};
constexpr const char *comparisons[] = {"==", "!=", "<", ">", "<=", ">="};

template <typename Enum, std::size_t N>
Enum parseEnum(const std::string &value, const char *const (&names)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
        if (value == names[i])
            return static_cast<Enum>(i);
    }
    throw std::invalid_argument("Unknown animation graph value: " + value);
}

bool compare(float a, AnimationComparison operation, float b) {
    switch (operation) {
    case AnimationComparison::Equal: return a == b;
    case AnimationComparison::NotEqual: return a != b;
    case AnimationComparison::Less: return a < b;
    case AnimationComparison::Greater: return a > b;
    case AnimationComparison::LessEqual: return a <= b;
    case AnimationComparison::GreaterEqual: return a >= b;
    }
    return false;
}

}

void AnimationStateMachine::validate() const {
    std::unordered_set<std::string> stateNames;
    for (const auto &state : states) {
        if (state.name.empty() || state.clip.empty() ||
            !stateNames.insert(state.name).second ||
            !std::isfinite(state.speed) || state.speed <= 0.0f ||
            !std::isfinite(state.x) || !std::isfinite(state.y))
            throw std::invalid_argument("Invalid or duplicate animation state: " + state.name);
    }
    if (!stateNames.contains(initialState))
        throw std::invalid_argument("Choose an initial animation state");

    std::unordered_map<std::string, AnimationParameterKind> parameterTypes;
    for (const auto &parameter : parameters) {
        if (parameter.name.empty() ||
            !parameterTypes.emplace(parameter.name, parameter.kind).second ||
            static_cast<unsigned>(parameter.kind) >= std::size(parameterKinds) ||
            !std::isfinite(parameter.initialValue) ||
            (parameter.kind != AnimationParameterKind::Float &&
             parameter.initialValue != 0.0f && parameter.initialValue != 1.0f) ||
            (parameter.kind == AnimationParameterKind::Trigger && parameter.initialValue != 0.0f))
            throw std::invalid_argument("Invalid or duplicate animation parameter: " + parameter.name);
    }

    for (const auto &transition : transitions) {
        if (!stateNames.contains(transition.from) || !stateNames.contains(transition.to) ||
            transition.from == transition.to ||
            !std::isfinite(transition.fadeSeconds) || transition.fadeSeconds < 0.0f)
            throw std::invalid_argument("Invalid animation transition: " + transition.from + " -> " + transition.to);

        for (const auto &condition : transition.conditions) {
            const auto parameter = parameterTypes.find(condition.parameter);
            if (parameter == parameterTypes.end() || !std::isfinite(condition.value) ||
                static_cast<unsigned>(condition.comparison) >= std::size(comparisons))
                throw std::invalid_argument("Invalid animation condition: " + condition.parameter);
            if (parameter->second == AnimationParameterKind::Trigger &&
                (condition.comparison != AnimationComparison::Equal || condition.value != 1.0f))
                throw std::invalid_argument("Trigger conditions must compare equal to 1");
            if (parameter->second == AnimationParameterKind::Bool &&
                ((condition.value != 0.0f && condition.value != 1.0f) ||
                 (condition.comparison != AnimationComparison::Equal &&
                  condition.comparison != AnimationComparison::NotEqual)))
                throw std::invalid_argument("Bool conditions must compare equality with 0 or 1");
        }
    }
}

AnimationStateMachine AnimationStateMachine::load(const std::filesystem::path &path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Could not open animation graph: " + path.string());
    const auto json = Json::parse(input);
    if (json.at("version").get<int>() != 1)
        throw std::runtime_error("Unsupported animation graph version");

    AnimationStateMachine machine;
    machine.initialState = json.at("initial").get<std::string>();
    for (const auto &p : json.at("parameters")) {
        machine.parameters.push_back({p.at("name").get<std::string>(),
            parseEnum<AnimationParameterKind>(p.at("kind").get<std::string>(), parameterKinds),
            p.at("initial").get<float>()});
    }
    for (const auto &s : json.at("states")) {
        machine.states.push_back({s.at("name").get<std::string>(),
            s.at("clip").get<std::string>(), s.at("loop").get<bool>(),
            s.at("speed").get<float>(), s.value("x", 0.0f), s.value("y", 0.0f)});
    }
    for (const auto &t : json.at("transitions")) {
        AnimationTransition transition{t.at("from").get<std::string>(),
            t.at("to").get<std::string>(), t.at("fade").get<float>(),
            t.at("waitForEnd").get<bool>(), {}};
        for (const auto &c : t.at("conditions")) {
            transition.conditions.push_back({c.at("parameter").get<std::string>(),
                parseEnum<AnimationComparison>(c.at("comparison").get<std::string>(), comparisons),
                c.at("value").get<float>()});
        }
        machine.transitions.push_back(std::move(transition));
    }
    machine.validate();
    return machine;
}

void AnimationStateMachine::save(const std::filesystem::path &path) const {
    validate();
    Json json{{"version", 1}, {"initial", initialState},
              {"parameters", Json::array()}, {"states", Json::array()},
              {"transitions", Json::array()}};
    for (const auto &p : parameters) {
        json["parameters"].push_back({{"name", p.name},
            {"kind", parameterKinds[static_cast<unsigned>(p.kind)]}, {"initial", p.initialValue}});
    }
    for (const auto &s : states) {
        json["states"].push_back({{"name", s.name}, {"clip", s.clip}, {"loop", s.loop},
                                  {"speed", s.speed}, {"x", s.x}, {"y", s.y}});
    }
    for (const auto &t : transitions) {
        Json conditions = Json::array();
        for (const auto &c : t.conditions) {
            conditions.push_back({{"parameter", c.parameter},
                {"comparison", comparisons[static_cast<unsigned>(c.comparison)]}, {"value", c.value}});
        }
        json["transitions"].push_back({{"from", t.from}, {"to", t.to}, {"fade", t.fadeSeconds},
            {"waitForEnd", t.waitForEnd}, {"conditions", std::move(conditions)}});
    }
    std::ofstream output(path, std::ios::trunc);
    output << json.dump(2) << '\n';
    output.close();
    if (!output)
        throw std::runtime_error("Could not save animation graph: " + path.string());
}

void Animator::setStateMachine(AnimationStateMachine graph) {
    graph.validate();
    for (const auto &state : graph.states) {
        if (!findClip(state.clip))
            throw std::invalid_argument("Animation state references unknown clip: " + state.clip);
    }
    machine = std::move(graph);
    parameterValues.clear();
    for (const auto &parameter : machine.parameters)
        parameterValues.emplace(parameter.name, parameter.initialValue);

    const auto initial = std::ranges::find(machine.states, machine.initialState, &AnimationState::name);
    play(initial->clip, initial->loop, initial->speed);
    activeState = initial->name;
}

bool Animator::setParameter(std::string_view name, AnimationParameterKind kind, float value) {
    const auto parameter = std::ranges::find_if(machine.parameters,
        [&](const auto &p) { return p.name == name && p.kind == kind; });
    if (parameter == machine.parameters.end() || !std::isfinite(value))
        return false;
    parameterValues.at(parameter->name) = value;
    return true;
}

bool Animator::setFloat(std::string_view parameter, float value) {
    return setParameter(parameter, AnimationParameterKind::Float, value);
}

bool Animator::setBool(std::string_view parameter, bool value) {
    return setParameter(parameter, AnimationParameterKind::Bool, value ? 1.0f : 0.0f);
}

bool Animator::trigger(std::string_view parameter) {
    return setParameter(parameter, AnimationParameterKind::Trigger, 1.0f);
}

void Animator::updateStateMachine() {
    if (activeState.empty() || fadeDuration > 0.0f)
        return;
    for (const auto &transition : machine.transitions) {
        if (transition.from != activeState || (transition.waitForEnd && !isFinished()))
            continue;
        if (!std::ranges::all_of(transition.conditions, [&](const auto &condition) {
                return compare(parameterValues.at(condition.parameter),
                               condition.comparison, condition.value);
            }))
            continue;

        const auto target = std::ranges::find(machine.states, transition.to, &AnimationState::name);
        crossfade(target->clip, transition.fadeSeconds, target->loop, target->speed);
        activeState = target->name;
        for (const auto &condition : transition.conditions) {
            const auto parameter = std::ranges::find(machine.parameters, condition.parameter,
                                                      &AnimationParameter::name);
            if (parameter->kind == AnimationParameterKind::Trigger)
                parameterValues.at(parameter->name) = 0.0f;
        }
        return;
    }
}
