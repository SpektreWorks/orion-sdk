#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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

#define MAX_DISTANCE_IGNORE_MILES 20.0
#define MIN_DISTANCE_IGNORE_MILES 0.1894 //1000 feet

double distance(double lat0d, double lon0d, double lat1d, double lon1d);
static void KillProcess(const char *pMessage, int Value);
static void ProcessArgs(int argc, char **argv, int *pLevel);

static OrionPkt_t PktIn;

// Elevation tile level of detail - defaults to 12
static int TileLevel = 12;

int main(int argc, char **argv)
{
    int i,j;
    unsigned char sum = 0;
    char cksum[2];
    int sockfd;
    struct sockaddr_in servaddr;
    char buff[100];
    char lat[10];
    char lon[10];
    double latf,prev_latf = 0.0;
    double lonf,prev_lonf = 0.0;
    double d;
    int thistime;
    int lasttime = 0;


    // Process the command line arguments
    ProcessArgs(argc, argv, &TileLevel);

    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(15000);
    servaddr.sin_addr.s_addr = inet_addr("192.168.2.83");

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
                thistime = Geo.base.systemTime;
                //printf("time: %d\n",thistime);
                // If we got a valid target position from the gimbal
                if ((Geo.slantRange > 0) && (thistime > (lasttime + 10000)))
                {
                    lasttime = thistime;
                    printf("TARGET LLA: %10.6lf %11.6lf %6.1lf\r\n",
                           degrees(Geo.imagePosLLA[LAT]),
                           degrees(Geo.imagePosLLA[LON]),
                           Geo.imagePosLLA[ALT] - Geo.base.geoidUndulation);

                    latf = degrees(Geo.imagePosLLA[LAT]);
                    lonf = degrees(Geo.imagePosLLA[LON]);

                    d = distance(prev_latf,prev_lonf,latf,lonf);
                    if ( ((d > MIN_DISTANCE_IGNORE_MILES) && (d < MAX_DISTANCE_IGNORE_MILES)) ||
                         (prev_latf == 0.0) )
                    {
                        printf("distance: %f\n",d);
                        sprintf(lat,"%.4lf",latf);
                        sprintf(lon,"%.4lf",lonf);

                        i = 0;
                        buff[i++] = '$';
                        buff[i++] = 'S';
                        buff[i++] = 'W';
                        buff[i++] = ',';
                        for(j=0;j<strlen(lat);j++)
                        {
                            buff[i++] = lat[j];
                        }
                        buff[i++] = ',';
                        for(j=0;j<strlen(lon);j++)
                        {
                            buff[i++] = lon[j];
                        }
                        buff[i++] = ',';

                        sum = 0;
                        for(j=0;j<i;j++)
                        {
                            sum += buff[j];
                        }
                        sprintf(cksum,"%x",sum);
                        buff[i++] = cksum[0];
                        buff[i++] = cksum[1];
                        buff[i] = 0;

                        printf("buff: %s\n",buff);

                        sendto(sockfd,buff,i,0,(struct sockaddr*)&servaddr,sizeof(servaddr));

                        prev_latf = latf;
                        prev_lonf = lonf;
                    }
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

double distance(double lat0d, double lon0d, double lat1d, double lon1d)
{
    //Given a pair lats and lons in degrees, outputs the distance between them in miles
    double lat0 = lat0d * 3.14159/180.0; //degrees to radians
    double lon0 = lon0d * 3.14159/180.0;
    double lat1 = lat1d * 3.14159/180.0;
    double lon1 = lon1d * 3.14159/180.0;

    double term1, term2, term3, term4;

    term1 = sin((lat0-lat1)/2) * sin((lat0-lat1)/2);
    term2 = cos(lat0) * cos(lat1) * sin((lon0-lon1)/2) * sin((lon0-lon1)/2);
    term3 = asin(sqrt(term1 + term2));
    term4 = 2 * 6378.137 * term3; //distance in km

    return term4 * 0.62137; //convert km to miles
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
