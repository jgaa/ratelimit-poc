
#include "RateLimitSvc.h"

int main(int argc, char *argv[]) {

    auto server = rl::RateLimitSvc::create({});
    server->run();

    return 0;
}
