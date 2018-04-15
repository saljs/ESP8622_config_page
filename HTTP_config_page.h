#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#define MAGIC_BYTE 0xAB
#define TEXTBOX 1
#define CHECKBOX 2

class HTTP_config_page {
public:
    HTTP_config_page(const char* title);
    void addField(const char* n, uint8_t t);
    void addField(const char* n, uint8_t t, const char* d);
    void getField(const char* field, char* buffer);
    void parse(ESP8266WebServer* server);
    void setField(const char* field, const char* value);
    unsigned short getFreeBytes();
    ~HTTP_config_page();
private:
    typedef struct header {
        unsigned short length;
        unsigned short nextAddress;
    } header;
    typedef struct config_field {
        const char* name;
        const char* def;
        uint8_t type;
        unsigned short address;
        config_field* next;
        config_field* last;
    } config_field;
    
    config_field* settings;
    const char* name;
    unsigned short next;
    
    bool defragment();
    void saveField(config_field* field, const char* data);
    bool moveOffset(config_field* field, unsigned short n);
    void getField(config_field* field, char* buffer);
};
