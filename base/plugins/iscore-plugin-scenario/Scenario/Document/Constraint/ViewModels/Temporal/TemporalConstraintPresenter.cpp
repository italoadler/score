#include <Process/ProcessList.hpp>
#include <QGraphicsScene>
#include <QList>
#include <QApplication>
#include <Scenario/Commands/Constraint/AddProcessToConstraint.hpp>
#include <Scenario/DialogWidget/AddProcessDialog.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Constraint/ViewModels/Temporal/TemporalConstraintView.hpp>
#include <Scenario/Document/Constraint/ViewModels/Temporal/TemporalConstraintViewModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentPresenter.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <Scenario/Document/Constraint/Rack/Slot/SlotHandle.hpp>

#include <Scenario/Document/Constraint/Rack/RackPresenter.hpp>
#include <Scenario/Document/Constraint/Rack/RackView.hpp>
#include <Scenario/Document/Constraint/Rack/RackModel.hpp>

#include "TemporalConstraintHeader.hpp"
#include "TemporalConstraintPresenter.hpp"
#include "TemporalConstraintView.hpp"
#include <Process/ProcessContext.hpp>
#include <Process/LayerPresenter.hpp>
#include <Process/LayerView.hpp>
#include <Scenario/Application/Drops/ScenarioDropHandler.hpp>
#include <Scenario/Document/Constraint/ViewModels/ConstraintHeader.hpp>
#include <Scenario/Document/Constraint/ViewModels/ConstraintPresenter.hpp>
#include <Scenario/Document/Constraint/ViewModels/ConstraintViewModel.hpp>
#include <iscore/model/ModelMetadata.hpp>
#include <iscore/selection/Selectable.hpp>
#include <iscore/tools/Todo.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <iscore/widgets/GraphicsItem.hpp>
class QColor;
class QObject;
class QString;

namespace Scenario
{
TemporalConstraintPresenter::TemporalConstraintPresenter(
    const ConstraintModel& cstr_model,
    const Process::ProcessPresenterContext& ctx,
    QGraphicsItem* parentobject,
    QObject* parent)
  : ConstraintPresenter{cstr_model,
                        new TemporalConstraintView{*this, parentobject},
                        new TemporalConstraintHeader, ctx, parent}
{
  TemporalConstraintView& v = *Scenario::view(this);
  con(v, &TemporalConstraintView::constraintHoverEnter, this,
      &TemporalConstraintPresenter::constraintHoverEnter);

  con(v, &TemporalConstraintView::constraintHoverLeave, this,
      &TemporalConstraintPresenter::constraintHoverLeave);

  con(v, &ConstraintView::requestOverlayMenu,
      this, &TemporalConstraintPresenter::on_requestOverlayMenu);

  con(cstr_model, &ConstraintModel::executionStateChanged, &v,
      &TemporalConstraintView::setExecutionState);

  const auto& metadata = m_model.metadata();
  con(metadata, &iscore::ModelMetadata::LabelChanged, &v,
      &TemporalConstraintView::setLabel);

  con(metadata, &iscore::ModelMetadata::ColorChanged, &v,
      [&](iscore::ColorRef c) {
    v.setLabelColor(c);
    v.setColor(c);
  });

  con(metadata, &iscore::ModelMetadata::NameChanged, this,
      [&](const QString& name) { m_header->setText(name); });

  v.setLabel(metadata.getLabel());
  v.setLabelColor(metadata.getColor());
  v.setColor(metadata.getColor());
  m_header->setText(metadata.getName());
  v.setExecutionState(m_model.executionState());

  con(m_model.selection, &Selectable::changed, &v,
      &TemporalConstraintView::setFocused);
  con(m_model, &ConstraintModel::focusChanged, &v,
      &TemporalConstraintView::setFocused);

  // Drop
  con(v, &TemporalConstraintView::dropReceived, this,
      [=](const QPointF& pos, const QMimeData* mime) {
    m_context.app.interfaces<Scenario::ConstraintDropHandlerList>()
        .drop(m_model, mime);
  });

  // Header set-up
  auto header = static_cast<TemporalConstraintHeader*>(m_header);

  connect(
        header, &TemporalConstraintHeader::constraintHoverEnter, this,
        &TemporalConstraintPresenter::constraintHoverEnter);
  connect(
        header, &TemporalConstraintHeader::constraintHoverLeave, this,
        &TemporalConstraintPresenter::constraintHoverLeave);
  connect(
        header, &TemporalConstraintHeader::shadowChanged, &v,
        &TemporalConstraintView::setShadow);

  connect(
        header, &TemporalConstraintHeader::dropReceived, this,
        [=](const QPointF& pos, const QMimeData* mime) {
    m_context.app.interfaces<Scenario::ConstraintDropHandlerList>()
        .drop(m_model, mime);
  });

  // Go to full-view on double click
  connect(header, &TemporalConstraintHeader::doubleClicked, this, [this]() {
    using namespace iscore::IDocument;
    ScenarioDocumentPresenter& base
        = get<ScenarioDocumentPresenter>(*documentFromObject(m_model));

    base.setDisplayedConstraint(const_cast<ConstraintModel&>(m_model));
  });


  con(m_model, &ConstraintModel::smallViewVisibleChanged, this,
      &TemporalConstraintPresenter::on_rackVisibleChanged);


  con(m_model, &ConstraintModel::rackChanged,
      this, [=] (Slot::RackView t) {
    if(t == Slot::SmallView)
    {
      on_rackChanged();
    }
  });
  con(m_model, &ConstraintModel::slotAdded,
      this, [=] (const SlotId& s) {
    if(s.smallView())
      createSlot(s.index, m_model.smallView()[s.index]);
  });

  con(m_model, &ConstraintModel::slotRemoved,
      this, [=] (const SlotId& s) {
    if(s.smallView())
      on_slotRemoved(s.index);
  });

  con(m_model, &ConstraintModel::slotResized,
          this, [this] (const SlotId& s) {
    if(s.smallView())
      this->updatePositions();
  });

  con(m_model, &ConstraintModel::layerAdded,
      this, [=] (SlotId s, Id<Process::ProcessModel> proc) {
    if(s.smallView())
      createLayer(s.index, m_model.processes.at(proc));
  });
  con(m_model, &ConstraintModel::layerRemoved,
      this, [=] (SlotId s, Id<Process::ProcessModel> proc) {
    if(s.smallView())
      removeLayer(m_model.processes.at(proc));
  });
  con(m_model, &ConstraintModel::frontLayerChanged,
      this, [=] (int pos, OptionalId<Process::ProcessModel> proc) {

    if(proc)
      on_layerModelPutToFront(pos, m_model.processes.at(*proc));
    // TODO else
  });

  on_rackVisibleChanged(m_model.smallViewVisible());
}

TemporalConstraintPresenter::~TemporalConstraintPresenter()
{
  auto view = Scenario::view(this);
  // TODO deleteGraphicsObject
  if (view)
  {
    auto sc = view->scene();

    if (sc && sc->items().contains(view))
    {
      sc->removeItem(view);
    }

    view->deleteLater();
  }

}

void TemporalConstraintPresenter::on_requestOverlayMenu(QPointF)
{
  auto& fact = m_context.app.interfaces<Process::ProcessFactoryList>();
  auto dialog = new AddProcessDialog{fact, QApplication::activeWindow()};

  connect(
        dialog, &AddProcessDialog::okPressed, this,
        [&] (const auto& key) {
    auto cmd
        = Scenario::Command::make_AddProcessToConstraint(this->model(), key);

    CommandDispatcher<> d{m_context.commandStack};
    emit d.submitCommand(cmd);
  });

  dialog->launchWindow();
  dialog->deleteLater();
}

double TemporalConstraintPresenter::rackHeight() const
{
  qreal height = 0;
  for(const auto& slot : m_model.smallView())
  {
    height += slot.height + SlotHandle::handleHeight();
  }
  return height;
}

void TemporalConstraintPresenter::updateHeight()
{
  if (m_model.smallViewVisible())
  {
    m_view->setHeight(rackHeight() + ConstraintHeader::headerHeight());
  }
  else if (!m_model.smallViewVisible() && !m_model.processes.empty())
  {
    m_view->setHeight(ConstraintHeader::headerHeight());
  }
  else
  {
    m_view->setHeight(8);
  }

  updateChildren();
  emit heightChanged();

}

void TemporalConstraintPresenter::on_rackVisibleChanged(bool b)
{
  m_header->setVisible(b);
  on_rackChanged();
}

void TemporalConstraintPresenter::on_processesChanged(const Process::ProcessModel&)
{
  // TODO
}




void TemporalConstraintPresenter::createSlot(int pos, const Slot& slt)
{
  SlotPresenter p;
  p.handle = new SlotHandle{*this, pos, m_view};
  // p.view = new SlotView{};
  m_slots.insert(m_slots.begin() + pos, std::move(p));

  for(const auto& process : slt.processes)
  {
    createLayer(pos, m_model.processes.at(process));
  }
}

void TemporalConstraintPresenter::createLayer(int slot, const Process::ProcessModel& proc)
{
  const auto& procKey = proc.concreteKey();

  auto factory = m_context.processList.findDefaultFactory(procKey);
  auto proc_view = factory->makeLayerView(proc, m_view);
  auto proc_pres = factory->makeLayerPresenter(proc, proc_view, m_context, this);
  m_slots.at(slot).processes.push_back(LayerData{
                  &proc, proc_pres, proc_view
                });

  auto con_id = con(
        proc, &Process::ProcessModel::durationChanged, this,
        [&] (const TimeVal&) {
    int i = 0;
    for(const SlotPresenter& slot : m_slots)
    {
      auto it = ossia::find_if(slot.processes,
                               [&] (const LayerData& elt) {
        return elt.model->id() == proc.id();
      });

      if (it != slot.processes.end())
        updateProcessShape(i, *it);
      i++;
    }
  });

  con(proc, &IdentifiedObjectAbstract::identified_object_destroying, this,
      [=] { QObject::disconnect(con_id); });

  auto frontLayer = m_model.smallView().at(slot).frontProcess;
  if (frontLayer && (*frontLayer == proc.id()))
  {
    on_layerModelPutToFront(slot, proc);
  }
  else
  {
    on_layerModelPutToBack(slot, proc);
  }

  updatePositions();
}

void TemporalConstraintPresenter::updateProcessShape(int slot, const LayerData& data)
{
  data.presenter->setHeight(m_model.smallView().at(slot).height - SlotHandle::handleHeight());

  auto width = m_model.duration.defaultDuration().toPixels(m_zoomRatio);
  data.presenter->setWidth(width);
  data.presenter->parentGeometryChanged();
  data.view->update();
}

void TemporalConstraintPresenter::removeLayer(const Process::ProcessModel& proc)
{
  for(SlotPresenter& slot : m_slots)
  {
    boost::range::remove_erase_if(slot.processes, [&] (const LayerData& elt) {
      bool to_delete = elt.model->id() == proc.id();

      if (to_delete)
      {
        // No need to delete the view, the process presenters already do it.
        QPointer<Process::LayerView> view_p{elt.view};
        delete elt.presenter;
        if (view_p)
          deleteGraphicsItem(elt.view);
      }

      return to_delete;
    });
  }
}

void TemporalConstraintPresenter::on_slotRemoved(int pos)
{
  SlotPresenter& slot = m_slots.at(pos);
  for(LayerData& elt : slot.processes)
  {
    QPointer<Process::LayerView> view_p{elt.view};
    delete elt.presenter;
    if (view_p)
      deleteGraphicsItem(elt.view);
  }

  deleteGraphicsItem(slot.handle);
  //deleteGraphicsItem(slot.view);

  m_slots.erase(m_slots.begin() + pos);
}

void TemporalConstraintPresenter::updateProcessesShape()
{
  for(int i = 0; i < m_slots.size(); i++)
  {
    for(const LayerData& proc : m_slots[i].processes)
    {
      updateProcessShape(i, proc);
    }
  }
  updateScaling();
}

void TemporalConstraintPresenter::updatePositions()
{
  using namespace std;
  // Vertical shape
  m_view->setHeight(rackHeight() + ConstraintHeader::headerHeight());

  // Set the slots position graphically in order.
  qreal currentSlotY = ConstraintHeader::headerHeight();

  for(int i = 0; i < m_slots.size(); i++)
  {
    const SlotPresenter& slot = m_slots[i];
    const Slot& model = m_model.smallView()[i];

    for(const LayerData& proc : slot.processes)
    {
      proc.view->setPos(0, currentSlotY);
      proc.view->update();
    }

    currentSlotY += model.height + SlotHandle::handleHeight();
    slot.handle->setPos(0, currentSlotY - SlotHandle::handleHeight() / 2.);
  }

  // Horizontal shape
  on_defaultDurationChanged(m_model.duration.defaultDuration());

  updateProcessesShape();
}

void TemporalConstraintPresenter::on_layerModelPutToFront(int slot, const Process::ProcessModel& proc)
{
  // Put the selected one at z+1 and the others at -z; set "disabled" graphics
  // mode.
  // OPTIMIZEME by saving the previous to front and just switching...
  for (const LayerData& elt : m_slots.at(slot).processes)
  {
    if (elt.model->id() == proc.id())
    {
      elt.presenter->putToFront();
    }
    else
    {
      elt.presenter->putBehind();
    }
  }
}

void TemporalConstraintPresenter::on_layerModelPutToBack(int slot, const Process::ProcessModel& proc)
{
  for (const LayerData& elt : m_slots.at(slot).processes)
  {
    if (elt.model->id() == proc.id())
    {
      elt.presenter->putBehind();
      return;
    }
  }
}

void TemporalConstraintPresenter::on_rackChanged()
{
  // Remove existing
  for(int i = m_slots.size(); i --> 0 ; )
  {
    on_slotRemoved(i);
  }

  // Recreate
  m_slots.reserve(m_model.smallView().size());

  int i = 0;
  for(const auto& slt : m_model.smallView())
  {
    createSlot(i, slt);
    i++;
  }

  // Update view
  updatePositions();
}

void TemporalConstraintPresenter::updateScaling()
{
  ConstraintPresenter::updateScaling();
  updateHeight();
}

void TemporalConstraintPresenter::on_zoomRatioChanged(ZoomRatio val)
{
  ConstraintPresenter::on_zoomRatioChanged(val);

  for(const SlotPresenter& slot : m_slots)
  {
    for(const LayerData& proc : slot.processes)
    {
      proc.presenter->on_zoomRatioChanged(val);
    }
  }

  updateProcessesShape();
}

void TemporalConstraintPresenter::on_defaultDurationChanged(const TimeVal& v)
{
  ConstraintPresenter::on_defaultDurationChanged(v);

  const auto w = m_view->defaultWidth();
  for(const SlotPresenter& slot : m_slots)
  {
    slot.handle->setWidth(w);
    for(const LayerData& proc : slot.processes)
    {
      proc.presenter->setWidth(w);
    }
  }
}


}
