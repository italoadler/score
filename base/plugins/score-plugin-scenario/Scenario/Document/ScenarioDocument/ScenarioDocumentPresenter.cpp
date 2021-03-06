// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <Process/Process.hpp>
#include <Process/ProcessList.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Interval/FullView/FullViewIntervalPresenter.hpp>
#include <Scenario/Document/DisplayedElements/DisplayedElementsToolPalette/DisplayedElementsToolPaletteFactoryList.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentView.hpp>
#include <Scenario/Document/TimeRuler/TimeRuler.hpp>
#include <Scenario/Document/Minimap/Minimap.hpp>
#include <score/application/ApplicationContext.hpp>
#include <score/tools/Clamp.hpp>
#include <score/widgets/DoubleSlider.hpp>

#include <Process/Style/ScenarioStyle.hpp>
#include <Process/LayerView.hpp>
#include <QPolygon>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtGlobal>
#include <Scenario/Application/ScenarioApplicationPlugin.hpp>
#include <Scenario/Document/DisplayedElements/DisplayedElementsProviderList.hpp>
#include <Scenario/Settings/ScenarioSettingsModel.hpp>
#include <score/document/DocumentInterface.hpp>

#include "ScenarioDocumentPresenter.hpp"
#include "ZoomPolicy.hpp"
#include <Process/LayerPresenter.hpp>
#include <Process/TimeValue.hpp>
#include <Process/Tools/ProcessGraphicsView.hpp>
#include <Scenario/Document/Interval/IntervalDurations.hpp>
#include <Scenario/Document/DisplayedElements/DisplayedElementsModel.hpp>
#include <Scenario/Document/DisplayedElements/DisplayedElementsPresenter.hpp>
#include <Scenario/Document/ScenarioDocument/ProcessFocusManager.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioScene.hpp>
#include <QMainWindow>
#include <score/document/DocumentContext.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#include <score/plugins/documentdelegate/DocumentDelegatePresenter.hpp>
#include <score/selection/SelectionDispatcher.hpp>
#include <score/selection/SelectionStack.hpp>
#include <score/statemachine/GraphicsSceneToolPalette.hpp>
#include <score/model/path/ObjectIdentifier.hpp>
#include <score/model/path/ObjectPath.hpp>
#include <score/tools/Todo.hpp>
#include <ossia/detail/math.hpp>

namespace Scenario
{
const ScenarioDocumentModel& ScenarioDocumentPresenter::model() const
{
  return static_cast<const ScenarioDocumentModel&>(m_model);
}

ZoomRatio ScenarioDocumentPresenter::zoomRatio() const
{
  return m_zoomRatio;
}

ScenarioDocumentView& ScenarioDocumentPresenter::view() const
{
  return safe_cast<ScenarioDocumentView&>(m_view);
}

ScenarioDocumentPresenter::ScenarioDocumentPresenter(
    const score::DocumentContext& ctx,
    score::DocumentPresenter* parent_presenter,
    const score::DocumentDelegateModel& delegate_model,
    score::DocumentDelegateView& delegate_view)
    : DocumentDelegatePresenter{parent_presenter, delegate_model,
                                         delegate_view}
    , m_scenarioPresenter{*this}
    , m_selectionDispatcher{ctx.selectionStack}
    , m_focusManager{ctx.document.focusManager()}
    , m_context{ctx, m_focusDispatcher}

{
  using namespace score;

  // Setup the connections
  con(score::GUIAppContext().mainWindow, SIGNAL(sizeChanged(QSize)),
      this, SLOT(on_windowSizeChanged(QSize)), Qt::QueuedConnection);
  con(score::GUIAppContext().mainWindow, SIGNAL(ready()),
      this, SLOT(on_viewReady()), Qt::QueuedConnection);

  con(view().view(), &ProcessGraphicsView::sizeChanged, this,
      &ScenarioDocumentPresenter::on_windowSizeChanged, Qt::QueuedConnection);
  con(view().view(), &ProcessGraphicsView::horizontalZoom, this,
      &ScenarioDocumentPresenter::on_horizontalZoom);
  con(view().view(), &ProcessGraphicsView::verticalZoom, this,
      &ScenarioDocumentPresenter::on_verticalZoom);
  con(view().view(), &ProcessGraphicsView::scrolled, this,
      &ScenarioDocumentPresenter::on_horizontalPositionChanged);

  connect(
      &m_scenarioPresenter,
      &DisplayedElementsPresenter::requestFocusedPresenterChange,
      &focusManager(),
      static_cast<void (Process::ProcessFocusManager::*)(
          QPointer<Process::LayerPresenter>)>(
          &Process::ProcessFocusManager::focus));

  con(view().timeRuler(), &TimeRuler::drag, this,
      &ScenarioDocumentPresenter::on_timeRulerScrollEvent);

  con(view().minimap(), &Minimap::visibleRectChanged,
      this, &ScenarioDocumentPresenter::on_minimapChanged);

  // Focus
  connect(
      &m_focusDispatcher, SIGNAL(focus(QPointer<Process::LayerPresenter>)),
      this, SIGNAL(setFocusedPresenter(QPointer<Process::LayerPresenter>)),
      Qt::QueuedConnection);

  con(ctx.app.settings<Settings::Model>(),
      &Settings::Model::GraphicZoomChanged, this, [&](double d) {
        auto& skin = ScenarioStyle::instance();
        skin.setIntervalWidth(d);
      });


  // Help for the FocusDispatcher.
  connect(
      this, &ScenarioDocumentPresenter::setFocusedPresenter, &m_focusManager,
      static_cast<void (Process::ProcessFocusManager::*)(
          QPointer<Process::LayerPresenter>)>(
          &Process::ProcessFocusManager::focus));

  con(m_focusManager, &Process::ProcessFocusManager::sig_defocusedViewModel,
      this, &ScenarioDocumentPresenter::on_viewModelDefocused);
  con(m_focusManager, &Process::ProcessFocusManager::sig_focusedViewModel,
      this, &ScenarioDocumentPresenter::on_viewModelFocused);
  con(m_focusManager, &Process::ProcessFocusManager::sig_focusedRoot,
      this, [] {
    ScenarioApplicationPlugin& app = score::GUIAppContext().guiApplicationPlugin<ScenarioApplicationPlugin>();
    app.editionSettings().setExpandMode(ExpandMode::GrowShrink);
  }, Qt::QueuedConnection);

  setDisplayedInterval(model().baseInterval());
}

ScenarioDocumentPresenter::~ScenarioDocumentPresenter()
{
}

IntervalModel& ScenarioDocumentPresenter::displayedInterval() const
{
  return displayedElements.interval();
}

const DisplayedElementsPresenter&ScenarioDocumentPresenter::presenters() const
{
  return m_scenarioPresenter;
}

void ScenarioDocumentPresenter::selectAll()
{
  auto processmodel = focusManager().focusedModel();
  if (processmodel)
  {
    m_selectionDispatcher.setAndCommit(processmodel->selectableChildren());
  }
}

void ScenarioDocumentPresenter::deselectAll()
{
  m_selectionDispatcher.setAndCommit(Selection{});
}

void ScenarioDocumentPresenter::selectTop()
{
  focusManager().focus(this);
  score::SelectionDispatcher{m_context.selectionStack}
    .setAndCommit(
     {&displayedElements.startState(),
      &displayedElements.interval(),
      &displayedElements.endState()});
}

void ScenarioDocumentPresenter::setMillisPerPixel(ZoomRatio newRatio)
{
  m_zoomRatio = newRatio;

  view().timeRuler().setPixelPerMillis(1.0 / m_zoomRatio);
  m_scenarioPresenter.on_zoomRatioChanged(m_zoomRatio);
}

void ScenarioDocumentPresenter::on_horizontalZoom(
    QPointF zoom, QPointF scenePoint)
{
  auto& map = view().minimap();

  // Position in pixels of the scroll in the viewport
  const double x_view = view().view().mapFromScene(scenePoint).x();
  const auto x_view_percent = x_view / double(view().viewportRect().width());

  // Zoom while keeping the zoomed-to position constant
  auto lh = map.leftHandle();
  auto rh = map.rightHandle();

  // Zoom
  lh += 0.01 * (rh - lh) * x_view_percent * zoom.y() / 2.;
  rh -= 0.01 * (rh - lh) * (1. - x_view_percent) * zoom.y() / 2.;

  view().minimap().modifyHandles(lh, rh);
}


void ScenarioDocumentPresenter::on_verticalZoom(
    QPointF zoom, QPointF scenePoint)
{
  auto z = ossia::clamp(zoom.y(), -100., 100.);
  if(z == 0.)
    return;
  auto& c = displayedInterval();
  const FullRack& slts = c.fullView();
  for(std::size_t i = 0; i < slts.size(); i++)
  {
    SlotId slot{i, Slot::FullView};
    c.setSlotHeight(slot, c.getSlotHeight(slot) + z);
  }
}
void ScenarioDocumentPresenter::on_timeRulerScrollEvent(
    QPointF previous, QPointF current)
{
  view().view().scrollHorizontal(previous.x() - current.x());
}

static bool window_size_set = false;
void ScenarioDocumentPresenter::on_windowSizeChanged(QSize s)
{
  if(m_zoomRatio == -1)
    return;

  // Keep the same zoom level with the new width.
  // Left handle should not move.
  auto new_w = view().viewWidth();

  view().timeRuler().setWidth(new_w);

  // Update the time interval if the window is greater.
  auto& c = displayedInterval();
  auto visible_rect = view().visibleSceneRect();
  if(visible_rect.width() > c.duration.guiDuration().toPixels(m_zoomRatio))
  {
    auto t = TimeVal::fromMsecs(m_zoomRatio * visible_rect.width());
    c.duration.setGuiDuration(t);
  }

  updateMinimap();
  window_size_set = true;
}

void ScenarioDocumentPresenter::on_horizontalPositionChanged(int dx)
{
  if(m_updatingView)
    return;
  auto& c = displayedInterval();
  auto& gv = view().view();

  if(dx < 0 && !m_zooming)
  {
    auto cur_rect = gv.mapToScene(gv.rect()).boundingRect();
    auto scene_rect = gv.sceneRect();
    if(cur_rect.x() + cur_rect.width() - dx > (scene_rect.width()))
    {
      auto t = TimeVal::fromMsecs(m_zoomRatio * (cur_rect.x() + cur_rect.width() - dx));
      c.duration.setGuiDuration(t);
      scene_rect.adjust(0, 0, 5, 0);
      gv.setSceneRect(scene_rect);
    }
  }
  else if(dx > 0 && !m_zooming)
  {
    TimeVal min_time = (c.duration.isMaxInfinite() ? c.duration.defaultDuration() : c.duration.maxDuration()) * 1.1;
    for(Process::ProcessModel& proc : c.processes)
    {
      if(proc.contentHasDuration())
      {
        auto d = proc.contentDuration();
        if(d > min_time)
          min_time = d;
      }
    }
    if(min_time < c.duration.guiDuration())
    {
      auto cur_rect = gv.mapToScene(gv.rect()).boundingRect();
      auto t = std::max(TimeVal::fromMsecs(m_zoomRatio * (cur_rect.x() + cur_rect.width() - dx)), min_time);
      c.duration.setGuiDuration(t);

      auto scene_rect = gv.sceneRect();
      scene_rect.adjust(0, 0, -dx, 0);
      gv.setSceneRect(scene_rect);
    }
  }

  QRectF visible_scene_rect = view().visibleSceneRect();

  view().timeRuler().setStartPoint(
      TimeVal::fromMsecs(visible_scene_rect.x() * m_zoomRatio));
  const auto dur = c.duration.guiDuration() ;
  c.setMidTime(dur * (visible_scene_rect.center().x() / dur.toPixels(m_zoomRatio)));

  if(!m_updatingMinimap)
  {
    updateMinimap();
  }
}

double ScenarioDocumentPresenter::computeReverseZoom(ZoomRatio r)
{
  const auto view_width = view().viewportRect().width();
  const auto dur = displayedInterval().duration.guiDuration();

  const auto map_w = view().minimap().width();

  return map_w * r * view_width / dur.msec();
}

ZoomRatio ScenarioDocumentPresenter::computeZoom(double l, double r)
{
  const auto map_w = view().minimap().width();
  const auto view_width = view().viewportRect().width();
  const auto dur = displayedInterval().duration.guiDuration();

  // Compute new zoom level
  const auto disptime = (dur * ((r - l) / map_w)).msec();
  return disptime / view_width;
}

void ScenarioDocumentPresenter::on_viewReady()
{
  QTimer::singleShot(0, [=] {
    auto z = displayedInterval().zoom();
    if(z > 0)
    {
      auto& c = displayedInterval();
      auto minimap_handle_width = computeReverseZoom(z);
      auto rx = (c.midTime() / c.duration.guiDuration()) * view().minimap().width() - minimap_handle_width / 2.;
      view().minimap().modifyHandles(rx, rx + minimap_handle_width);
    }
    else
    {
      view().minimap().setLargeView();
    }

    if(!window_size_set)
        on_windowSizeChanged({});
  });
}


void ScenarioDocumentPresenter::on_minimapChanged(double l, double r)
{
  m_updatingMinimap = true;
  auto& c = displayedInterval();
  const auto dur = c.duration.guiDuration();

  // Compute new zoom level
  const auto newZoom = computeZoom(l, r);

  // Compute new x position
  const auto newCstWidth = dur.toPixels(newZoom);
  auto view_width = view().viewportRect().width();
  const auto newX = newCstWidth * l / view_width;

  m_zooming = true;
  auto& gv = view().view();
  const auto& vp = *gv.viewport();
  const double w = vp.width();
  const double h = vp.height();

  const QRectF visible_scene_rect = view().visibleSceneRect();
  const double y = visible_scene_rect.top();

  // Set zoom
  if (newZoom != m_zoomRatio)
    setMillisPerPixel(newZoom);

  // Set viewport position
  auto newView = QRectF{newX, y, (qreal)w, (qreal)h};
  gv.ensureVisible(newView, 0., 0.);

  view().timeRuler().setWidth(gv.width());

  // Save state in interval
  c.setZoom(newZoom);
  c.setMidTime(dur * (view().visibleSceneRect().center().x() / dur.toPixels(newZoom)));

  m_zooming = false;

  m_updatingMinimap = false;
}

void ScenarioDocumentPresenter::updateRect(const QRectF& rect)
{
  view().view().setSceneRect(rect);
}

const Process::ProcessPresenterContext&ScenarioDocumentPresenter::context() const
{
  return m_context;
}

void ScenarioDocumentPresenter::updateMinimap()
{
  if(m_zoomRatio == -1)
    return;

  auto& minimap = view().minimap();

  const auto viewRect = view().viewportRect();
  const auto visibleSceneRect = view().visibleSceneRect();
  const auto viewWidth = viewRect.width();
  const auto cstDur = displayedInterval().duration.guiDuration();
  const auto cstWidth = cstDur.toPixels(m_zoomRatio);

  // ZoomRatio in the minimap view
  const auto zoomRatio = cstDur.msec() / viewWidth;

  minimap.setWidth(viewWidth);
  if(m_miniLayer)
  {
    m_miniLayer->setWidth(viewWidth);
    m_miniLayer->setZoomRatio(zoomRatio);
  }

  // Compute min handle spacing.
  // The maximum zoom in the main view should be 10 pixels for one millisecond.
  // Given the viewWidth and the guiDuration, compute the distance required.
  minimap.setMinDistance(20 * viewWidth / cstDur.msec());

  // Compute handle positions.
  const auto vp_x1 = visibleSceneRect.left();
  const auto vp_x2 = visibleSceneRect.right();

  const auto lh_x = viewWidth * (vp_x1 / cstWidth);
  //minimap.setLeftHandle(lh_x);

  const auto rh_x = viewWidth * (vp_x2 / cstWidth);
  minimap.setHandles(lh_x, rh_x);
  //minimap.setRightHandle(rh_x);
}

double ScenarioDocumentPresenter::displayedDuration() const
{
  return 0.9 * displayedInterval().duration.guiDuration().msec();
}

void ScenarioDocumentPresenter::setDisplayedInterval(IntervalModel& interval)
{
  auto& ctx = score::IDocument::documentContext(model());
  if (displayedElements.initialized())
  {
    if (&interval == &displayedElements.interval())
    {
      auto pres = score::IDocument::try_get<ScenarioDocumentPresenter>(
            ctx.document);
      if(pres) pres->selectTop();
      return;
    }
  }

  auto& provider
      = ctx.app.interfaces<DisplayedElementsProviderList>();
  displayedElements.setDisplayedElements(
      provider.make(&DisplayedElementsProvider::make, interval));

  m_focusManager.focusNothing();

  disconnect(m_intervalConnection);
  disconnect(m_durationConnection);
  if (&interval != &model().baseInterval())
  {
    m_intervalConnection
        = con(interval, &QObject::destroyed, this, [&]() {
            setDisplayedInterval(model().baseInterval());
          });
  }
  m_durationConnection = con(interval.duration, &IntervalDurations::guiDurationChanged,
                             this, [=] {
    updateMinimap();
  });

  // Setup of the layer in the minimap
  delete m_miniLayer;
  m_miniLayer = nullptr;

  auto& layerFactoryList = ctx.app.interfaces<Process::LayerFactoryList>();
  for(auto& proc : interval.processes)
  {
    if(auto fac = layerFactoryList.findDefaultFactory(proc))
    {
      if((m_miniLayer = fac->makeMiniLayer(proc, nullptr)))
      {
        m_miniLayer->setHeight(40);
        m_miniLayer->setWidth(view().minimap().width());
        view().minimap().scene()->addItem(m_miniLayer);
        con(proc, &Process::ProcessModel::identified_object_destroying,
            this, [=] {
          delete m_miniLayer;
          m_miniLayer = nullptr;
        });
        break;
      }
    }
  }

  // Setup of the state machine.
  const auto& fact
      = ctx.app.interfaces<DisplayedElementsToolPaletteFactoryList>();
  m_stateMachine
      = fact.make(&DisplayedElementsToolPaletteFactory::make, *this, interval);

  m_updatingView = true;
  m_scenarioPresenter.on_displayedIntervalChanged(interval);
  m_updatingView = false;
  connect(
      m_scenarioPresenter.intervalPresenter(),
      &FullViewIntervalPresenter::intervalSelected, this,
      &ScenarioDocumentPresenter::setDisplayedInterval);

  on_viewReady();
  updateMinimap();
}


void ScenarioDocumentPresenter::on_viewModelDefocused(
    const Process::ProcessModel* vm)
{
  // Deselect
  // Note : why these two lines ?
  // selectionStack.clear() should clear the selection everywhere anyway.
  if (vm)
    vm->setSelection({});

  score::IDocument::documentContext(*this).selectionStack.clearAllButLast();
}

void ScenarioDocumentPresenter::on_viewModelFocused(
    const Process::ProcessModel* process)
{
  // If the parent of the layer is a interval, we set the focus on the
  // interval too.
  auto slot = process->parent();
  if (!slot)
    return;
  auto rack = slot->parent();
  if (!rack)
    return;
  auto cm = rack->parent();
  if (auto interval = dynamic_cast<IntervalModel*>(cm))
  {
    if (m_focusedInterval)
      m_focusedInterval->focusChanged(false);

    m_focusedInterval = interval;
    m_focusedInterval->focusChanged(true);
  }
}

void ScenarioDocumentPresenter::setNewSelection(const Selection& s)
{
  static QMetaObject::Connection cur_proc_connection;
  auto process = m_focusManager.focusedModel();

  // Manages the selection (different case if we're
  // selecting something in a process, or something in full view)
  if (s.empty())
  {
    if (process)
    {
      process->setSelection(Selection{});
      QObject::disconnect(cur_proc_connection);
    }

    displayedElements.setSelection(Selection{});
    // Note : once here was a call to defocus a presenter. Why ? See git blame.
  }
  else if (ossia::any_of(s, [&](const QObject* obj) {
             return obj == &displayedElements.interval()
                    || obj == &displayedElements.startTimeSync()
                    || obj == &displayedElements.endTimeSync()
                    || obj == &displayedElements.startEvent()
                    || obj == &displayedElements.endEvent()
                    || obj == &displayedElements.startState()
                    || obj == &displayedElements.endState();
           }))
  {
    if (process)
    {
      process->setSelection(Selection{});
      QObject::disconnect(cur_proc_connection);
    }

    m_focusManager.focus(
          &score::IDocument::get<Scenario::ScenarioDocumentPresenter>(*score::IDocument::documentFromObject(this)));

    displayedElements.setSelection(s);
  }
  else
  {
    displayedElements.setSelection(Selection{});

    // We know by the presenter that all objects
    // in a given selection are in the same Process.
    auto newProc = Process::parentProcess(*s.begin());
    if (process && newProc != process)
    {
      process->setSelection(Selection{});
      QObject::disconnect(cur_proc_connection);
    }

    if (newProc)
    {
      newProc->setSelection(s);
      cur_proc_connection = connect(newProc, &Process::ProcessModel::identified_object_destroying,
              this, [&] {
        m_selectionDispatcher.setAndCommit(Selection{});
      }, Qt::UniqueConnection);
    }
  }

  view().view().setFocus();
}

Process::ProcessFocusManager&ScenarioDocumentPresenter::focusManager() const
{
  return m_focusManager;
}
}
