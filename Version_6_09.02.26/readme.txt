In this version,
I developed a logic for Repeater where it will check whether the received packet is same as 
that received within 5 seconds.
Simply it means that it stores received packet in buffer.
Then if a same packet is received within 5 sec, it will suppress it and won't transmit it further

This prevented continuous toggling of packet between repeaters.

The funtion used to validate is "should_accept"-
static bool should_accept(const char *mac) {
    int64_t now = esp_timer_get_time() / 1000; // ms

    if(last_mac_valid && strcasecmp(last_mac,mac) ==0 && (now-last_mac_time)< SUPPRESSION_WINDOW){
        ESP_LOGW(TAG,"Duplicate MAC suppressed: %s", mac);
        return false;
    }

    //Update last mac
    strncpy(last_mac,mac,sizeof(last_mac));
    last_mac[sizeof(last_mac) - 1] = '\0';
    last_mac_time = now;
    last_mac_valid = true;

    return true;
}