#pragma once
#include <httplib.h>
#include <memory>

#include "services/AuthService.h"

namespace rtrs {

void registerAdminRoutes(httplib::Server& server, std::shared_ptr<AuthService> auth);

}
