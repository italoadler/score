#pragma once
#include <Curve/Process/CurveProcessModel.hpp>
#include <State/Address.hpp>
#include <State/Value.hpp>
#include <State/Message.hpp>
#include <Process/State/MessageNode.hpp>
#include <Process/State/ProcessStateDataInterface.hpp>
#include <Process/ProcessMetadata.hpp>
#include <iscore_plugin_interpolation_export.h>

namespace Interpolation
{
class ProcessModel;
}

PROCESS_METADATA(
        ,
        Interpolation::ProcessModel,
        "aa569e11-03a9-4023-92c2-b590e88fec90",
        "Interpolation",
        "Interpolation"
        )

namespace Interpolation
{
class ProcessState final :
        public ProcessStateDataInterface
{
    public:
        enum Point { Start, End };
        // watchedPoint : something between 0 and 1
        ProcessState(
                ProcessModel& process,
                Point watchedPoint,
                QObject* parent);

        ProcessModel& process() const;

        ::State::Message message() const;
        Point point() const;

        ProcessState* clone(QObject* parent) const override;

        std::vector<State::Address> matchingAddresses() override;
        ::State::MessageList messages() const override;
        ::State::MessageList setMessages(
                const ::State::MessageList&,
                const Process::MessageNode&) override;

    private:
        Point m_point{};
};

class ISCORE_PLUGIN_INTERPOLATION_EXPORT ProcessModel final :
        public Curve::CurveProcessModel
{
        ISCORE_SERIALIZE_FRIENDS(ProcessModel, DataStream)
        ISCORE_SERIALIZE_FRIENDS(ProcessModel, JSONObject)
        MODEL_METADATA_IMPL(Interpolation::ProcessModel)

        Q_OBJECT
        Q_PROPERTY(State::Address address READ address WRITE setAddress NOTIFY addressChanged)
        Q_PROPERTY(State::Value start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(State::Value end READ end WRITE setEnd NOTIFY endChanged)
        Q_PROPERTY(bool tween READ tween WRITE setTween NOTIFY tweenChanged)

    public:
        ProcessModel(const TimeValue& duration,
                     const Id<Process::ProcessModel>& id,
                     QObject* parent);
        ~ProcessModel();

        template<typename Impl>
        ProcessModel(Deserializer<Impl>& vis, QObject* parent) :
            CurveProcessModel{vis, parent},
            m_startState{new ProcessState{*this, ProcessState::Start, this}},
            m_endState{new ProcessState{*this, ProcessState::End, this}}
        {
            vis.writeTo(*this);
        }


        ::State::Address address() const;

        State::Value start() const;
        State::Value end() const;

        void setAddress(const ::State::Address& arg);
        void setStart(State::Value arg);
        void setEnd(State::Value arg);

        bool tween() const
        {
            return m_tween;
        }
        void setTween(bool tween)
        {
            if (m_tween == tween)
                return;

            m_tween = tween;
            emit tweenChanged(tween);
        }

        QString prettyName() const override;

    signals:
        void addressChanged(const ::State::Address&);
        void startChanged(const State::Value&);
        void endChanged(const State::Value&);
        void tweenChanged(bool tween);

    private:
        //// ProcessModel ////
        void setDurationAndScale(const TimeValue& newDuration) override;
        void setDurationAndGrow(const TimeValue& newDuration) override;
        void setDurationAndShrink(const TimeValue& newDuration) override;

        /// States
        ProcessState* startStateData() const override;
        ProcessState* endStateData() const override;

        ProcessModel(const ProcessModel& source,
                        const Id<Process::ProcessModel>& id,
                        QObject* parent);

        void setCurve_impl() override;
        ::State::Address m_address;

        State::Value m_start{};
        State::Value m_end{};

        ProcessState* m_startState{};
        ProcessState* m_endState{};
        bool m_tween = false;
};
}