#pragma once

#include <memory>

#include "motis/module/module.h"

namespace motis::revise {

struct revise : public motis::module::module {
  revise();
  ~revise() override;

  revise(revise const&) = delete;
  revise& operator=(revise const&) = delete;

  revise(revise&&) = delete;
  revise& operator=(revise&&) = delete;

  void init(motis::module::registry&) override;

private:
  motis::module::msg_ptr update(motis::module::msg_ptr const&);
  motis::module::msg_ptr update(Connection const* con);
  motis::module::msg_ptr update(ReviseRequest const* req);
};

}  // namespace motis::revise
