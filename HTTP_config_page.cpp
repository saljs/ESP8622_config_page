#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <EEPROM.h>
#include "HTTP_config_page.h"

HTTP_config_page::HTTP_config_page(const char* title) {
    name = title;
    settings = NULL;
    next = 1;
}

void HTTP_config_page::addField(const char* n, uint8_t t) {
    config_field* newField = (config_field*)malloc(sizeof(config_field));
    newField->name = n;
    newField->type = t;
    newField->def = "false";
    newField->address = next;
    
    header location;
    EEPROM.get(newField->address, location);
    if(location.nextAddress == 0 || EEPROM.read(0) != MAGIC_BYTE) {
        //the node has no next
        next = 0;
    }
    else {
        next = location.nextAddress;
    }
    if(settings == NULL) {
        settings = newField;
        newField->last = NULL;
        return;
    }
    config_field* tmp = settings;
    while(tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = newField;
    newField->last = tmp;
}
void HTTP_config_page::addField(const char* n, uint8_t t, const char* d) {
    config_field* newField = (config_field*)malloc(sizeof(config_field));
    newField->name = n;
    newField->type = t;
    newField->def = d;
    newField->address = next;
    
    header location;
    EEPROM.get(newField->address, location);
    if(location.nextAddress == 0 || EEPROM.read(0) != MAGIC_BYTE) {
        //the node has no next
        next = 0;
    }
    else {
        next = location.nextAddress;
    }
    if(settings == NULL) {
        settings = newField;
        newField->last = NULL;
        return;
    }
    config_field* tmp = settings;
    while(tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = newField;
    newField->last = tmp;
}

void HTTP_config_page::getField(const char* field, char* buffer) {
    config_field* tmp = settings;
    while(tmp != NULL) {
        if(strcmp(field, tmp->name) == 0) {
            getField(tmp, buffer);
            return;
        }
        tmp = tmp->next;
    }
}
void HTTP_config_page::getField(config_field* field, char* buffer) {
    if(field->address == 0 || EEPROM.read(0) != MAGIC_BYTE) {
        strcpy(buffer, field->def);
        return;
    }
    header location;
    EEPROM.get(field->address, location);
    int i;
    for(i = 0; i < location.length; i++) {
        buffer[i] = EEPROM.read(field->address+sizeof(header)+i);
    }
    buffer[i] = '\0';
}

void HTTP_config_page::setField(const char* field, const char* value) {
    config_field* tmp = settings;
    while(tmp != NULL) {
        if(strcmp(field, tmp->name) == 0) {
            saveField(tmp, value);
            return;
        }
        tmp = tmp->next;
    }
}

void HTTP_config_page::saveField(config_field* field, const char* data) {
    if(EEPROM.read(0) != MAGIC_BYTE) {
        EEPROM.write(0, MAGIC_BYTE);
    }
    const unsigned short len = sizeof(header) + strlen(data) + field->address;
    bool needsDefrag = false;
    if(field->address == 0) {
        //assign the addres to the farthest in the list
        config_field* tmp = settings;
        while(tmp != NULL) {
            if(tmp->address > field->address) {
                field->address = tmp->address;
            }
            tmp = tmp->next;
        }
        header tmpLoc;
        EEPROM.get(field->address, tmpLoc);
        field->address += sizeof(header) + tmpLoc.length;
        
        //write out the address to the last node
        EEPROM.get(field->last->address, tmpLoc);
        tmpLoc.nextAddress = field->address;
        EEPROM.put(field->last->address, tmpLoc);
    }
    else if(field->next != NULL && len > field->next->address && field->next->address > 0) {
        //shift the data over to make room
        needsDefrag = !moveOffset(field->next, len - field->next->address);
    }
    
    if(len >= EEPROM.length() || needsDefrag) {
        //we've run out of room, try defragmenting
        if(defragment()) {
            saveField(field, data);
        }
        return;
    }
    
    header location;
    location.nextAddress = 0;
    if(field->next != NULL) {
        location.nextAddress = field->next->address;
    }
    location.length = strlen(data);
    //do the save
    EEPROM.put(field->address, location);
    for(int i = 0; i < location.length; i++) {
        if(EEPROM.read(field->address+sizeof(header)+i) != data[i]) {
            EEPROM.write(field->address+sizeof(header)+i, data[i]);
        }
    }
    EEPROM.commit();    
}

bool HTTP_config_page::moveOffset(config_field* field, unsigned short n) {
    header location;
    EEPROM.get(field->address, location);
    const unsigned short len = sizeof(header) + location.length + field->address + n;
    if(len >= EEPROM.length()) {
        return false;
    }
    else if(field->next != NULL && len > field->next->address && field->next->address > 0) {
        //move over the next field as well
        bool noDefrag = moveOffset(field->next, n);
        if(!noDefrag) {
            return false;
        }
    }
    //move the data over n places
    const unsigned short start = sizeof(header) + field->address + location.length - 1;
    for(unsigned short i = start; i >= field->address; i--) {
        EEPROM.write(i+n, EEPROM.read(i));
    }
    //save new address
    field->address += n;
    //update last field
    header last;
    EEPROM.get(field->last->address, last);
    last.nextAddress = field->address;
    EEPROM.put(field->last->address, last);
    EEPROM.commit();
    return true;
}
    
bool HTTP_config_page::defragment() {
    bool hasShifted = false;
    config_field* tmp = settings;
    while(tmp != NULL) {
        if(tmp->last != NULL) {
            header location;
            EEPROM.get(tmp->last->address, location);
            const unsigned short end = sizeof(header) + location.length + tmp->last->address;
            if(end < tmp->address) {
                //shift the data over
                location.nextAddress = end;
                EEPROM.put(tmp->last->address, location);
                EEPROM.get(tmp->address, location);
              
                for(int i = 0; i < sizeof(header) + location.length; i++) {
                    EEPROM.write(end+i, EEPROM.read(tmp->address+i));
                }
                tmp->address = end;
                EEPROM.commit();
                hasShifted = true;
            }
        }
        tmp = tmp->next;
    }
    
    return hasShifted;
}

unsigned short HTTP_config_page::getFreeBytes() {
    if(EEPROM.read(0) != MAGIC_BYTE) {
        return EEPROM.length();
    }
    unsigned short taken = 0;
    config_field* tmp = settings;
    while(tmp != NULL) {
        if(tmp->address > 0) {
            header location;
            EEPROM.get(tmp->address, location);
            taken += location.length + sizeof(header);
        }
        tmp = tmp->next;
    }
    return EEPROM.length() - taken;
}
    
void HTTP_config_page::parse(ESP8266WebServer* server) {
    if(server->method() == HTTP_POST) {
        //process all the vars
        config_field* tmp = settings;
        while(tmp != NULL) {
            String val = server->arg(String(tmp->name));
            if(val != "") {
                saveField(tmp, ESP8266WebServer::urlDecode(val).c_str());
            }
            else {
                saveField(tmp, tmp->def);
            }
            tmp = tmp->next;
        }
    }
    String html = "<!DOCTYPE html><html><head><title>" + String(name) + "</title></head><body><h1>" + String(name) + "</h1><form method=\"post\">";
    config_field* tmp = settings;
    while(tmp != NULL) {
        header location;
        if(tmp->address == 0 || EEPROM.read(0) != MAGIC_BYTE) {
            location.length = 1;
        }
        else {
            EEPROM.get(tmp->address, location);
        }
        char val[location.length];
        getField(tmp, val);
        if(tmp->type == TEXTBOX) {
            html += String(tmp->name) + "<input name=\"" + String(tmp->name) + "\" type=\"text\" value=\"" + String(val) + "\"><br/>";
        }
        else if(tmp->type == CHECKBOX) {
            if(strcmp(val, "true") == 0) {
                html += String(tmp->name) + ":<br> &#8226;True<input name=\"" + String(tmp->name) + "\" type=\"radio\" value=\"true\" checked=\"checked\"><br /> &#8226;False<input name=\"" + String(tmp->name) + "\" type=\"radio\" value=\"false\"><br />";
            }
            else {
                html += String(tmp->name) + ":<br> &#8226;True<input name=\"" + String(tmp->name) + "\" type=\"radio\" value=\"true\"><br /> &#8226;False<input name=\"" + String(tmp->name) + "\" type=\"radio\" value=\"false\" checked=\"checked\"><br />";
            }
        }
        tmp = tmp->next;
    }
    html += "<input type=\"submit\" value=\"Save\"></form></body></html>";
    server->send(200, "text/html", html);
}

HTTP_config_page::~HTTP_config_page() {
    config_field* tmp = settings;
    config_field* next;
    while(tmp != NULL) {
        next = tmp->next;
        free(tmp);
    }
}
