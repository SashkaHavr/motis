#include <fstream>

#include "motis/core/common/timing.h"
#include "motis/core/schedule/time.h"
#include "motis/core/journey/journey.h"

#include "motis/mcraptor/cpu/cpu_raptor.h"
#include "motis/mcraptor/raptor_timetable.h"
#include "motis/mcraptor/raptor_util.h"
#include "motis/mcraptor/reconstructor.h"

#if defined(MOTIS_CUDA)
#include "motis/mcraptor/gpu/gpu_raptor.cuh"
#endif

namespace motis::mcraptor {

inline auto get_departure_range(time const begin, time const end,
                                std::vector<time> const& departure_events) {
  std::ptrdiff_t const lower =
      std::distance(std::cbegin(departure_events),
                    std::lower_bound(std::cbegin(departure_events),
                                     std::cend(departure_events), begin)) -
      1;
  std::ptrdiff_t const upper =
      std::distance(std::cbegin(departure_events),
                    std::upper_bound(std::cbegin(departure_events),
                                     std::cend(departure_events), end)) -
      1;

  return std::pair(lower, upper);
}

template <class L, class MCRaptor>
inline std::vector<journey> raptor_gen(raptor_query<L>& q, raptor_statistics& stats,
                                       schedule const& sched,
                                       raptor_meta_info const& raptor_sched,
                                       raptor_timetable const& timetable,
                                       MCRaptor raptor) {
  reconstructor reconstructor(sched, raptor_sched, timetable);

  // Get departure range before we do the +1 query
  auto const& dep_events = q.use_start_metas_
                               ? raptor_sched.departure_events_with_metas_
                               : raptor_sched.departure_events_;
  auto const [lower, upper] = get_departure_range(
      q.source_time_begin_, q.source_time_end_, dep_events[q.source_]);

  stats.raptor_queries_ += 1;
  /*q.source_time_begin_ = q.source_time_end_ + 1;*/
  MOTIS_START_TIMING(plus_one_time);
  raptor.set_query_source_time(q.source_time_end_ + 1);
  raptor.invoke_cpu_raptor();
  stats.raptor_time_ += MOTIS_GET_TIMING_US(plus_one_time);

  MOTIS_START_TIMING(plus_one_rec_time);
  reconstructor.add(q);
  stats.rec_time_ += MOTIS_GET_TIMING_US(plus_one_rec_time);

  for (auto dep_idx = upper; dep_idx != lower; --dep_idx) {
    raptor.reset();
    stats.raptor_queries_ += 1;
    time new_query_time = dep_events[q.source_][dep_idx];
    q.source_time_begin_ = new_query_time;

    MOTIS_START_TIMING(raptor_time);
    raptor.set_query_source_time(new_query_time);
    raptor.invoke_cpu_raptor();
    stats.raptor_time_ += MOTIS_GET_TIMING_US(raptor_time);

    MOTIS_START_TIMING(rec_timing);
    reconstructor.add(q);
    stats.rec_time_ += MOTIS_GET_TIMING_US(rec_timing);
  }

  /*std::cout << "Lower" << lower * 60 + sched.schedule_begin_;
  std::cout << "\nUpper" << upper * 60 + sched.schedule_begin_ << "\n";
  std::cout << stats.raptor_queries_ << "\n";*/

  //return reconstructor.get_journeys(q.source_time_end_);
  return reconstructor.get_journeys();
}

inline std::vector<journey> cpu_raptor(base_query& bq,
                                       raptor_statistics& stats,
                                       schedule const& sched,
                                       raptor_meta_info const& raptor_sched,
                                       raptor_timetable const& tt) {
  if(bq.forward_) {
    raptor_query<label_departure> q =
        raptor_query<label_departure>{bq, raptor_sched, tt};
    return raptor_gen<label_departure, mc_raptor_departure>( q, stats, sched, raptor_sched, tt,mc_raptor_departure(q));
  }
  else {
    raptor_query<label_arrival> q =
        raptor_query<label_arrival>{bq, raptor_sched, tt};
    return raptor_gen<label_arrival, mc_raptor_arrival>(q, stats, sched, raptor_sched, tt, mc_raptor_arrival(q));
  }



}

#if defined(MOTIS_CUDA)
inline std::vector<journey> gpu_raptor(d_query& dq, raptor_statistics& stats,
                                       schedule const& sched,
                                       raptor_meta_info const& raptor_sched,
                                       raptor_timetable const& tt) {
  return raptor_gen(dq, stats, sched, raptor_sched, tt,
                    [&](d_query& dq) { return invoke_gpu_raptor(dq); });
}
#endif

}  // namespace motis::mcraptor
