#include "motis/rsl/messages.h"

#include "utl/to_vec.h"

#include "motis/core/conv/station_conv.h"
#include "motis/core/conv/trip_conv.h"

using namespace motis::module;
using namespace flatbuffers;

namespace motis::rsl {

Offset<TransferInfo> to_fbs(FlatBufferBuilder& fbb,
                            std::optional<transfer_info> const& ti) {
  if (ti) {
    auto const& val = ti.value();
    return CreateTransferInfo(fbb,
                              val.type_ == transfer_info::type::SAME_STATION
                                  ? TransferType_SAME_STATION
                                  : TransferType_FOOTPATH,
                              val.duration_);
  } else {
    return CreateTransferInfo(fbb, TransferType_NONE);
  }
}

Offset<CompactJourneyLeg> to_fbs(schedule const& sched, FlatBufferBuilder& fbb,
                                 journey_leg const& leg) {
  return CreateCompactJourneyLeg(
      fbb, to_fbs(fbb, leg.trip_),
      to_fbs(fbb, *sched.stations_[leg.enter_station_id_]),
      to_fbs(fbb, *sched.stations_[leg.exit_station_id_]),
      motis_to_unixtime(sched, leg.enter_time_),
      motis_to_unixtime(sched, leg.exit_time_),
      to_fbs(fbb, leg.enter_transfer_));
}

Offset<CompactJourney> to_fbs(schedule const& sched, FlatBufferBuilder& fbb,
                              compact_journey const& cj) {
  return CreateCompactJourney(
      fbb, fbb.CreateVector(utl::to_vec(cj.legs_, [&](journey_leg const& leg) {
        return to_fbs(sched, fbb, leg);
      })));
}

Offset<PassengerGroup> to_fbs(schedule const& sched, FlatBufferBuilder& fbb,
                              passenger_group const& pg) {
  return CreatePassengerGroup(fbb, pg.id_, pg.sub_id_, pg.passengers_,
                              to_fbs(sched, fbb, pg.compact_planned_journey_));
}

Offset<void> to_fbs(schedule const& sched, FlatBufferBuilder& fbb,
                    passenger_localization const& loc) {
  if (loc.in_trip()) {
    return CreatePassengerInTrip(fbb, to_fbs(sched, fbb, loc.in_trip_),
                                 to_fbs(fbb, *loc.at_station_),
                                 motis_to_unixtime(sched, loc.arrival_time_))
        .Union();
  } else {
    return CreatePassengerAtStation(fbb, to_fbs(fbb, *loc.at_station_),
                                    motis_to_unixtime(sched, loc.arrival_time_))
        .Union();
  }
}

PassengerLocalization fbs_localization_type(passenger_localization const& loc) {
  return loc.in_trip() ? PassengerLocalization_PassengerInTrip
                       : PassengerLocalization_PassengerAtStation;
}
Offset<MonitoringEvent> to_fbs(schedule const& sched, FlatBufferBuilder& fbb,
                               monitoring_event const& me) {
  return CreateMonitoringEvent(fbb, static_cast<MonitoringEventType>(me.type_),
                               to_fbs(sched, fbb, me.group_),
                               fbs_localization_type(me.localization_),
                               to_fbs(sched, fbb, me.localization_));
}

Offset<PassengerForecastResult> to_fbs(schedule const& sched,
                                       FlatBufferBuilder& fbb,
                                       simulation_result const& res,
                                       graph const& g) {
  auto const trip_with_edges_to_fbs = [&](trip const* trp,
                                          std::vector<edge*> const& edges) {
    std::vector<Offset<EdgeOverCapacity>> fb_edges;
    for (auto const e : edges) {
      if (e->type_ != edge_type::TRIP) {
        continue;
      }
      fb_edges.emplace_back(CreateEdgeOverCapacity(
          fbb, e->passengers_, e->capacity_, res.additional_passengers_.at(e),
          to_fbs(fbb, e->from(g)->get_station(sched)),
          to_fbs(fbb, e->to(g)->get_station(sched))));
    }
    return CreateTripOverCapacity(fbb, to_fbs(sched, fbb, trp),
                                  fbb.CreateVector(fb_edges));
  };

  return CreatePassengerForecastResult(
      fbb, res.is_over_capacity(), res.edge_count_over_capacity(),
      res.total_passengers_over_capacity(),
      fbb.CreateVector(utl::to_vec(
          res.trips_over_capacity_with_edges(), [&](auto const& kv) {
            return trip_with_edges_to_fbs(kv.first, kv.second);
          })));
}

msg_ptr make_passenger_forecast_msg(
    schedule const& sched, rsl_data const& data,
    std::map<unsigned, std::vector<combined_passenger_group>> const&
        combined_groups,
    simulation_result const& sim_result) {
  message_creator mc;
  std::vector<Offset<PassengerGroupForecast>> fb_groups;
  for (auto const& [destination_station_id, cpgs] : combined_groups) {
    (void)destination_station_id;
    for (auto const& cpg : cpgs) {
      auto const loc_type = fbs_localization_type(cpg.localization_);
      auto const loc = to_fbs(sched, mc, cpg.localization_);
      for (auto const& grp : cpg.groups_) {
        // TODO(pablo): forecast_journey
        fb_groups.emplace_back(CreatePassengerGroupForecast(
            mc, to_fbs(sched, mc, *grp), loc_type, loc, 0));
      }
    }
  }
  mc.create_and_finish(
      MsgContent_PassengerForecast,
      CreatePassengerForecast(mc, mc.CreateVector(fb_groups),
                              to_fbs(sched, mc, sim_result, data.graph_))
          .Union(),
      "/rsl/passenger_forecast");
  return make_msg(mc);
}

}  // namespace motis::rsl
