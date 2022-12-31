import QtQuick 2.3
import QtTest 1.0

import org.qfield 1.0

import QFieldControls 1.0

TestCase {
    name: "Positioning"

    Positioning {
        id: positioning
        deviceId: 'udp:localhost:1958'
        active: true
        ellipsoidalElevation: true

        coordinateTransformer: CoordinateTransformer {
          id: coordinateTransformer
          destinationCrs: CoordinateReferenceSystemUtils.wgs84Crs()
          transformContext: CoordinateReferenceSystemUtils.emptyTransformContext()
          deltaZ: 0
          skipAltitudeTransformation: false
          verticalGrid: ''
        }
    }

    function test_ellipsoidalElevation() {
        coordinateTransformer.deltaZ = 0
        coordinateTransformer.verticalGrid = ''
        // wait a few seconds so positioning can catch some NMEA strings
        wait(2500)

        // the elevation in the NMEA stream goes between 320 to 322, and the ellispodal adjustment is -26.0 meters
        // we therefore simply check whether we are int the 200s value range, which indicates ellispodal elevation is
        // being returned
        compare(Math.floor(positioning.positionInformation.elevation / 100), 2)
    }

    function test_deltaZ() {
        coordinateTransformer.deltaZ = -100
        coordinateTransformer.verticalGrid = ''
        // wait a few seconds so positioning can catch some NMEA strings
        wait(2500)

        // the elevation in the NMEA stream's range is in the 290s
        // we therefore simply check whether we are in the 100s value range, which indicates
        // the delta Z value has been applied
        compare(Math.floor(positioning.projectedPosition.z / 100), 1)
    }

    function test_verticalGrid() {
        coordinateTransformer.deltaZ = 0
        coordinateTransformer.verticalGrid = dataDir + '/testgrid.tif'
        // wait a few seconds so positioning can catch some NMEA strings
        wait(2500)

        // the elevation in the NMEA stream's range is in the 290s
        // we therefore simply check whether we have a negative value to see
        // if the grid value (~300) has been applied
        compare(Math.floor(positioning.projectedPosition.z / 100), -1)
    }
}