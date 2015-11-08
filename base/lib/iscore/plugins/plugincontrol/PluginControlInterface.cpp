#include "PluginControlInterface.hpp"

#include <QApplication>

using namespace iscore;


PluginControlInterface::PluginControlInterface(iscore::Presenter* presenter,
                                               const QString& name,
                                               QObject* parent):
    NamedObject{name, parent},
    m_presenter{presenter}
{
    connect(this, &PluginControlInterface::documentChanged,
            this, &PluginControlInterface::on_documentChanged);

    connect(presenter, &iscore::Presenter::prepareNewDocument,
            this, &PluginControlInterface::on_prepareNewDocument);

    connect(qApp, &QApplication::applicationStateChanged,
            this, &PluginControlInterface::on_focusChanged);
}

PluginControlInterface::~PluginControlInterface()
{

}

void PluginControlInterface::populateMenus(MenubarManager*)
{

}


QList<OrderedToolbar> PluginControlInterface::makeToolbars()
{
    return {};
}

QList<QAction*> PluginControlInterface::actions()
{
    return {};
}


DocumentDelegatePluginModel*PluginControlInterface::loadDocumentPlugin(
        const QString& name,
        const VisitorVariant& var,
        DocumentModel* parent)
{
    return nullptr;
}


Presenter*PluginControlInterface::presenter() const
{
    return m_presenter;
}

Document*PluginControlInterface::currentDocument() const
{
    return m_presenter->currentDocument();
}

void PluginControlInterface::on_prepareNewDocument()
{

}


void PluginControlInterface::on_documentChanged(
        iscore::Document* olddoc,
        iscore::Document* newdoc)
{

}

void PluginControlInterface::on_newDocument(iscore::Document* doc)
{

}

void PluginControlInterface::on_loadedDocument(iscore::Document *doc)
{

}


void PluginControlInterface::on_focusChanged(Qt::ApplicationState st)
{
    if(st == Qt::ApplicationActive)
        emit focused();
    else
        emit defocused();
}
