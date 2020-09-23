#pragma once

#include <chrono>
#include <memory>

#include "Bucket.h"

namespace rl {

class RateLimiter
{
public:
    class Bucket
    {
    public:
        bool reduce(const size_t tokens);
        size_t get() const noexcept {
            return tokens_;
        }

        Bucket(size_t startTokens, const RateLimiter& rl):
            tokens_{startTokens}, rl_{rl}
        {}

    private:
        using time_point_t = std::chrono::steady_clock::time_point;
        time_point_t lastRefillTime_ = std::chrono::steady_clock::now();
        size_t tokens_;
        const RateLimiter& rl_;
    };

    RateLimiter(std::string name, double refillTime, double refillAmount, size_t maxTokens)
        : name_{name}, refillTime_{refillTime}, refillAmount_{refillAmount}
        , maxTokens_{maxTokens}
    {}

    std::unique_ptr<Bucket> createBucket() const;

private:
    const std::string name_;
    const double refillTime_;
    const double refillAmount_;
    const size_t maxTokens_;
};

} // ns
