#pragma once

class QString;
template <class Key, class T> class QMap;

namespace iscore {
enum class IOType : int { Invalid, In, Out, InOut };
const QMap<IOType, QString>& IOTypeStringMap();

inline bool hasInput(IOType t)
{ return t == IOType::InOut || t == IOType::In; }
inline bool hasOutput(IOType t)
{ return t == IOType::InOut || t == IOType::Out; }
}
