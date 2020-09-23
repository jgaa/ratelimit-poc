#ifndef RATELIMITSVC_H
#define RATELIMITSVC_H

#include <memory>
#include <string>

namespace rl {

struct Config {
    std::string listenAddress = "0.0.0.0:5000";
};

class RateLimitSvc
{
public:
    RateLimitSvc() = default;
    virtual ~RateLimitSvc() = default;

    virtual void run() = 0;

    static std::unique_ptr<RateLimitSvc> create(const Config& config);
};

}// ns

#endif // RATELIMITSVC_H
