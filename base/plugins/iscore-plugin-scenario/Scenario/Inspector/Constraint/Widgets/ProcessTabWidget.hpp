#pragma once

#include <QWidget>
#include <Scenario/Inspector/Constraint/ConstraintInspectorWidget.hpp>

namespace Inspector{
    class InspectorSectionWidget;
}
namespace Scenario {
class ProcessTabWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit ProcessTabWidget(const ConstraintInspectorWidget& parentCstr, QWidget *parent = 0);

    signals:

    public slots:
        void createProcess(const ProcessFactoryKey& processName);
        void displaySharedProcess(const Process::ProcessModel&);

        void updateDisplayedValues();

    private:
        void ask_processNameChanged(const Process::ProcessModel& p, QString s);
        void createLayerInNewSlot(const Id<Process::ProcessModel>& processId);

        const ConstraintInspectorWidget& m_constraintWidget;
        Inspector::InspectorSectionWidget* m_processSection;

        std::vector<Inspector::InspectorSectionWidget*> m_processesSectionWidgets;

};

}