#include "motis/gbfs_new_valhalla/gbfs_new_valhalla.h"

#include "motis/core/common/logging.h"
#include "motis/core/schedule/schedule.h"
#include "motis/core/conv/position_conv.h"
#include "motis/core/conv/station_conv.h"
#include "motis/module/context/motis_call.h"
#include "motis/module/context/motis_http_req.h"
#include "motis/module/context/motis_parallel_for.h"
#include "motis/module/event_collector.h"
#include "motis/module/message.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "net/http/client/client.h"
#include "geo/point_rtree.h"

#include <fstream>

namespace fbs = flatbuffers;
using namespace motis::logging;
using namespace motis::module;
using namespace motis::gbfs;

namespace motis::gbfs_new_valhalla {

struct positions {};

struct gbfs_new_valhalla::impl {
  explicit impl(config const& c)
      : config_{c} {}

  void create_public_transport_stations_file(schedule const& sched) {
    auto& stations = sched.stations_;
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    rapidjson::Value locations(rapidjson::kArrayType);
    for(const auto& s : stations) {
      rapidjson::Value location(rapidjson::kObjectType);
      location.AddMember("lat", s->lat(), allocator);
      location.AddMember("lon", s->lng(), allocator);
      location.AddMember("gbfs_transport_station_id", s->index_, allocator);
      locations.PushBack(location.Move(), allocator);
    }
    document.AddMember("locations", locations.Move(), allocator);

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    std::string request_string = buffer.GetString();

    std::ofstream file("stations_file.json");
    file << request_string;
    file.close();
  }

  void init(schedule const& sched) {
    pt_stations_rtree_ = geo::make_point_rtree(sched.stations_, [](auto const& s) {
        return geo::latlng{s->lat(), s->lng()};
      });
    if(config_.create_stations_file_on_init) {
      create_public_transport_stations_file(sched);
    }
    LOG(logging::info) << "GBFS initialized";
  }

  inline http_future_t send_request(std::string url, std::string body) {
    return motis_http(net::http::client::request(url, net::http::client::request::method::GET, net::http::client::request::str_map(), body));
  }

  msg_ptr route(schedule const& sched, msg_ptr const& m) {
    auto const req = motis_content(GBFSRoutingRequest, m);

    auto const duration = req->max_bike_duration() * 60;
    auto const dist = req->max_bike_duration() * 60 * 10; // use bike duration as total time for now
    auto const x = from_fbs(req->x());
    auto const stations = pt_stations_rtree_.in_radius(x, dist);
    std::string dir = req->dir() == SearchDir_Forward ? "Forward" : "Backward";
    std::string request_string;
    {
      rapidjson::Document document;
      document.SetObject();
      rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
      rapidjson::Value locations(rapidjson::kArrayType);
      rapidjson::Value start_location(rapidjson::kObjectType);
      start_location.AddMember("lat", x.lat_, allocator);
      start_location.AddMember("lon", x.lng_, allocator);
      locations.PushBack(start_location.Move(), allocator);
      document.AddMember("locations", locations.Move(), allocator);

      rapidjson::Value stations_json(rapidjson::kArrayType);
      for (const auto s : stations) {
        const auto& station = sched.stations_[s];
        rapidjson::Value location(rapidjson::kObjectType);
        location.AddMember("lat", station->lat(), allocator);
        location.AddMember("lon", station->lng(), allocator);
        location.AddMember("gbfs_transport_station_id", s, allocator);
        stations_json.PushBack(location.Move(), allocator);
      }
      document.AddMember("targets", stations_json.Move(), allocator);
      document.AddMember("is_forward", req->dir() == SearchDir_Forward, allocator);
      document.AddMember("gbfs_max_duration", duration, allocator);

      rapidjson::StringBuffer buffer;
      buffer.Clear();
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      document.Accept(writer);
      request_string = buffer.GetString();
    }

    auto gbfs_route_response = send_request(config_.gbfs_route_url(), request_string);
    std::string route_str = gbfs_route_response->val().body;

    std::string result_str;
    std::vector<std::string> temp(stations.size());
    {
      rapidjson::Document document;
      document.Parse(route_str.c_str());
      rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
      for (auto& route : document["routes"].GetArray()) {
        size_t station_id = route["p"]["id"].GetUint64();
        const auto& station = sched.stations_[station_id];
        route["p"].AddMember("name", rapidjson::StringRef(station->name_.data()), allocator);
        rapidjson::Value pos(rapidjson::kObjectType);
        pos.AddMember("lat", station->lat(), allocator);
        pos.AddMember("lng", station->lng(), allocator);
        route["p"].AddMember("pos", pos.Move(), allocator);
        route["p"].EraseMember("id");
        std::string id_str = std::to_string(station_id);
        temp.push_back(id_str);
        route["p"].AddMember("id", rapidjson::StringRef(temp.back().c_str()), allocator);
      }
      document.AddMember("dir", rapidjson::StringRef(dir.c_str()), allocator);

      rapidjson::Document r_document;
      r_document.SetObject();
      rapidjson::Document::AllocatorType& r_allocator = document.GetAllocator();
      rapidjson::Value dest(rapidjson::kObjectType);
      dest.AddMember("type", "Module", r_allocator);
      dest.AddMember("target", "/gbfs_new_valhalla/route", r_allocator);
      r_document.AddMember("destination", dest.Move(), r_allocator);
      r_document.AddMember("content_type", "GBFSRoutingResponse", r_allocator);
      r_document.AddMember("content", document.Move(), r_allocator);

      rapidjson::StringBuffer buffer;
      buffer.Clear();
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      r_document.Accept(writer);
      result_str = buffer.GetString();
    }

    return make_msg(result_str, true);
  }

  config const& config_;
  geo::point_rtree pt_stations_rtree_;
};

gbfs_new_valhalla::gbfs_new_valhalla() : module("gbfs_new_valhalla", "gbfs_new_valhalla") {
  param(config_.valhalla_server_url, "valhalla_server_url",
        "Url of valhalla server");
  param(config_.create_stations_file_on_init, "create_stations_file_on_init",
        "Create file with station ids and lat,lng when module is initialized");
}

gbfs_new_valhalla::~gbfs_new_valhalla() = default;

void gbfs_new_valhalla::import(import_dispatcher& reg) {
  using import::OSRMEvent;
  std::make_shared<event_collector>(
      get_data_directory().generic_string(), "gbfs_new_valhalla", reg,
      [this](event_collector::dependencies_map_t const&,
             event_collector::publish_fn_t const&) {
        import_successful_ = true;
      })
      ->require("SCHEDULE",
                [](msg_ptr const& msg) {
                  return msg->get()->content_type() == MsgContent_ScheduleEvent;
                });
}

void gbfs_new_valhalla::init(motis::module::registry& r) {
  impl_ = std::make_unique<impl>(config_);

  r.subscribe("/init", [&]() { impl_->init(get_sched()); }, {});

  r.register_op("/gbfs_new_valhalla/route", [&](msg_ptr const& m) { return impl_->route(get_sched(), m); },
                {kScheduleReadAccess, {to_res_id(global_res_id::GBFS_DATA), ctx::access_t::READ}});
}

}  // namespace motis::gbfs_new_valhalla
