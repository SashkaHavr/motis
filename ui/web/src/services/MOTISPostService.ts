import axios from 'axios'
import { App } from 'vue'
import { StationGuessResponseContent } from '../models/StationGuess';
import { AddressGuessResponseContent } from '../models/AddressGuess';
import Trip from '../models/Trip';
import TripResponseContent from '../models/TripResponseContent';
import { RailVizStationResponseContent } from '../models/DepartureTimetable';
import { TrainGuessResponseContent } from '../models/TrainGuess'
import ConnectionResponseContent, { ConnectionRequestContent } from "../models/ConnectionContent"
import InitialScheduleInfoResponseContent from "../models/InitRequestResponseContent"

const apiEndPoint = "http://localhost:8081/";

/* eslint-disable camelcase*/
const service: MOTISPostService = {
  async getStationGuessResponse(input: string, gc: number) {
    const rq = {
      destination: {
        type: "Module",
        target: "/guesser"
      },
      content_type: "StationGuesserRequest",
      content: {
        input: input,
        guess_count: gc
      }
    };
    return (await axios.post<StationGuessResponse>(apiEndPoint, rq)).data.content;
  },
  async getAddressGuessResponse(input: string) {
    const rq = {
      destination: {
        type: "Module",
        target: "/address"
      },
      content_type: "AddressRequest",
      content: {
        input: input
      }
    };
    return (await axios.post<AddressGuessResponse>(apiEndPoint, rq)).data.content;
  },
  async getTripResponce(trip: Trip) {
    const rq = {
      destination: {
        type: "Module",
        target: "/trip_to_connection"
      },
      content_type: "TripId",
      content: trip
    };
    return (await axios.post<TripResponce>(apiEndPoint, rq)).data.content;
  },
  async getDeparturesResponse(station: string, byScheduleTime: boolean, direction: ("BOTH" | "EARLIER" | "LATER"), eventCount: number, time: number) {
    const rq = {
      destination: {
        type: "Module",
        target: "/railviz/get_station"
      },
      content_type: "RailVizStationRequest",
      content: {
        by_schedule_time: byScheduleTime,
        direction: direction,
        event_count: eventCount,
        station_id: station,
        time: time
      },
    };
  return (await axios.post<RailVizStationResponse>(apiEndPoint, rq)).data.content;
  },
  async getTrainGuessResponse(currentTime: number, currentTrainNum: number){
    const rq = {
        destination: {
            target: "/railviz/get_trip_guesses",
            type: "Module"
        },
        content_type: "RailVizTripGuessRequest",
        content: {
            guess_count: 20,
            time: currentTime,
            train_num: currentTrainNum
        },
    }
    return (await axios.post<TrainGuessResponse>(apiEndPoint, rq)).data.content;
  },
  async getConnectionResponse(connectionRequest: ConnectionRequestContent, smthing: boolean){
    const rq = {
        destination: {
          target: "/intermodal",
          type: "Module"
        },
        content_type: "IntermodalRoutingRequest",
        content: {
          ...connectionRequest,
          search_type: "Accessibility",
          search_dir: "Forward"
        },
    }
    return (await axios.post<ConnectionResponse>(apiEndPoint, rq)).data.content;
  },
  async getConnectionResponseRaptorMcraptor(connectionRequest: ConnectionRequestContent, mcRaptor: boolean){
    const rq = {
      "destination": {
          "type": "Module",
          "target": mcRaptor ? "/mcraptor" : "/raptor"
      },
      "content_type": "RoutingRequest",
      "content": {
          "start_type": connectionRequest.start_type,
          "start": connectionRequest.start,
          "destination": connectionRequest.destination,
          "search_type": "Accessibility",
          "search_dir": "Forward",
          "via": [],
          "additional_edges": [],
          "use_start_metas": false,
          "use_dest_metas": false,
          "use_start_footpaths": false,
          "schedule": 0
      }
    }
    return (await axios.post<ConnectionResponse>(apiEndPoint, rq)).data.content;
  },
  async getInitialRequestScheduleInfo(){
    const rq = {
      content: {},
      content_type: "MotisNoMessage",
      destination: {
        target: "/lookup/schedule_info",
        type: "Module"
      }
    }
    return (await axios.post<InitialScheduleInfoResponse>(apiEndPoint, rq)).data.content;
  }
}

interface StationGuessResponse {
  destination: {
    type: string,
    target: string
  },
  content_type: string,
  content: StationGuessResponseContent,
  id: number
}

interface AddressGuessResponse {
  destination: {
    target: string
  },
  content_type: string,
  content: AddressGuessResponseContent,
  id: number
}

interface TripResponce {
  destination: {
    type: string,
    target: string
  },
  content_type: string,
  content: TripResponseContent,
  id: number
}

interface RailVizStationResponse {
  destination: {
    type: string,
    target: string
  },
  content_type: string,
  content: RailVizStationResponseContent,
  id: number
}

interface TrainGuessResponse {
  destination: {
      type: string,
      target: string
  },
  content_type: string,
  content: TrainGuessResponseContent,
  id: number
}

interface ConnectionResponse {
  destination: {
    type: string,
    target: string
  },
  content_type: string,
  content: ConnectionResponseContent,
}

interface InitialScheduleInfoResponse{
  destination: {
    type: string,
    target: string
  },
  content_type: string,
  content: InitialScheduleInfoResponseContent,
  id: number
}

/* eslint-enable camelcase*/

interface MOTISPostService {
  getStationGuessResponse(input: string, gc: number): Promise<StationGuessResponseContent>
  getAddressGuessResponse(input: string): Promise<AddressGuessResponseContent>
  getTripResponce(input: Trip) : Promise<TripResponseContent>
  getDeparturesResponse(station: string, byScheduleTime: boolean, direction: string, eventCount: number, time: number) : Promise<RailVizStationResponseContent>
  getTrainGuessResponse(currentTime: number, currentTrainNum: number): Promise<TrainGuessResponseContent>
  getConnectionResponse(connectionRequest: ConnectionRequestContent, smthing: boolean): Promise<ConnectionResponseContent>
  getInitialRequestScheduleInfo(): Promise<InitialScheduleInfoResponseContent>,
  getConnectionResponseRaptorMcraptor(connectionRequest: ConnectionRequestContent, mcRaptor: boolean) : Promise<ConnectionResponseContent>
}


















declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $postService: MOTISPostService
  }
}

export default {
  install: (app: App) : void => {
    app.config.globalProperties.$postService = service;
  }
}
