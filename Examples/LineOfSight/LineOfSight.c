#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "LineOfSight.h"
#include "GeolocateTelemetry.h"
#include "OrionComm.h"

#define TILE_CACHE 25
// #define DEBUG

#ifdef DEBUG
# define DEBUG_PRINT printf
#else
# define DEBUG_PRINT(...)
#endif

static void KillProcess(const char *pMessage, int Value);
static void ProcessArgs(int argc, char **argv, int *pLevel);

static OrionPkt_t PktIn;

// Elevation tile level of detail - defaults to 12
static int TileLevel = 12;

int main(int argc, char **argv)
{
    // Process the command line arguments
    ProcessArgs(argc, argv, &TileLevel);

    // Loop forever
    while (1)
    {
        GeolocateTelemetry_t Geo;

        // Pull all queued up packets off the comm interface
        while (OrionCommReceive(&PktIn))
        {
            // If this packet is a geolocate telemetry packet
            if (DecodeGeolocateTelemetry(&PktIn, &Geo))
            {
                //double TargetLla[NLLA]
                double Range;

                // If we got a valid target position from the gimbal
                if (Geo.slantRange > 0)
                {
                    Range = Geo.slantRange;
                    printf("TARGET LLA: %10.6lf %11.6lf %6.1lf, RANGE: %6.0lf\r",
                           degrees(Geo.imagePosLLA[LAT]),
                           degrees(Geo.imagePosLLA[LON]),
                           Geo.imagePosLLA[ALT] - Geo.base.geoidUndulation,
                           Range);
                }

            }
        }

        // Sleep for 20 ms so as not to hog the entire CPU
        fflush(stdout);
        usleep(20000);
    }

    // Finally, be done!
    return 0;
}

// This function just shuts things down consistently with a nice message for the user
static void KillProcess(const char *pMessage, int Value)
{
    // Print out the error message that got us here
    printf("%s\n", pMessage);
    fflush(stdout);

    // Close down the active file descriptors
    OrionCommClose();

    // Finally exit with the proper return value
    exit(Value);

}// KillProcess

static void ProcessArgs(int argc, char **argv, int *pLevel)
{
    char Error[80];

    // If we can't connect to a gimbal, kill the app right now
    if (OrionCommOpen(&argc, &argv) == FALSE)
        KillProcess("", 1);

    // Use a switch with fall-through to overwrite the default geopoint
    switch (argc)
    {
    case 2: *pLevel = atoi(argv[1]);         // Tile level of detail
    case 1: break;                           // Serial port path
    // If there aren't enough arguments
    default:
        // Kill the application and print the usage info
        sprintf(Error, "USAGE: %s [/dev/ttyXXX] [LoD]", argv[0]);
        KillProcess(Error, -1);
        break;
    };

}// ProcessArgs
