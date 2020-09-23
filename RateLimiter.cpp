#include "RateLimiter.h"
#include <math.h>
#include <iostream>

using namespace  std;

namespace rl {

unique_ptr<RateLimiter::Bucket> RateLimiter::createBucket() const
{
    return make_unique<Bucket>(maxTokens_, *this);
}

bool RateLimiter::Bucket::reduce(const size_t tokens)
{
    const auto now = chrono::steady_clock::now();
    auto timediff = chrono::duration_cast<chrono::duration<double>>(now -
                                                               lastRefillTime_);
    const auto refillCount = static_cast<size_t>(floor(timediff.count() / rl_.refillTime_));

    clog << "   " << rl_.name_ << " reduce : ";

    if (refillCount > 0) {
        tokens_ = min<size_t>(tokens_ + (refillCount * rl_.refillAmount_), rl_.maxTokens_);

        if (tokens_ == rl_.maxTokens_) {
            // Prevent any time-skew from building up when the bucket is full
            lastRefillTime_ = now;
            clog << refillCount << " tokens. ";
        } else {
            auto duration = chrono::duration<double>(refillCount * rl_.refillTime_);
            auto ms = chrono::duration_cast<chrono::milliseconds>(duration);
            lastRefillTime_ = min<time_point_t>(now, lastRefillTime_ + ms);

            clog << refillCount << " tokens, "
                 << ms.count() << " ms duration ";
        }
    }

    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        clog << ' ' << tokens_ << " tokens left, OK" << endl;
        return true;
    }

    clog << " Denied." << endl;
    return false;
}

}
