// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Process/LayerPresenter.hpp>
#include <Process/Process.hpp>

#include <Scenario/Application/ScenarioApplicationPlugin.hpp>
#include <Scenario/Commands/Interval/AddLayerInNewSlot.hpp>
#include <Scenario/Commands/Interval/AddOnlyProcessToInterval.hpp>
#include <Scenario/Commands/Interval/CreateProcessInExistingSlot.hpp>
#include <Scenario/Commands/Interval/CreateProcessInNewSlot.hpp>
#include <Scenario/Commands/Interval/Rack/AddSlotToRack.hpp>
#include <Scenario/Commands/Interval/Rack/RemoveSlotFromRack.hpp>
#include <Scenario/Commands/Interval/Rack/Slot/AddLayerModelToSlot.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateCommentBlock.hpp>
#include <Scenario/DialogWidget/AddProcessDialog.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Interval/Slot.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/Temporal/TemporalScenarioPresenter.hpp>
#include <Scenario/Process/Temporal/TemporalScenarioView.hpp>
#include <Scenario/ViewCommands/PutLayerModelToFront.hpp>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/multi_index/detail/hash_index_iterator.hpp>
#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/command/Dispatchers/MacroCommandDispatcher.hpp>
#include <score/selection/Selection.hpp>
#include <score/selection/SelectionStack.hpp>
#include <score/tools/std/Optional.hpp>

#include <QPoint>
#include <QString>
#include <algorithm>

#include "ScenarioContextMenuManager.hpp"
#include <Process/ProcessList.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#include <score/model/EntityMap.hpp>
#include <score/model/Identifier.hpp>
#include <Scenario/Document/Interval/FullView/FullViewIntervalPresenter.hpp>

namespace Scenario
{
void ScenarioContextMenuManager::createSlotContextMenu(
    const score::DocumentContext& ctx,
    QMenu& menu,
    const TemporalIntervalPresenter& pres,
    int slot_index)
{
  using namespace Scenario::Command;
  // TODO see
  // http://stackoverflow.com/questions/21443023/capturing-a-reference-by-reference-in-a-c11-lambda
  auto& interval = pres.model();
  const Slot& slot = interval.smallView().at(slot_index);
  SlotPath slot_path{interval, slot_index, Slot::SmallView};

  // Then creation of a new slot with existing processes
  auto new_processes_submenu = menu.addMenu(tr("Show process in new slot"));
  for (const Id<Process::ProcessModel>& proc : slot.processes)
  {
    auto& p = interval.processes.at(proc);
    auto name = p.prettyName();
    if(name.isEmpty())
      name = p.prettyShortName();
    QAction* procAct
        = new QAction{name, new_processes_submenu};
    QObject::connect(procAct, &QAction::triggered, [&]() {
      auto cmd = new Scenario::Command::AddLayerInNewSlot{
          interval, p.id()};
      CommandDispatcher<>{ctx.commandStack}.submitCommand(cmd);
    });
    new_processes_submenu->addAction(procAct);
  }

  // Then removal of slot
  auto removeSlotAct = new QAction{tr("Remove this slot"), nullptr};
  QObject::connect(removeSlotAct, &QAction::triggered, [&,slot_path]() {
    auto cmd = new Scenario::Command::RemoveSlotFromRack{slot_path, slot_path.find(ctx)};
    CommandDispatcher<>{ctx.commandStack}.submitCommand(cmd);
  });
  menu.addAction(removeSlotAct);

  menu.addSeparator();

  // Then Add process in this slot
  auto existing_processes_submenu
      = menu.addMenu(tr("Add existing process in this slot"));
  for (const Process::ProcessModel& p : interval.processes)
  {
    // OPTIMIZEME by filtering before.
    if (ossia::none_of(slot.processes,
            [&] (const Id<Process::ProcessModel>& layer) {
              return layer == p.id();
            }))
    {
      auto name = p.prettyName();
      if(name.isEmpty())
        name = p.prettyShortName();
      QAction* procAct
          = new QAction{name, existing_processes_submenu};
      QObject::connect(procAct, &QAction::triggered, [&, slot_path] () mutable {
        auto cmd2 = new Scenario::Command::AddLayerModelToSlot{std::move(slot_path), p};
        CommandDispatcher<>{ctx.commandStack}.submitCommand(cmd2);
      });
      existing_processes_submenu->addAction(procAct);
    }
  }

  auto addNewProcessInExistingSlot
      = new QAction{tr("Add new process in this slot"), &menu};
  QObject::connect(addNewProcessInExistingSlot, &QAction::triggered, [&, slot_path]() {
    auto& fact = ctx.app.interfaces<Process::ProcessFactoryList>();
    AddProcessDialog* dialog =new AddProcessDialog{fact, qApp->activeWindow()};

    QObject::connect(
        dialog, &AddProcessDialog::okPressed, [&, slot_path] (const auto& proc) mutable {
          QuietMacroCommandDispatcher<Scenario::Command::CreateProcessInExistingSlot>
              disp{ctx.commandStack};

          auto cmd1 = new Scenario::Command::AddOnlyProcessToInterval(
              interval, proc);
          cmd1->redo(ctx);
          disp.submitCommand(cmd1);

          auto cmd2 = new Scenario::Command::AddLayerModelToSlot(
              std::move(slot_path), interval.processes.at(cmd1->processId()));
          cmd2->redo(ctx);
          disp.submitCommand(cmd2);

          disp.commit();
        });

    dialog->launchWindow();
    dialog->deleteLater();
  });
  menu.addAction(addNewProcessInExistingSlot);

  // Then Add process in a new slot
  auto addNewProcessInNewSlot
      = new QAction{tr("Add process in a new slot"), &menu};
  QObject::connect(addNewProcessInNewSlot, &QAction::triggered, [&]() {
    auto& fact = ctx.app.interfaces<Process::ProcessFactoryList>();
    AddProcessDialog* dialog = new AddProcessDialog{fact, qApp->activeWindow()};

    QObject::connect(
        dialog, &AddProcessDialog::okPressed, [&](const auto& proc) {
          using cmd = Scenario::Command::CreateProcessInNewSlot;
          QuietMacroCommandDispatcher<cmd> disp{ctx.commandStack};

          cmd::create(disp, interval, proc);

          disp.commit();
        });

    dialog->launchWindow();
    dialog->deleteLater();
  });
  menu.addAction(addNewProcessInNewSlot);
}

void ScenarioContextMenuManager::createSlotContextMenu(
    const score::DocumentContext& docContext,
    QMenu& menu,
    const FullViewIntervalPresenter& slotp,
    int slot_index)
{

}

void ScenarioContextMenuManager::createProcessSelectorContextMenu(
    const score::DocumentContext& ctx,
    QMenu& menu,
    const TemporalIntervalPresenter& pres,
    int slot_index)
{
  using namespace Scenario::Command;
  // TODO see
  // http://stackoverflow.com/questions/21443023/capturing-a-reference-by-reference-in-a-c11-lambda
  auto& interval = pres.model();
  const Slot& slot = interval.smallView().at(slot_index);
  SlotPath slot_path{interval, slot_index, Slot::SmallView};

  for (const Id<Process::ProcessModel>& proc : slot.processes)
  {
    auto& p = interval.processes.at(proc);
    auto name = p.prettyName();
    if(name.isEmpty())
      name = p.prettyShortName();
    QAction* procAct
        = new QAction{name, &menu};
    QObject::connect(procAct, &QAction::triggered, [&,slot_path] () mutable {
      PutLayerModelToFront cmd{std::move(slot_path), p.id()};
      cmd.redo(ctx);
    });
    menu.addAction(procAct);
  }
}


void ScenarioContextMenuManager::createLayerContextMenu(
    QMenu& menu,
    QPoint pos,
    QPointF scenepos,
    const Process::LayerContextMenuManager& lcmmgr,
    Process::LayerPresenter& pres)
{
  using namespace score;

  bool has_slot_menu = false;

  // Fill with slot actions
  if(auto small_view = dynamic_cast<TemporalIntervalPresenter*>(pres.parent()))
  {
    auto& context = pres.context().context;
    //if (context.selectionStack.currentSelection().toList().isEmpty())
    {
      auto slotSubmenu = menu.addMenu(tr("Slot"));
      ScenarioContextMenuManager::createSlotContextMenu(
          context, *slotSubmenu, *small_view, small_view->indexOfSlot(pres));
      has_slot_menu = true;
    }
  }

  // Then the process-specific part
  if (has_slot_menu)
  {
    auto processMenu = menu.addMenu(pres.model().prettyShortName());
    pres.fillContextMenu(*processMenu, pos, scenepos, lcmmgr);
  }
  else
  {
    pres.fillContextMenu(menu, pos, scenepos, lcmmgr);
  }
}

void ScenarioContextMenuManager::createLayerContextMenuForProcess(
    QMenu& menu,
    QPoint pos,
    QPointF scenepos,
    const Process::LayerContextMenuManager& lcmmgr,
    Process::LayerPresenter& pres)
{
  using namespace score;

  bool has_slot_menu = false;

  // Fill with slot actions
  if(auto small_view = dynamic_cast<TemporalIntervalPresenter*>(pres.parent()))
  {
    auto& context = pres.context().context;
    //if (context.selectionStack.currentSelection().toList().isEmpty())
    {
      // auto slotSubmenu = menu.addMenu(tr("Slot"));
      ScenarioContextMenuManager::createProcessSelectorContextMenu(
          context, menu, *small_view, small_view->indexOfSlot(pres));
      has_slot_menu = true;
    }
  }
}
}
