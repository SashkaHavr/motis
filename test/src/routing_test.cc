#include "gtest/gtest.h"

#include "motis/core/journey/journey.h"
#include "motis/core/journey/message_to_journeys.h"
#include "motis/module/message.h"
#include "motis/test/motis_instance_test.h"
#include "motis/test/schedule/simple_realtime.h"

using namespace flatbuffers;
using namespace motis;
using namespace motis::test;
using namespace motis::module;
using namespace motis::routing;
using motis::test::schedule::simple_realtime::dataset_opt;

struct routing_itest : public motis_instance_test {
  routing_itest()
      : motis::test::motis_instance_test(
            dataset_opt, {"routing", "csa", "raptor", "tripbased"}) {}

  msg_ptr make_routing_request(std::string const& target) {
    message_creator fbb;
    fbb.create_and_finish(
        MsgContent_RoutingRequest,
        CreateRoutingRequest(
            fbb, motis::routing::Start_OntripStationStart,
            CreateOntripStationStart(
                fbb,
                CreateInputStation(fbb, fbb.CreateString("8000096"),
                                   fbb.CreateString("")),
                unix_time(1300))
                .Union(),
            CreateInputStation(fbb, fbb.CreateString("8000080"),
                               fbb.CreateString("")),
            SearchType_Default, SearchDir_Forward,
            fbb.CreateVector(std::vector<Offset<Via>>()),
            fbb.CreateVector(std::vector<Offset<AdditionalEdgeWrapper>>()))
            .Union(),
        target);
    return make_msg(fbb);
  }
};

TEST_F(routing_itest, all_routings_deliver_equal_journey) {
  for (auto const& target : {"/routing", "/tripbased", "/raptor_cpu", "/csa"}) {
    auto const response = call(make_routing_request(target));
    auto const res = motis_content(RoutingResponse, response);
    EXPECT_EQ(res->connections()->size(), 1);
  }
}