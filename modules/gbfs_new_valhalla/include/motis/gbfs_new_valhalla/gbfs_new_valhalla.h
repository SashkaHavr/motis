#pragma once

#include <memory>
#include <string>
#include <vector>

#include "motis/module/module.h"

namespace motis::gbfs_new_valhalla {

struct config {
  std::string valhalla_server_url = "http://localhost:8002/";

  std::string gbfs_route_url() const {
    return valhalla_server_url + "gbfs_route";
  }
};

struct gbfs_new_valhalla : public motis::module::module {
  gbfs_new_valhalla();
  ~gbfs_new_valhalla() override;

  gbfs_new_valhalla(gbfs_new_valhalla const&) = delete;
  gbfs_new_valhalla& operator=(gbfs_new_valhalla const&) = delete;

  gbfs_new_valhalla(gbfs_new_valhalla&&) = delete;
  gbfs_new_valhalla& operator=(gbfs_new_valhalla&&) = delete;

  void init(motis::module::registry&) override;
  void import(motis::module::import_dispatcher&) override;
  bool import_successful() const override { return import_successful_; }

private:
  bool import_successful_{false};

  struct impl;
  std::unique_ptr<impl> impl_;
  config config_;
};

}  // namespace motis::gbfs_new_valhalla
