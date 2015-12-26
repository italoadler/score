
if(ISCORE_STATIC_QT)
    get_target_property(QtCore_LIB Qt5::Core LOCATION)
    get_filename_component(QT_LIB_FOLDER ${QtCore_LIB} DIRECTORY)

    find_library(qtharfbuzzng_LIBRARY NAMES qtharfbuzzng libqtharfbuzzng HINTS ${QT_LIB_FOLDER})
    find_library(qtfreetype_LIBRARY NAMES qtfreetype libqtfreetype HINTS ${QT_LIB_FOLDER})
    find_library(Qt5XcbQpa_LIBRARY NAMES Qt5XcbQpa libQt5XcbQpa HINTS ${QT_LIB_FOLDER})
    find_library(Qt5PlatformSupport_LIBRARY NAMES Qt5PlatformSupport libQt5PlatformSupport HINTS ${QT_LIB_FOLDER})
    find_library(qtpcre_LIBRARY NAMES qtpcre libqtpcre HINTS ${QT_LIB_FOLDER})

    find_library(qsvg_LIBRARY NAMES qsvg libqsvg HINTS ${QT_LIB_FOLDER}/../plugins/imageformats)

    target_link_libraries(
        ${APPNAME}
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Svg Qt5::Xml
    )

    if(TARGET Qt5::WebSockets)
      target_link_libraries(${APPNAME} Qt5::WebSockets)
    endif()

    if(TARGET Qt5::QXcbIntegrationPlugin)
      target_link_libraries(${APPNAME}
        Qt5::QXcbIntegrationPlugin
        ${Qt5XcbQpa_LIBRARY}
        ${Qt5PlatformSupport_LIBRARY}
        ${qtharfbuzzng_LIBRARY}
        ${qtpcre_LIBRARY}
        ${qtfreetype_LIBRARY}
        ${qsvg_LIBRARY}
        GL
        Xi
        xcb-render xcb-image xcb-icccm xcb-sync xcb-xfixes xcb-shm xcb-randr xcb-shape xcb-keysyms xcb-xkb xcb-render-util  xcb
        X11-xcb Xrender Xext X11
        z dl rt
      )
    endif()
      target_link_libraries(${APPNAME}
         m pthread
      )
endif()
