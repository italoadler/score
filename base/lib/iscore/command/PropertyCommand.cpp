#include "PropertyCommand.hpp"

iscore::PropertyCommand::PropertyCommand(ObjectPath &&path, QString property):
    SerializableCommand("", "", ""),
    m_path{std::move(path)},
    m_property{property}
{

}

void iscore::PropertyCommand::undo()
{
    m_path.find<QObject>().setProperty(m_property.toLatin1().constData(), m_old);
}

void iscore::PropertyCommand::redo()
{
    m_path.find<QObject>().setProperty(m_property.toLatin1().constData(), m_new);
}

void iscore::PropertyCommand::serializeImpl(QDataStream & s) const
{
    s << m_path << m_property << m_old << m_new;
}

void iscore::PropertyCommand::deserializeImpl(QDataStream & s)
{
    s >> m_path >> m_property >> m_old >> m_new;
}
