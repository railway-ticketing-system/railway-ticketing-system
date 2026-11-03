#pragma once
#include "services/TrainService.h"

namespace rtrs {

struct TrainInput {
    std::string number, name;
    std::vector<Stop> stops;
    std::vector<TrainClass> classes;
};

struct ManagedTrain {
    long long id = 0;
    bool active = true;
    TrainInput details;
};

struct TrainWriteResult {
    int status = 400;
    FieldErrors errors;
    ManagedTrain train;
};

class AdminTrainService {
public:
    AdminTrainService(Database& db, TrainService& trains) : db_(db), trains_(trains) {}
    std::vector<ManagedTrain> list();
    std::optional<ManagedTrain> findById(long long id);
    TrainWriteResult create(TrainInput input);
    TrainWriteResult update(long long id, TrainInput input);
    TrainWriteResult deactivate(long long id);

private:
    void insertStops(long long id, const TrainInput& input);
    Database& db_;
    TrainService& trains_;
};

}
