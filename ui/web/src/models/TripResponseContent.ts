/* eslint-disable camelcase */
import CustomMovement from "./CustomMovement";
import Attribute from "./SmallTypes/Attribute";
import FreeText from "./SmallTypes/FreeText";
import Problem from "./SmallTypes/Problem";
import Stop from "./Stop";
import Transport from "./Transport";
import Trip from "./Trip";
import Range from "./SmallTypes/Range";

export default interface TripResponseContent {
  stops: Stop[],
  transports: Move[],
  trips: {
    range: Range,
    id: Trip
  }[],
  attributes: Attribute[],
  free_texts: FreeText[],
  problems: Problem[],
  night_penalty: number,
  db_costs: number,
  status: string
}


export interface Move {
  move_type: "Walk" | "Transport",
  move: Transport | CustomMovement
}