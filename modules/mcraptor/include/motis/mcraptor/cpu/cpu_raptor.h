#pragma once

#include "motis/mcraptor/cpu/mark_store.h"

#include "motis/mcraptor/raptor_query.h"
#include "motis/mcraptor/raptor_result.h"
#include "motis/mcraptor/raptor_statistics.h"
#include "motis/mcraptor/raptor_timetable.h"

namespace motis::mcraptor {
template <class T, class L>
struct mc_raptor {

  mc_raptor(raptor_query<L> const& q) : query_(q),
                                    result_(q.result()),
                                    route_labels_(q.tt_.stop_count()),
                                    transfer_labels_(q.tt_.stop_count()),
                                    stops_for_transfers_(* new cpu_mark_store(q.tt_.stop_count())),
                                    stops_for_routes_(* new cpu_mark_store(q.tt_.stop_count())),
                                    round_(-1) {};

  void invoke_cpu_raptor();

  bag<L>* current_round();
  bag<L>* previous_round();
  void start_new_round();

  void init_arrivals();
  void arrival_by_route(stop_id stop, L& new_label);
  void arrival_by_transfer(stop_id stop, L& label);
  void relax_transfers();
  void collect_routes_serving_updated_stops();
  void scan_routes();

  inline std::vector<std::pair<route_id, route_stops_index>> get_routes_times_for_stop(stop_id stop);

  //static derived methods


  //fields
  raptor_query<L> const& query_;
  rounds<L>& result_;
  int round_;

  std::vector<bag<L>> route_labels_;
  std::vector<bag<L>> transfer_labels_;
  std::map<route_id, route_stops_index> routes_serving_updated_stops_;
  cpu_mark_store& stops_for_transfers_;
  cpu_mark_store& stops_for_routes_;

};

struct mc_raptor_departure: public mc_raptor<mc_raptor_departure, label_departure> {
  mc_raptor_departure(raptor_query<label_departure> const& q) : mc_raptor(q) { }
};

struct mc_raptor_arrival: public mc_raptor<mc_raptor_arrival, label_arrival> {
  mc_raptor_arrival(raptor_query<label_arrival> const& q) : mc_raptor(q) { }
};

template class mc_raptor<mc_raptor_departure, label_departure>;
template class mc_raptor<mc_raptor_arrival, label_arrival>;

}  // namespace motis::mcraptor