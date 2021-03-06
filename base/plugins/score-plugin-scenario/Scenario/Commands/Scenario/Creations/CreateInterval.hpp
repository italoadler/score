#pragma once
#include <QString>
#include <Scenario/Commands/ScenarioCommandFactory.hpp>
#include <score/command/Command.hpp>
#include <score/model/path/Path.hpp>
#include <score/model/Identifier.hpp>
#include <score/tools/std/Optional.hpp>

#include <score_plugin_scenario_export.h>
struct DataStreamInput;
struct DataStreamOutput;

namespace Scenario
{
class StateModel;
class IntervalModel;
class ProcessModel;

namespace Command
{
/**
        * @brief The CreateEventAfterEventCommand class
        *
        * This Command creates a interval and another event in a scenario,
        * starting from an event selected by the user.
        */
class SCORE_PLUGIN_SCENARIO_EXPORT CreateInterval final
    : public score::Command
{
  SCORE_COMMAND_DECL(
      ScenarioCommandFactoryName(), CreateInterval, "Create an interval")
public:
  CreateInterval(
      const Scenario::ProcessModel& scenarioPath,
      Id<StateModel>
          startState,
      Id<StateModel>
          endState);
  CreateInterval& operator=(CreateInterval&&) = default;

  const Path<Scenario::ProcessModel>& scenarioPath() const
  {
    return m_path;
  }

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

  const Id<IntervalModel>& createdInterval() const
  {
    return m_createdIntervalId;
  }

  const Id<StateModel>& startState() const
  {
    return m_startStateId;
  }

protected:
  void serializeImpl(DataStreamInput&) const override;
  void deserializeImpl(DataStreamOutput&) override;

private:
  Path<Scenario::ProcessModel> m_path;
  QString m_createdName;

  Id<IntervalModel> m_createdIntervalId{};

  Id<StateModel> m_startStateId{};
  Id<StateModel> m_endStateId{};
  double m_startStatePos{-1};
  double m_endStatePos{-1};
};
}
}
