/*! Code built upon https://github.com/grpc/grpc/blob/v1.31.0/examples/cpp/helloworld/greeter_async_server.cc
 */

#include <string>
#include <thread>
#include <unordered_map>

#include "RateLimitSvc.h"
#include "RateLimiter.h"
#include "ratelimit.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

using namespace std;

namespace rl {

class RateLimitSvcImpl : public RateLimitSvc {
    using service_t = ::pb::lyft::ratelimit::RateLimitService::AsyncService;

public:
    RateLimitSvcImpl(const Config& config)
        : config_{config}
    {}

    ~RateLimitSvcImpl() override {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void run() override {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(config_.listenAddress,
                                 grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);

        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();

        clog << ServerVersion() << " listening on " << config_.listenAddress << endl;
        processRequests();
    }

private:
    class Request {
    public:
        Request(RateLimitSvcImpl& rlsi, service_t* service, grpc::ServerCompletionQueue* cq)
            : rlsi_{rlsi}, service_{service}, cq_{cq}
        {
            proceed();
        }

        void proceed() {
            //clog << "proceed: called. Status is " << to_string(status_) << endl;

            if (status_ == CallStatus::CREATE) {
                status_ = CallStatus::PROCESS;

                //clog << "proceed: CallStatus::CREATE (queuing next receive)" << endl;

                service_->RequestShouldRateLimit(
                            &ctx_, &request_, &responder_, cq_, cq_, this);
                return;
            }

            if (status_ == CallStatus::PROCESS) {
                // Create a new instance for the next incoming request
                new Request{rlsi_, service_, cq_};

                clog << "Received message: domain=" << request_.domain()
                     << ", " << request_.descriptors().size() << " descriptors."
                     << endl;

                string key;

                // Iterate over the descriptors. Each descriptor
                // represent one label group in under the `ambassador` label
                // in the mapping definition for Ambassador
                for(const auto& rd : request_.descriptors()) {

                    // Iterate over the entries
                    for(const auto& kv : rd.entries()) {
                        clog << "   " << kv.key() << '=' << kv.value();

                        if (kv.key() == "auth") {
                           key = kv.value();
                        }
                    }

                    if (!rd.entries().empty()) {
                        clog << endl;
                    }
                }

                // Do actual rate limiting
                reply_.set_overall_code(
                            rlsi_.isWithinLimits(key)
                            ? ::pb::lyft::ratelimit::RateLimitResponse::OK
                            : ::pb::lyft::ratelimit::RateLimitResponse::OVER_LIMIT);

                status_ = CallStatus::FINISH;
                responder_.Finish(reply_, grpc::Status::OK, this);

                //clog << "proceed: Finished request" << endl;
                return;
            }

            //clog << "proceed: Bye then." << endl;
            GPR_ASSERT(status_ == CallStatus::FINISH);
            delete this;
        }

    private:
        RateLimitSvcImpl& rlsi_;
        service_t *service_ = {};
        grpc::ServerCompletionQueue* cq_ = {};
        grpc::ServerContext ctx_;
        ::pb::lyft::ratelimit::RateLimitRequest request_;
        ::pb::lyft::ratelimit::RateLimitResponse reply_;
        grpc::ServerAsyncResponseWriter<
        ::pb::lyft::ratelimit::RateLimitResponse> responder_{&ctx_};

        enum class CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_ = CallStatus::CREATE;

        string to_string(CallStatus cs) {
            static const array<string, 3> names_  = { "CREATE", "PROCESS", "FINISH" };
            return names_.at(static_cast<size_t>(cs));
        }
    };

    void processRequests() {
        new Request(*this, &service_, cq_.get());
        while(true) {
            void *tag = {};
            bool ok = true;
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<Request*>(tag)->proceed();
        }
    }

    bool isWithinLimits(const std::string& key) {
        // A naive "lock them all" approach is fine for this POC
        lock_guard<mutex> lock(mutex);

        auto it = buckets_.find(key);
        if (it == buckets_.end()) {
            if (!key.empty()) {
                it = buckets_.emplace(key, rl_.createBucket()).first;
            } else {
                // Shared bucket for all requests without authorization header
                it = buckets_.emplace(key, rlNoAuth_.createBucket()).first;
            }
        }
        assert(it !=  buckets_.end());
        return it->second->reduce(1);
    }

    const string& ServerVersion() {
    #define str1(s) #s
    #define str(s) str1(s)
      static const auto value = "rated "s + str(RATED_VERSION);
      return value;
    #undef str1
    #undef str
    }


    const Config config_;
    unique_ptr<grpc::ServerCompletionQueue> cq_;
    service_t service_;
    unique_ptr<grpc::Server> server_;
    mutex mutex_;
    RateLimiter rl_{"Authorized", 1, 1, 60};
    RateLimiter rlNoAuth_{"Shared", 5, 2, 100};
    unordered_map<string, unique_ptr<RateLimiter::Bucket>> buckets_;
};

std::unique_ptr<RateLimitSvc> RateLimitSvc::create(const Config& config)
{
    return make_unique<RateLimitSvcImpl>(config);
}

} // ns
