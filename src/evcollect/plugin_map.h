/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <evcollect/util/return_code.h>

namespace evcollect {
class SourcePlugin;
class OutputPlugin;
struct ProcessConfig;

class PluginMap {
public:

  PluginMap(const ProcessConfig* config);
  ~PluginMap();

  void registerSourcePlugin(
      const std::string& plugin_name,
      std::unique_ptr<SourcePlugin> plugin);

  ReturnCode getSourcePlugin(
      const std::string& plugin_name,
      SourcePlugin** plugin) const;

  void registerOutputPlugin(
      const std::string& plugin_name,
      std::unique_ptr<OutputPlugin> plugin);

  ReturnCode getOutputPlugin(
      const std::string& plugin_name,
      OutputPlugin** plugin) const;

protected:

  struct SourcePluginBinding {
    std::unique_ptr<SourcePlugin> plugin;
    bool plugin_initialized;
  };

  struct OutputPluginBinding {
    std::unique_ptr<OutputPlugin> plugin;
    bool plugin_initialized;
  };

  const ProcessConfig* config_;
  mutable std::unordered_map<std::string, SourcePluginBinding> source_plugins_;
  mutable std::unordered_map<std::string, OutputPluginBinding> output_plugins_;
};

} // namespace evcollect
