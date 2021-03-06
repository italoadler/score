// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ScenarioStyle.hpp"
#include <score/model/Skin.hpp>
// TODO namespace
ScenarioStyle::ScenarioStyle(const score::Skin& s) noexcept
    :

    IntervalBase{&s.Base1}
    , IntervalSelected{&s.Base2}
    , IntervalPlayFill{&s.Base3}
    , IntervalPlayDashFill{&s.Pulse1}
    , IntervalWaitingDashFill{&s.Pulse2}
    , IntervalLoop{&s.Warn1}
    , IntervalWarning{&s.Warn2}
    , IntervalInvalid{&s.Warn3}
    , IntervalMuted{&s.Tender2}
    , IntervalDefaultLabel{&s.Gray}
    , IntervalDefaultBackground{&s.Transparent1}
    ,

    RackSideBorder{&s.Base1}
    ,

    IntervalFullViewParentSelected{&s.Emphasis1}
    ,

    IntervalHeaderText{&s.Light}
    , IntervalHeaderBottomLine{&s.Transparent1}
    , IntervalHeaderRackHidden{&s.Transparent1}
    , IntervalHeaderSideBorder{&s.Base1}
    ,

    ProcessViewBorder{&s.Gray}
    ,

    SlotFocus{&s.Base2}
    , SlotOverlayBorder{&s.Dark}
    , SlotOverlay{&s.Transparent2}
    , SlotHandle{&s.Transparent3}
    ,

    TimenodeDefault{&s.Gray}
    , TimenodeSelected{&s.Base2}
    ,

    EventDefault{&s.Emphasis4}
    , EventWaiting{&s.HalfLight}
    , EventPending{&s.Warn1}
    , EventHappened{&s.Base3}
    , EventDisposed{&s.Warn3}
    , EventSelected{&s.Base2}
    ,

    ConditionDefault{&s.Smooth3}
    , ConditionWaiting{&s.Gray}
    , ConditionDisabled{&s.Base1}
    , ConditionFalse{&s.Smooth1}
    , ConditionTrue{&s.Smooth2}
    ,

    StateOutline{&s.Light}
    , StateSelected{&s.Base2}
    , StateDot{&s.Base1}
    ,

    Background{&s.Background1}
    , ProcessPanelBackground{&s.Transparent1}
    ,

    TimeRulerBackground{&s.Background1}
    , TimeRuler{&s.Base1}
    , LocalTimeRuler{&s.Gray}
    , CommentBlockPen{Qt::white, 1.}
    , SeparatorPen{Qt::white, 2.}
    , SeparatorBrush{Qt::white}
    , TransparentBrush{Qt::transparent}
{
  update(s);
  QObject::connect(&s, &score::Skin::changed, [&] { this->update(s); });
}

void ScenarioStyle::setIntervalWidth(double w)
{
  IntervalSolidPen.setWidth(3 * w);
  IntervalDashPen.setWidth(3 * w);
  IntervalPlayPen.setWidth(3 * w);
  IntervalPlayDashPen.setWidth(3 * w);
  IntervalWaitingDashPen.setWidth(3 * w);
}

ScenarioStyle& ScenarioStyle::instance()
{
  static ScenarioStyle s(score::Skin::instance());
  return s;
}

ScenarioStyle::ScenarioStyle() noexcept
{
  update(score::Skin::instance());
}

void ScenarioStyle::update(const score::Skin& skin)
{
  IntervalSolidPen = QPen{QBrush{Qt::black}, 3, Qt::SolidLine, Qt::SquareCap,
                            Qt::RoundJoin};
  IntervalDashPen = [] {
    QPen pen{QBrush{Qt::black}, 3, Qt::CustomDashLine, Qt::SquareCap,
             Qt::RoundJoin};

    pen.setDashPattern({2., 4.});
    return pen;
  }();
  IntervalRackPen
      = QPen{Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  IntervalPlayPen
      = QPen{Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  IntervalPlayDashPen = IntervalDashPen;
  IntervalWaitingDashPen = IntervalDashPen;
  IntervalHeaderSeparator
      = QPen{IntervalHeaderSideBorder.getBrush(), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  FullViewIntervalHeaderSeparator
      = QPen{IntervalHeaderSideBorder.getBrush(), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin};

  IntervalBrace
      = QPen{IntervalBase.getBrush(), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  IntervalBraceSelected = IntervalBrace;
  IntervalBraceSelected.setBrush(IntervalSelected.getBrush());
  IntervalBraceWarning = IntervalBrace;
  IntervalBraceWarning.setBrush(IntervalWarning.getBrush());
  IntervalBraceInvalid = IntervalBrace;
  IntervalBraceInvalid.setBrush(IntervalInvalid.getBrush());
  IntervalHeaderTextPen = QPen{IntervalHeaderText.getBrush().color()};

  // don't: IntervalSolidPen.setCosmetic(true);
  //IntervalDashPen.setCosmetic(true);
  IntervalRackPen.setCosmetic(true);
  //IntervalPlayPen.setCosmetic(true);
  //IntervalPlayDashPen.setCosmetic(true);
  //IntervalWaitingDashPen.setCosmetic(true);

  ConditionPen = QPen{Qt::black, 2};
  ConditionTrianglePen = QPen{Qt::black, 2};
  ConditionTrianglePen.setCosmetic(true);

  TimenodePen = QPen{Qt::black, 2, Qt::DotLine, Qt::SquareCap, Qt::MiterJoin};
  TimenodeBrush = QBrush{Qt::black};
  TimenodePen.setCosmetic(true);

  MinimapPen = QPen{QColor(qRgba(80, 100, 140, 100)), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  MinimapBrush = QBrush{qRgba(20, 70, 80, 1)};

  StateTemporalPointBrush = QBrush{Qt::black};
  StateTemporalPointPen.setCosmetic(true);
  StateBrush = QBrush{Qt::black};
  EventPen = QPen{Qt::black, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  EventPen.setCosmetic(true);
  EventBrush = QBrush{Qt::black};

  TimeRulerLargePen = QPen{TimeRuler.getBrush(), 2, Qt::SolidLine};
  TimeRulerLargePen.setCosmetic(true);
  TimeRulerSmallPen = QPen{TimeRuler.getBrush(), 1, Qt::SolidLine};
  TimeRulerSmallPen.setCosmetic(true);

  SlotHandlePen.setWidth(0);
  SlotHandlePen.setBrush(ProcessViewBorder.getBrush());

  MiniScenarioPen.setCosmetic(true);

  Bold10Pt = skin.SansFont;
  Bold10Pt.setPointSize(10);
  Bold10Pt.setBold(true);

  Bold12Pt = Bold10Pt;
  Bold12Pt.setPointSize(12);

  Medium7Pt = skin.SansFont;
  Medium7Pt.setPointSize(7);
  Medium7Pt.setStyleStrategy(QFont::NoAntialias);
  Medium7Pt.setHintingPreference(QFont::HintingPreference::PreferFullHinting);

  Medium8Pt = skin.MonoFont;
  Medium8Pt.setPointSize(8);

  Medium12Pt = skin.SansFont;
  Medium12Pt.setPointSize(12);
  Medium12Pt.setStyleStrategy(QFont::NoAntialias);
}
