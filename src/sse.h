/*
 * Server-sent Events (SSE) for ESP8266
 * Copyright (c) 2020 rene-d. All right reserved.
 */

#pragma once

#include "webserver.h"

#include <ESP8266WebServer.h>
#include <list>
#include <user_interface.h>

class SseClient
{
    WiFiClient client_;

public:
    explicit SseClient(ESP8266WebServer &server)
    {
        // récupère le _currentClient : comme l'application est monothreadée
        // c'est forcément celui qui déclenché la callback on()
        client_ = server.client();

        if (client_)
        {
            Serial.printf_P(PSTR("new client %p\n"), this);

            client_.println(F("HTTP/1.1 200 OK"));
            client_.println(F("Content-Type: text/event-stream;charset=UTF-8"));
            client_.println(F("Connection: close"));              // the connection will be closed after completion of the response
            client_.println(F("Access-Control-Allow-Origin: *")); // allow any connection. We don't want Arduino to host all of the website ;-)
            client_.println(F("Cache-Control: no-cache"));        // refresh the page automatically every 5 sec
            client_.println();
            client_.flush();
        }
    }

    virtual ~SseClient()
    {
        // give the web browser time to receive the data
        delay(1);
        // close the connection:
        client_.stop();
        Serial.printf_P(PSTR("client %p disconnected\n"), this);
    }

    bool connected()
    {
        return client_.connected();
    }

    void send_event(const String &data)
    {
        client_.print("data: ");
        client_.print(data);
        client_.print("\r\n\r\n");

        client_.flush();
    }

    String remote() const
    {
        // lack of const specifier in the SDK
        String s = const_cast<WiFiClient &>(client_).remoteIP().toString();
        s += ":";
        s += const_cast<WiFiClient &>(client_).remotePort();
        return s;
    }
};

class SseClients
{
    std::list<SseClient *> clients_;

public:
    void on(const String &uri, ESP8266WebServer &server)
    {
        server.on(uri, [&server, this] {
            if (webserver_access_ok())
            {
                this->handle_sse_data(server);
            }
        });
    }

    void handle_sse_data(ESP8266WebServer &server)
    {
        if (clients_.size() < 2)
        {
            // if (clients_.size() == 0)
            // {
            //     // Set CPU speed to 160MHz
            //     system_update_cpu_freq(160);
            // }
            clients_.push_back(new SseClient(server));
        }
        else
        {
            server.send(503, mime::mimeTable[mime::txt].mimeType, "Service Unavailable (too many clients)");
        }
    }

    size_t count() const
    {
        return clients_.size();
    }

    String remotes() const
    {
        String s;
        for (const auto &it : clients_)
        {
            s += it->remote() + " ";
        }
        return s;
    }

    void handle_clients(const String *send_data = nullptr)
    {
        if (clients_.empty())
        {
            // nothing to do
            return;
        }

        auto it = clients_.begin();
        auto end = clients_.end();

        while (it != end)
        {
            if ((*it)->connected())
            {
                if (send_data)
                {
                    (*it)->send_event(*send_data);
                }
                ++it;
            }
            else
            {
                delete *it;
                it = clients_.erase(it);

                // if (clients_.size() == 0)
                // {
                //     // back to 80MHz
                //     system_update_cpu_freq(80);
                // }
            }
        }
    }
};
