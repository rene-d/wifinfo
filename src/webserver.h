// module téléinformation client
// rene-d 2020

#pragma once

enum AccessType
{
    NO_ACCESS,  // mot de passe et pas authentifié
    RESTRICTED, // mot de passe et derrière un reverse proxy
    FULL,       // pas de mot de passe ou mot de passe et pas derrière un reverse proxy
};

void webserver_setup();
void webserver_loop();
AccessType webserver_get_auth();
bool webserver_access_full();
bool webserver_access_ok();
