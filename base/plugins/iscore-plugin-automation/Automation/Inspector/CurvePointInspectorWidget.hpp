#pragma once

#include <Inspector/InspectorWidgetBase.hpp>
#include <iscore/command/Dispatchers/OngoingCommandDispatcher.hpp>

class CurvePointModel;
class QDoubleSpinBox;
class QWidget;
namespace iscore {
class Document;
}  // namespace iscore

class CurvePointInspectorWidget final : public InspectorWidgetBase
{
    Q_OBJECT
    public:
        explicit CurvePointInspectorWidget(
            const CurvePointModel& model,
            const iscore::DocumentContext& context,
            QWidget* parent);

    public slots:
        void on_pointChanged(double);
        void on_editFinished();

    private:
        const CurvePointModel& m_model;
        QDoubleSpinBox* m_XBox;
        QDoubleSpinBox* m_YBox;
        OngoingCommandDispatcher m_dispatcher;
        double m_yFactor;
        double m_xFactor;

        double m_Ymin;
};
