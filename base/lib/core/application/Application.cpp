#include <core/application/Application.hpp>
#include <core/application/OpenDocumentsFile.hpp>

#include <core/presenter/Presenter.hpp>
#include <core/view/View.hpp>

#include <core/undo/Panel/UndoPanelFactory.hpp>
#include <core/undo/UndoControl.hpp>
#include <QSplashScreen>
#include <QFontDatabase>
#include <core/document/DocumentBackups.hpp>
#include "iscore_git_info.hpp"
using namespace iscore;
#include <QMessageBox>
#include <QFileInfo>
#include <QStyleFactory>
#include "SafeQApplication.hpp"
static Application* application_instance = nullptr;


#ifdef ISCORE_DEBUG

static void myMessageOutput(
        QtMsgType type,
        const QMessageLogContext &context,
        const QString &msg)
{
    auto basename_arr = QFileInfo(context.file).baseName().toUtf8();
    auto basename = basename_arr.constData();

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), basename, context.line, context.function);
        break;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), basename, context.line, context.function);
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), basename, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), basename, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), basename, context.line, context.function);
        ISCORE_BREAKPOINT;
        std::terminate();
    }
}

#endif

Application::Application(int& argc, char** argv) :
    NamedObject {"Application", nullptr}
{
#ifdef ISCORE_DEBUG
    qInstallMessageHandler(myMessageOutput);
#endif
    // Application
    // Crashes if put in member initialization list... :(
    m_app = new SafeQApplication{argc, argv};
    ::application_instance = this;

    m_applicationSettings.parse();

    init();
}

Application::Application(
        const ApplicationSettings& appSettings,
        int& argc,
        char** argv) :
    NamedObject {"Application", nullptr},
    m_applicationSettings(appSettings)
{
#ifdef ISCORE_DEBUG
    qInstallMessageHandler(myMessageOutput);
#endif
    // Application
    // Crashes if put in member initialization list... :(
    m_app = new SafeQApplication{argc, argv};
    ::application_instance = this;

    init();
}


Application::~Application()
{
    this->setParent(nullptr);
    delete m_presenter;

    DocumentBackups::clear();
    delete m_app;
}

void Application::init()
{
#if !defined(ISCORE_DEBUG)
    QPixmap logo{":/iscore.png"};
    QSplashScreen splash{logo, Qt::FramelessWindowHint};
    if(m_applicationSettings.gui)
        splash.show();
#endif

    QFontDatabase::addApplicationFont(":/Ubuntu-R.ttf");
    m_app->setFont(QFont{"Ubuntu", 10, QFont::Normal});
    this->setParent(m_app);
    this->setObjectName("Application");

    QCoreApplication::setOrganizationName("OSSIA");
    QCoreApplication::setOrganizationDomain("i-score.org");
    QCoreApplication::setApplicationName("i-score");
    QCoreApplication::setApplicationVersion(
                QString("%1.%2.%3-%4")
                .arg(ISCORE_VERSION_MAJOR)
                .arg(ISCORE_VERSION_MINOR)
                .arg(ISCORE_VERSION_PATCH)
                .arg(ISCORE_VERSION_EXTRA)
                );

    qRegisterMetaType<ObjectIdentifierVector> ("ObjectIdentifierVector");
    qRegisterMetaType<Selection>("Selection");

    QFile stylesheet_file{":/qdarkstyle/qdarkstyle.qss"};
    stylesheet_file.open(QFile::ReadOnly);
    QString stylesheet = QLatin1String(stylesheet_file.readAll());

    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setStyleSheet(stylesheet);

    // Settings
    m_settings = std::make_unique<Settings> (this);

    // MVP
    m_view = new View{this};
    m_presenter = new Presenter{m_view, this};

    // Plugins
    m_pluginManager.reloadPlugins();
    m_pluginManager.addControl(new UndoControl{m_presenter, m_presenter});
    m_pluginManager.addPanel(new UndoPanelFactory);

    loadPluginData();

    // View

    if(m_applicationSettings.gui)
    {
        m_view->show();

    #if !defined(ISCORE_DEBUG)
        splash.finish(m_view);
    #endif
    }

    if(!m_applicationSettings.loadList.empty())
    {
        for(const auto& doc : m_applicationSettings.loadList)
            m_presenter->loadFile(doc);

        if(!m_presenter->documents().isEmpty())
        {
            if(m_applicationSettings.autoplay)
            {
                // TODO find a better way, most likely by allowing plug-ins to have
                // command line arguments, too. Eww.
                emit autoplay();
            }
            return; // A document was loaded correctly
        }
    }

    // Try to reload if there was a crash
    if(m_applicationSettings.tryToRestore && DocumentBackups::canRestoreDocuments())
    {
        m_presenter->restoreDocuments();
    }
    else
    {
        if(!m_pluginManager.m_documentPanelList.empty())
            m_presenter->newDocument(m_pluginManager.m_documentPanelList.front());
    }
}

Application &Application::instance()
{
    return *application_instance;
}

void Application::loadPluginData()
{
    for(auto& set : m_pluginManager.m_settingsList)
    {
        m_settings->setupSettingsPlugin(set);
    }

    for(auto& cmd : m_pluginManager.m_controlList)
    {
        m_presenter->registerPluginControl(cmd);
    }

    std::sort(m_presenter->toolbars().begin(), m_presenter->toolbars().end());
    for(auto& toolbar : m_presenter->toolbars())
    {
        m_view->addToolBar(toolbar.bar);
    }

    for(auto& pnl : m_pluginManager.m_panelList)
    {
        m_presenter->registerPanel(pnl);
    }

    for(auto& pnl : m_pluginManager.m_documentPanelList)
    {
        m_presenter->registerDocumentDelegate(pnl);
    }
}

