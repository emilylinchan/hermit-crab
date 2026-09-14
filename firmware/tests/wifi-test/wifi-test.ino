#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_netif.h"

// Network
const char* ssid           = "HermitCrab"; // Network name
const char* password       = "12345678";   // Set to NULL to have an open AP
const bool  hide_SSID      = false;        // Disable SSID broadcast
const int   max_connection = 2;            // Maximum simultaneous connected clients on the AP

// Configure IP address
IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

void display_connected_devices()
{
    wifi_sta_list_t wifi_sta_list;
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    if (wifi_sta_list.num > 0)
        Serial.println("-----------");

    for (uint8_t i = 0; i < wifi_sta_list.num; i++)
    {
        wifi_sta_info_t station = wifi_sta_list.sta[i];
        Serial.printf("[+] Device %d | MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
            i,
            station.mac[0], station.mac[1], station.mac[2],
            station.mac[3], station.mac[4], station.mac[5]);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n[*] Creating AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ssid, password, hide_SSID, max_connection);
    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
}

void loop()
{
    display_connected_devices();
    delay(5000);
}

// From PC: type in terminal "ping 192.168.0.1"
