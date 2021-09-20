import React, { useState } from "react";

import { TripId } from "../api/protocol/motis";
import { PaxMonEdgeLoadInfoWithStats } from "../data/loadInfo";

import TripLoadForecastChart from "./TripLoadForecastChart";
import TripSectionDetails from "./TripSectionDetails";

type TripDetailsProps = {
  tripId: TripId;
};

function TripDetails({ tripId }: TripDetailsProps): JSX.Element {
  const [selectedSection, setSelectedSection] =
    useState<PaxMonEdgeLoadInfoWithStats | null>(null);

  return (
    <div>
      <TripLoadForecastChart
        tripId={tripId}
        mode="Interactive"
        onSectionClick={setSelectedSection}
      />
      <TripSectionDetails tripId={tripId} selectedSection={selectedSection} />
    </div>
  );
}

export default TripDetails;