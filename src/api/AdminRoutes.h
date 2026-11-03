#pragma once
#include <httplib.h>
#include <memory>

#include "services/AuthService.h"
#include "services/TrainService.h"

namespace rtrs {

void registerAdminRoutes(httplib::Server& server, Database& db, std::shared_ptr<AuthService> auth,
                         std::shared_ptr<TrainService> trains);

}
