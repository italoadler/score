// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "score_plugin_library.hpp"

#include <Library/Panel/LibraryPanelFactory.hpp>
#include <score/plugins/customfactory/FactorySetup.hpp>

score_plugin_library::score_plugin_library()
{
}

score_plugin_library::~score_plugin_library()
{
}

std::vector<std::unique_ptr<score::InterfaceBase>>
score_plugin_library::guiFactories(
    const score::GUIApplicationContext& ctx,
    const score::InterfaceKey& key) const
{
  return instantiate_factories<score::ApplicationContext, FW<score::PanelDelegateFactory, Library::PanelDelegateFactory>>(
      ctx, key);
}
