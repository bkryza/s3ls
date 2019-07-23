#include <gflags/gflags.h>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/S3Errors.h>
#include <aws/s3/model/Delete.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

DEFINE_string(host, "", "S3 hostname");
DEFINE_string(bucket, "", "S3 bucket");
DEFINE_string(prefix, "", "S3 prefix");
DEFINE_string(accesskey, "", "S3 access key");
DEFINE_string(secretkey, "", "S3 secret key");
DEFINE_uint64(maxkeys, 1000, "Number of entries per request (max 1000)");
DEFINE_bool(verbose, false, "List all files and properties");
DEFINE_bool(https, false, "Use HTTPS");

using namespace std::chrono;

template <typename F, typename... Args>
static auto measure(F &&func, Args &&... args)
{
    auto start = steady_clock::now();
    auto outcome =
        std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    return std::pair<milliseconds, Aws::S3::Model::ListObjectsOutcome>(
        duration_cast<milliseconds>(steady_clock::now() - start), outcome);
}

int main(int argc, char **argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    unsigned long long totalSize{};
    unsigned long long totalObjectCount{};
    milliseconds totalTime{};

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    std::string bucket{FLAGS_bucket};
    std::string key{FLAGS_prefix};
    std::string host{FLAGS_host};

    std::string accessKey{FLAGS_accesskey};
    std::string secretKey{FLAGS_secretkey};

    Aws::Auth::AWSCredentials credentials{accessKey.c_str(), secretKey.c_str()};
    Aws::Client::ClientConfiguration configuration;
    configuration.endpointOverride = host.c_str();
    configuration.region = "us-east-1";
    if (FLAGS_https)
        configuration.scheme = Aws::Http::Scheme::HTTPS;
    else
        configuration.scheme = Aws::Http::Scheme::HTTP;

    auto client =
        std::make_unique<Aws::S3::S3Client>(credentials, configuration,
            Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

    Aws::S3::Model::ListObjectsRequest request;
    request.SetBucket(bucket.c_str());
    request.SetPrefix(key.c_str());
    request.SetMaxKeys(FLAGS_maxkeys);

    std::string marker{};
    unsigned int i{};

    while (true) {
        auto [duration, outcome] =
            measure([&]() { return client->ListObjects(request); });

        if (!outcome.IsSuccess()) {
            std::cout << "ERROR: " << outcome.GetError().GetMessage();
            return 1;
        }

        if (outcome.GetResult().GetContents().empty())
            break;

        std::cout << "Got objects [" << i << ", "
                  << i + outcome.GetResult().GetContents().size() << "] in "
                  << duration.count() << "ms\n";
        if (FLAGS_verbose) {
            for (auto it = outcome.GetResult().GetContents().cbegin();
                 it != outcome.GetResult().GetContents().cend(); it++) {
                std::cout << it->GetKey().c_str() << "\n";
            }
        }

        i += outcome.GetResult().GetContents().size();

        totalObjectCount += outcome.GetResult().GetContents().size();
        totalTime += duration;

        marker = outcome.GetResult().GetContents().back().GetKey().c_str();
        request.SetMarker(marker);
    }

    std::cout << "Total time: " << duration_cast<seconds>(totalTime).count()
              << "s\n";
    std::cout << "Total objects: " << totalObjectCount << "\n";

    return 0;
}
